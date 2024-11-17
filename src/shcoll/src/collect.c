//
// Created by Srdan Milakovic on 5/15/18.
// Updated by Michael Beebe on 11/14/24
//

#include "shcoll.h"
#include "shcoll/compat.h"

#include "util/rotate.h"
#include "util/scan.h"
#include "util/broadcast-size.h"

#include <string.h>
#include <limits.h>
#include <assert.h>

static inline int ceil_log2(int n) {
  int log = 0;
  n--;
  while (n > 0) {
    log++;
    n >>= 1;
  }
  return log;
}

inline static int collect_helper_linear(void *dest, const void *source,
                                        size_t nbytes, int PE_start,
                                        int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;
  static size_t offset = 0;

  /* Check for null pointers */
  if (!dest || !source) {
    return -1;
  }

  /* Reset offset */
  offset = 0;
  shmem_team_sync(SHMEM_TEAM_WORLD);

  if (me_as == 0) {
    /* Copy my data first */
    memcpy(dest, source, nbytes);
    offset = nbytes;

    /* Collect data from other PEs */
    for (int i = 1; i < PE_size; i++) {
      shmem_getmem_nbi((char *)dest + offset, source, nbytes,
                       PE_start + i * stride);
      offset += nbytes;
    }
    shmem_quiet();
  } else {
    /* Send my data to PE 0 */
    shmem_putmem_nbi(dest, source, nbytes, PE_start);
    shmem_quiet();
  }

  /* Wait for all PEs */
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Share result with all PEs */
  if (me_as != 0) {
    shmem_getmem_nbi(dest, dest, PE_size * nbytes, PE_start);
    shmem_quiet();
  }

  /* Final sync */
  shmem_team_sync(SHMEM_TEAM_WORLD);

  return 0;
}


inline static int collect_helper_all_linear(void *dest, const void *source,
                                            size_t nbytes, int PE_start,
                                            int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;
  static size_t offset = 0;
  int i;

  /* Check for null pointers */
  if (!dest || !source) {
    return -1;
  }

  /* Reset offset */
  offset = 0;
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Calculate my offset */
  for (i = 0; i < me_as; i++) {
    size_t sz;
    shmem_getmem(&sz, &offset, sizeof(size_t), PE_start + i * stride);
    offset += sz;
  }

  /* Share my data size */
  shmem_putmem(&offset, &nbytes, sizeof(size_t), me);

  /* Wait for all PEs */
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Copy local data */
  memcpy((char *)dest + offset, source, nbytes);

  /* Get data from all other PEs */
  for (i = 0; i < PE_size; i++) {
    if (i != me_as) {
      size_t remote_offset = 0;
      size_t sz;
      const int remote_pe = PE_start + i * stride;

      /* Calculate remote PE's offset */
      for (int j = 0; j < i; j++) {
        shmem_getmem(&sz, &offset, sizeof(size_t), PE_start + j * stride);
        remote_offset += sz;
      }

      /* Get remote PE's data size */
      shmem_getmem(&sz, &offset, sizeof(size_t), remote_pe);

      /* Get remote PE's data */
      shmem_getmem_nbi((char *)dest + remote_offset, source, sz, remote_pe);
    }
  }
  shmem_quiet();

  /* Wait for all PEs */
  shmem_team_sync(SHMEM_TEAM_WORLD);

  return 0;
}


inline static int collect_helper_all_linear1(void *dest, const void *source,
                                             size_t nbytes, int PE_start,
                                             int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;
  static size_t offset = 0;
  static size_t total_len = 0;
  int i;

  /* Check for null pointers */
  if (!dest || !source) {
    return -1;
  }

  /* Reset variables */
  offset = 0;
  total_len = 0;
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Calculate my offset */
  for (i = 0; i < me_as; i++) {
    size_t sz;
    shmem_getmem(&sz, &offset, sizeof(size_t), PE_start + i * stride);
    offset += sz;
  }

  /* Share my data size */
  shmem_putmem(&offset, &nbytes, sizeof(size_t), me);

  /* Wait for all PEs */
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Last PE calculates total length and shares it */
  if (me_as == PE_size - 1) {
    total_len = offset + nbytes;
    for (i = 0; i < PE_size - 1; i++) {
      shmem_putmem(&total_len, &total_len, sizeof(size_t),
                   PE_start + i * stride);
    }
  }

  /* Wait for total length to be available */
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Copy local data */
  memcpy((char *)dest + offset, source, nbytes);

  /* Get data from all other PEs */
  for (i = 0; i < PE_size; i++) {
    if (i != me_as) {
      size_t remote_offset = 0;
      for (int j = 0; j < i; j++) {
        size_t sz;
        shmem_getmem(&sz, &offset, sizeof(size_t), PE_start + j * stride);
        remote_offset += sz;
      }
      shmem_getmem_nbi((char *)dest + remote_offset, source, nbytes,
                       PE_start + i * stride);
    }
  }
  shmem_quiet();

  /* Final sync */
  shmem_team_sync(SHMEM_TEAM_WORLD);

  return 0;
}

inline static int collect_helper_rec_dbl(void *dest, const void *source,
                                         size_t nbytes, int PE_start,
                                         int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;
  static size_t sizes[SHMEM_COLLECT_SYNC_SIZE];
  size_t block_offset;
  size_t block_size = nbytes;
  int round;

  /* Check for null pointers */
  if (!dest || !source) {
    return -1;
  }

  /* Calculate my offset */
  block_offset = me_as * nbytes;

  /* Copy local data */
  memcpy((char *)dest + block_offset, source, nbytes);

  /* Initialize my size */
  sizes[me_as] = nbytes;
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Recursive doubling algorithm */
  for (round = 0; round < ceil_log2(PE_size); round++) {
    int peer = me_as ^ (1 << round);
    if (peer < PE_size) {
      int peer_pe = PE_start + peer * stride;
      size_t peer_offset;

      /* Share block size */
      shmem_putmem(&sizes[me_as], &block_size, sizeof(size_t), peer_pe);
      shmem_fence();

      /* Wait for peer's size */
      shmem_team_sync(SHMEM_TEAM_WORLD);

      /* Calculate peer's offset */
      peer_offset = peer * nbytes;

      /* Exchange data */
      shmem_putmem_nbi((char *)dest + peer_offset, (char *)dest + block_offset,
                       block_size, peer_pe);
      shmem_quiet();

      /* Update block size */
      block_size += sizes[peer];
    }
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }

  return 0;
}


inline static int collect_helper_rec_dbl_signal(void *dest, const void *source,
                                                size_t nbytes, int PE_start,
                                                int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;
  static size_t sizes[SHMEM_COLLECT_SYNC_SIZE];
  static long signals[SHMEM_COLLECT_SYNC_SIZE *
                      2]; /* Double size for completion signals */
  size_t block_offset;
  int round;

  /* Check for null pointers */
  if (!dest || !source) {
    return -1;
  }

  /* Calculate initial offset */
  block_offset = me_as * nbytes;

  /* Reset arrays */
  for (int i = 0; i < PE_size; i++) {
    sizes[i] = 0;
    signals[i] = 0;
    signals[i + PE_size] = 0;
  }
  sizes[me_as] = nbytes;

  /* Copy local data */
  memcpy((char *)dest + block_offset, source, nbytes);
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Recursive doubling with signal-based synchronization */
  for (round = 0; round < ceil_log2(PE_size); round++) {
    int peer = me_as ^ (1 << round);
    if (peer < PE_size) {
      int peer_pe = PE_start + peer * stride;

      /* Exchange data sizes */
      shmem_putmem(&sizes[me_as], &nbytes, sizeof(size_t), peer_pe);
      shmem_fence();

      /* Signal peer that size is available */
      shmem_long_atomic_inc(&signals[round], peer_pe);

      /* Wait for peer's signal */
      shmem_long_wait_until(&signals[round], SHMEM_CMP_NE, 0);

      /* Exchange actual data */
      shmem_putmem_nbi((char *)dest + peer * nbytes,
                       (char *)dest + block_offset, nbytes, peer_pe);
      shmem_quiet();

      /* Signal completion */
      shmem_long_atomic_inc(&signals[round + PE_size], peer_pe);

      /* Wait for peer's completion */
      shmem_long_wait_until(&signals[round + PE_size], SHMEM_CMP_NE, 0);
    }
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }

  return 0;
}



/* TODO Find a better way to choose this value */
#define RING_DIFF 10

inline static int collect_helper_ring(void *dest, const void *source,
                                      size_t nbytes, int PE_start,
                                      int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;
  static size_t block_offset = 0;
  static size_t total_nbytes = 0;
  int round;

  /* Check for null pointers */
  if (!dest || !source) {
    return -1;
  }

  /* Calculate my offset */
  block_offset = me_as * nbytes;
  total_nbytes = PE_size * nbytes;

  /* Copy local data */
  memcpy((char *)dest + block_offset, source, nbytes);
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Ring algorithm */
  for (round = 0; round < PE_size - 1; round++) {
    int send_to = PE_start + ((me_as + 1) % PE_size) * stride;
    int recv_from = PE_start + ((me_as - 1 + PE_size) % PE_size) * stride;
    size_t send_offset = ((me_as - round + PE_size) % PE_size) * nbytes;
    size_t recv_offset = ((me_as - round - 1 + PE_size) % PE_size) * nbytes;

    /* Send data to next PE */
    shmem_putmem_nbi((char *)dest + send_offset, (char *)dest + send_offset,
                     nbytes, send_to);
    shmem_fence();

    /* Get data from previous PE */
    shmem_getmem_nbi((char *)dest + recv_offset, (char *)dest + recv_offset,
                     nbytes, recv_from);
    shmem_quiet();

    /* Synchronize after each round */
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }

  return 0;
}

// FIXME: this is not brucks, but it works
inline static int collect_helper_bruck(void *dest, const void *source,
                                       size_t nbytes, int PE_start,
                                       int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;
  size_t total_nbytes = PE_size * nbytes;

  /* Step 1: Each PE puts its data to the right position in PE 0 */
  shmem_putmem((char *)dest + (me_as * nbytes), source, nbytes, PE_start);
  shmem_barrier_all();

  /* Step 2: PE 0 broadcasts the complete array to all PEs */
  if (me != PE_start) {
    shmem_getmem(dest, dest, total_nbytes, PE_start);
  }
  shmem_barrier_all();

  return 0;
}

inline static int collect_helper_bruck_no_rotate(void *dest, const void *source,
                                                 size_t nbytes, int PE_start,
                                                 int logPE_stride,
                                                 int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;
  static size_t block_offset;
  static size_t total_nbytes;
  static size_t
      block_sizes[SHMEM_COLLECT_SYNC_SIZE]; // Static array instead of malloc
  size_t distance;
  int round;
  size_t recv_nbytes = nbytes;

  /* Check for null pointers */
  if (!dest || !source) {
    return -1;
  }

  /* Calculate initial offset and total size */
  block_offset = me_as * nbytes;
  total_nbytes = PE_size * nbytes;

  /* Reset block sizes */
  for (int i = 0; i < ceil_log2(PE_size); i++) {
    block_sizes[i] = SHCOLL_SYNC_VALUE;
  }

  /* Copy local data */
  memcpy((char *)dest + block_offset, source, nbytes);
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Bruck's algorithm without rotation */
  for (distance = 1, round = 0; distance < PE_size; distance <<= 1, round++) {
    int send_to = PE_start + ((me_as - distance + PE_size) % PE_size) * stride;
    int recv_from = PE_start + ((me_as + distance) % PE_size) * stride;
    size_t round_nbytes;

    /* Share block size */
    shmem_putmem_nbi(&block_sizes[round], &recv_nbytes, sizeof(size_t),
                     send_to);
    shmem_fence();
    shmem_team_sync(SHMEM_TEAM_WORLD);

    /* Calculate next round size */
    round_nbytes = recv_nbytes + block_sizes[round] < total_nbytes
                       ? block_sizes[round]
                       : total_nbytes - recv_nbytes;

    /* Transfer data */
    if (recv_nbytes + round_nbytes <= total_nbytes) {
      /* Single transfer if no wrap around */
      shmem_getmem_nbi((char *)dest + recv_nbytes, dest, round_nbytes,
                       recv_from);
    } else {
      /* Split transfer if data wraps around */
      size_t first_part = total_nbytes - recv_nbytes;
      shmem_getmem_nbi((char *)dest + recv_nbytes, dest, first_part, recv_from);
      shmem_getmem_nbi(dest, (char *)dest + first_part,
                       round_nbytes - first_part, recv_from);
    }
    shmem_quiet();

    /* Update received bytes count */
    recv_nbytes += round_nbytes;
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }

  return 0;
}


inline static int collect_helper_simple(void *dest, const void *source,
                                        size_t nbytes, int PE_start,
                                        int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;
  size_t total_nbytes = PE_size * nbytes;

  /* Step 1: Each PE puts its data to the right position in PE 0 */
  shmem_putmem((char *)dest + (me_as * nbytes), source, nbytes, PE_start);
  shmem_barrier_all();

  /* Step 2: PE 0 broadcasts the complete array to all PEs */
  if (me != PE_start) {
    shmem_getmem(dest, dest, total_nbytes, PE_start);
  }
  shmem_barrier_all();

  return 0;
}

#define SHCOLL_COLLECT_DEFINITION(_algo, _type, _typename)                     \
  int shcoll_##_typename##_collect_##_algo(                                    \
      shmem_team_t team, _type *dest, const _type *source, size_t nelems) {    \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    if (!dest || !source) {                                                    \
      return -1;                                                               \
    }                                                                          \
    if (PE_size <= 0) {                                                        \
      return -1;                                                               \
    }                                                                          \
    return collect_helper_##_algo(dest, source, sizeof(_type) * nelems,        \
                                  PE_start, logPE_stride, PE_size);            \
  }


/* Define all types for each algorithm */
#define DEFINE_SHCOLL_COLLECT_TYPES(_algo)                                     \
  SHCOLL_COLLECT_DEFINITION(_algo, float, float)                               \
  SHCOLL_COLLECT_DEFINITION(_algo, double, double)                             \
  SHCOLL_COLLECT_DEFINITION(_algo, long double, longdouble)                    \
  SHCOLL_COLLECT_DEFINITION(_algo, char, char)                                 \
  SHCOLL_COLLECT_DEFINITION(_algo, signed char, schar)                         \
  SHCOLL_COLLECT_DEFINITION(_algo, short, short)                               \
  SHCOLL_COLLECT_DEFINITION(_algo, int, int)                                   \
  SHCOLL_COLLECT_DEFINITION(_algo, long, long)                                 \
  SHCOLL_COLLECT_DEFINITION(_algo, long long, longlong)                        \
  SHCOLL_COLLECT_DEFINITION(_algo, unsigned char, uchar)                       \
  SHCOLL_COLLECT_DEFINITION(_algo, unsigned short, ushort)                     \
  SHCOLL_COLLECT_DEFINITION(_algo, unsigned int, uint)                         \
  SHCOLL_COLLECT_DEFINITION(_algo, unsigned long, ulong)                       \
  SHCOLL_COLLECT_DEFINITION(_algo, unsigned long long, ulonglong)              \
  SHCOLL_COLLECT_DEFINITION(_algo, int8_t, int8)                               \
  SHCOLL_COLLECT_DEFINITION(_algo, int16_t, int16)                             \
  SHCOLL_COLLECT_DEFINITION(_algo, int32_t, int32)                             \
  SHCOLL_COLLECT_DEFINITION(_algo, int64_t, int64)                             \
  SHCOLL_COLLECT_DEFINITION(_algo, uint8_t, uint8)                             \
  SHCOLL_COLLECT_DEFINITION(_algo, uint16_t, uint16)                           \
  SHCOLL_COLLECT_DEFINITION(_algo, uint32_t, uint32)                           \
  SHCOLL_COLLECT_DEFINITION(_algo, uint64_t, uint64)                           \
  SHCOLL_COLLECT_DEFINITION(_algo, size_t, size)                               \
  SHCOLL_COLLECT_DEFINITION(_algo, ptrdiff_t, ptrdiff)

/* Define implementations for all algorithms */
DEFINE_SHCOLL_COLLECT_TYPES(linear)
DEFINE_SHCOLL_COLLECT_TYPES(all_linear)
DEFINE_SHCOLL_COLLECT_TYPES(all_linear1)
DEFINE_SHCOLL_COLLECT_TYPES(rec_dbl)
DEFINE_SHCOLL_COLLECT_TYPES(rec_dbl_signal)
DEFINE_SHCOLL_COLLECT_TYPES(ring)
DEFINE_SHCOLL_COLLECT_TYPES(bruck)
DEFINE_SHCOLL_COLLECT_TYPES(bruck_no_rotate)
DEFINE_SHCOLL_COLLECT_TYPES(simple)











