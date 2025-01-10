/**
 * @file fcollect.c
 * @brief Implementation of various fcollect algorithms for OpenSHMEM
 * collectives
 * @author Srdan Milakovic
 * @author Michael Beebe
 * @date Created on 5/17/18, edited on 1/4/25
 */

#include "shcoll.h"
#include "shcoll/compat.h"
#include "../tests/util/debug.h"
#include "util/rotate.h"

#include <limits.h>
#include <string.h>
#include <assert.h>

/**
 * Helper function implementing linear fcollect algorithm
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= 2
 */
inline static void fcollect_helper_linear(void *dest, const void *source,
                                          size_t nbytes, int PE_start,
                                          int logPE_stride, int PE_size,
                                          long *pSync) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;

  shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);
  if (me != PE_start) {
    shmem_putmem_nbi((char *)dest + me_as * nbytes, source, nbytes, PE_start);
  } else {
    memcpy(dest, source, nbytes);
  }
  shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);

  shcoll_broadcast8_linear(dest, dest, nbytes * shmem_n_pes(), PE_start,
                           PE_start, logPE_stride, PE_size, pSync + 1);
}

/**
 * Helper function implementing all-to-all linear fcollect algorithm
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= 1
 */
inline static void fcollect_helper_all_linear(void *dest, const void *source,
                                              size_t nbytes, int PE_start,
                                              int logPE_stride, int PE_size,
                                              long *pSync) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;

  int i;
  int target;

  for (i = 1; i < PE_size; i++) {
    target = PE_start + ((i + me_as) % PE_size) * stride;
    shmem_putmem_nbi((char *)dest + me_as * nbytes, source, nbytes, target);
  }

  memcpy((char *)dest + me_as * nbytes, source, nbytes);

  shmem_fence();

  for (i = 1; i < PE_size; i++) {
    target = PE_start + ((i + me_as) % PE_size) * stride;
    shmem_long_atomic_inc(pSync, target);
  }

  shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE + PE_size - 1);
  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);
}

/**
 * Helper function implementing all-to-all linear fcollect algorithm variant 1
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= 1
 */
inline static void fcollect_helper_all_linear1(void *dest, const void *source,
                                               size_t nbytes, int PE_start,
                                               int logPE_stride, int PE_size,
                                               long *pSync) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;

  int i;
  int target;

  for (i = 1; i < PE_size; i++) {
    target = PE_start + ((i + me_as) % PE_size) * stride;
    shmem_putmem_nbi((char *)dest + me_as * nbytes, source, nbytes, target);
  }

  memcpy((char *)dest + me_as * nbytes, source, nbytes);

  shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);
}

/**
 * Helper function implementing recursive doubling fcollect algorithm
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= ⌈log(max_rank)⌉
 */
inline static void fcollect_helper_rec_dbl(void *dest, const void *source,
                                           size_t nbytes, int PE_start,
                                           int logPE_stride, int PE_size,
                                           long *pSync) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;
  int mask;
  int peer;
  int i;
  int data_block = me_as;

  assert(((PE_size - 1) & PE_size) == 0);

  memcpy((char *)dest + me_as * nbytes, source, nbytes);

  for (mask = 0x1, i = 0; mask < PE_size; mask <<= 1, i++) {
    peer = PE_start + (me_as ^ mask) * stride;

    shmem_putmem_nbi((char *)dest + data_block * nbytes,
                     (char *)dest + data_block * nbytes, nbytes * mask, peer);
    shmem_fence();
    shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE + 1, peer);

    data_block &= ~mask;

    shmem_long_wait_until(pSync + i, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE, me);
  }
}

/**
 * Helper function implementing ring-based fcollect algorithm
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= 1
 */
inline static void fcollect_helper_ring(void *dest, const void *source,
                                        size_t nbytes, int PE_start,
                                        int logPE_stride, int PE_size,
                                        long *pSync) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;
  int peer = PE_start + ((me_as + 1) % PE_size) * stride;
  int data_block = me_as;
  int i;

  memcpy((char *)dest + data_block * nbytes, source, nbytes);

  for (i = 1; i < PE_size; i++) {
    shmem_putmem_nbi((char *)dest + data_block * nbytes,
                     (char *)dest + data_block * nbytes, nbytes, peer);
    shmem_fence();
    shmem_long_atomic_inc(pSync, peer);

    data_block = (data_block - 1 + PE_size) % PE_size;
    shmem_long_wait_until(pSync, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + i);
  }

  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);
}

/**
 * Helper function implementing Bruck's fcollect algorithm
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= ⌈log(max_rank)⌉
 */
inline static void fcollect_helper_bruck(void *dest, const void *source,
                                         size_t nbytes, int PE_start,
                                         int logPE_stride, int PE_size,
                                         long *pSync) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;
  size_t distance;
  int round;
  int peer;
  size_t sent_bytes = nbytes;
  size_t total_nbytes = PE_size * nbytes;
  size_t to_send;

  memcpy(dest, source, nbytes);

  for (distance = 1, round = 0; distance < PE_size; distance <<= 1, round++) {
    peer = (int)(PE_start + ((me_as - distance + PE_size) % PE_size) * stride);
    to_send = (2 * sent_bytes <= total_nbytes) ? sent_bytes
                                               : total_nbytes - sent_bytes;

    shmem_putmem_nbi((char *)dest + sent_bytes, dest, to_send, peer);
    shmem_fence();
    shmem_long_p(pSync + round, SHCOLL_SYNC_VALUE + 1, peer);

    sent_bytes += distance * nbytes;
    shmem_long_wait_until(pSync + round, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    shmem_long_p(pSync + round, SHCOLL_SYNC_VALUE, me);
  }

  rotate(dest, total_nbytes, me_as * nbytes);
}

/**
 * Helper function implementing Bruck's fcollect algorithm without final
 * rotation
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= ⌈log(max_rank)⌉
 */
inline static void fcollect_helper_bruck_no_rotate(void *dest,
                                                   const void *source,
                                                   size_t nbytes, int PE_start,
                                                   int logPE_stride,
                                                   int PE_size, long *pSync) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;
  size_t distance;
  int round;
  int peer;
  size_t sent_bytes = nbytes;
  size_t total_nbytes = PE_size * nbytes;
  size_t to_send;

  size_t my_offset_nbytes = nbytes * me_as;
  char *my_offset = (char *)dest + my_offset_nbytes;

  memcpy(my_offset, source, nbytes);

  for (distance = 1, round = 0; distance < PE_size; distance <<= 1, round++) {
    peer = (int)(PE_start + ((me_as - distance + PE_size) % PE_size) * stride);
    to_send = (2 * sent_bytes <= total_nbytes) ? sent_bytes
                                               : total_nbytes - sent_bytes;

    if (my_offset_nbytes + to_send <= total_nbytes) {
      shmem_putmem_nbi(my_offset, my_offset, to_send, peer);
    } else {
      shmem_putmem_nbi(my_offset, my_offset, total_nbytes - my_offset_nbytes,
                       peer);
      shmem_putmem_nbi(dest, dest, to_send - (total_nbytes - my_offset_nbytes),
                       peer);
    }

    shmem_fence();
    shmem_long_p(pSync + round, SHCOLL_SYNC_VALUE + 1, peer);

    sent_bytes += distance * nbytes;
    shmem_long_wait_until(pSync + round, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    shmem_long_p(pSync + round, SHCOLL_SYNC_VALUE, me);
  }
}

/**
 * Helper function implementing Bruck's fcollect algorithm with signal
 * operations
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= ⌈log(max_rank)⌉
 */
inline static void fcollect_helper_bruck_signal(void *dest, const void *source,
                                                size_t nbytes, int PE_start,
                                                int logPE_stride, int PE_size,
                                                long *pSync) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;
  size_t distance;
  int round;
  int peer;
  size_t sent_bytes = nbytes;
  size_t total_nbytes = PE_size * nbytes;
  size_t to_send;

  memcpy(dest, source, nbytes);

  for (distance = 1, round = 0; distance < PE_size; distance <<= 1, round++) {
    peer = (int)(PE_start + ((me_as - distance + PE_size) % PE_size) * stride);
    to_send = (2 * sent_bytes <= total_nbytes) ? sent_bytes
                                               : total_nbytes - sent_bytes;

    shmem_putmem_signal_nb((char *)dest + sent_bytes, dest, to_send,
                           (uint64_t *)(pSync + round), SHCOLL_SYNC_VALUE + 1,
                           peer, NULL);

    sent_bytes += distance * nbytes;
    shmem_long_wait_until(pSync + round, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    shmem_long_p(pSync + round, SHCOLL_SYNC_VALUE, me);
  }

  rotate(dest, total_nbytes, me_as * nbytes);
}

/**
 * Helper function implementing in-place Bruck's fcollect algorithm
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= ⌈log(max_rank)⌉
 */
inline static void fcollect_helper_bruck_inplace(void *dest, const void *source,
                                                 size_t nbytes, int PE_start,
                                                 int logPE_stride, int PE_size,
                                                 long *pSync) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;
  size_t distance;
  int round;
  int peer;
  size_t sent_bytes = nbytes;
  size_t total_nbytes = PE_size * nbytes;
  size_t to_send;

  memcpy(dest, source, nbytes);

  for (distance = 1, round = 0; distance < PE_size; distance <<= 1, round++) {
    peer = (int)(PE_start + ((me_as - distance + PE_size) % PE_size) * stride);
    to_send = (2 * sent_bytes <= total_nbytes) ? sent_bytes
                                               : total_nbytes - sent_bytes;

    shmem_putmem_nbi((char *)dest + sent_bytes, dest, to_send, peer);
    shmem_fence();
    shmem_long_p(pSync + round, SHCOLL_SYNC_VALUE + 1, peer);

    sent_bytes += distance * nbytes;
    shmem_long_wait_until(pSync + round, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    shmem_long_p(pSync + round, SHCOLL_SYNC_VALUE, me);
  }

  rotate_inplace(dest, total_nbytes, me_as * nbytes);
}

/**
 * Helper function implementing neighbor exchange fcollect algorithm
 *
 * @param dest Destination buffer on all PEs
 * @param source Source buffer containing local data
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array of size >= 2
 */
inline static void
fcollect_helper_neighbor_exchange(void *dest, const void *source, size_t nbytes,
                                  int PE_start, int logPE_stride, int PE_size,
                                  long *pSync) {
  assert(PE_size % 2 == 0);

  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();

  int neighbor_pe[2];
  int send_offset[2];
  int send_offset_diff;

  int i, parity;
  void *data;

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;

  if (me_as % 2 == 0) {
    neighbor_pe[0] = PE_start + ((me_as + 1) % PE_size) * stride;
    neighbor_pe[1] = PE_start + ((me_as - 1 + PE_size) % PE_size) * stride;

    send_offset[0] = (me_as - 2 + PE_size) % PE_size & ~0x1;
    send_offset[1] = me_as & ~0x1;

    send_offset_diff = 2;
  } else {
    neighbor_pe[0] = PE_start + ((me_as - 1 + PE_size) % PE_size) * stride;
    neighbor_pe[1] = PE_start + ((me_as + 1) % PE_size) * stride;

    send_offset[0] = (me_as + 2) % PE_size & ~0x1;
    send_offset[1] = me_as & ~0x1;

    send_offset_diff = -2 + PE_size;
  }

  /* First round */
  data = (char *)dest + me_as * nbytes;

  memcpy(data, source, nbytes);

  shmem_putmem_nbi(data, data, nbytes, neighbor_pe[0]);
  shmem_fence();
  shmem_long_atomic_inc(pSync, neighbor_pe[0]);

  shmem_long_wait_until(pSync, SHMEM_CMP_GE, 1);

  /* Remaining npes/2 - 1 rounds */
  for (i = 1; i < PE_size / 2; i++) {
    parity = (i % 2) ? 1 : 0;
    data = ((char *)dest) + send_offset[parity] * nbytes;

    /* Send data */
    shmem_putmem_nbi(data, data, 2 * nbytes, neighbor_pe[parity]);
    shmem_fence();
    shmem_long_atomic_inc(pSync + parity, neighbor_pe[parity]);

    /* Calculate offset for the next round */
    send_offset[parity] = (send_offset[parity] + send_offset_diff) % PE_size;
    send_offset_diff = PE_size - send_offset_diff;

    /* Wait for the data from the neighbor */
    shmem_long_wait_until(pSync + parity, SHMEM_CMP_GT, i / 2);
  }

  pSync[0] = SHCOLL_SYNC_VALUE;
  pSync[1] = SHCOLL_SYNC_VALUE;
}

/**
 * Macro to define fcollect functions for different data sizes
 *
 * @param _algo Algorithm name
 * @param _size Data size in bits
 */
#define SHCOLL_FCOLLECT_SIZE_DEFINITION(_algo, _size)                          \
  void shcoll_fcollect##_size##_##_algo(                                       \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size, long *pSync) {                            \
    fcollect_helper_##_algo(dest, source, (_size) / CHAR_BIT * nelems,         \
                            PE_start, logPE_stride, PE_size, pSync);           \
  }

/* @formatter:off */

SHCOLL_FCOLLECT_SIZE_DEFINITION(linear, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(linear, 64)

SHCOLL_FCOLLECT_SIZE_DEFINITION(all_linear, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(all_linear, 64)

SHCOLL_FCOLLECT_SIZE_DEFINITION(all_linear1, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(all_linear1, 64)

SHCOLL_FCOLLECT_SIZE_DEFINITION(rec_dbl, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(rec_dbl, 64)

SHCOLL_FCOLLECT_SIZE_DEFINITION(ring, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(ring, 64)

SHCOLL_FCOLLECT_SIZE_DEFINITION(bruck, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(bruck, 64)

SHCOLL_FCOLLECT_SIZE_DEFINITION(bruck_no_rotate, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(bruck_no_rotate, 64)

SHCOLL_FCOLLECT_SIZE_DEFINITION(bruck_signal, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(bruck_signal, 64)

SHCOLL_FCOLLECT_SIZE_DEFINITION(bruck_inplace, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(bruck_inplace, 64)

SHCOLL_FCOLLECT_SIZE_DEFINITION(neighbor_exchange, 32)
SHCOLL_FCOLLECT_SIZE_DEFINITION(neighbor_exchange, 64)

/* @formatter:on */

/**
 * Macro to define fcollect functions for different data types
 *
 * @param _algo Algorithm name
 * @param type Data type
 * @param _typename Type name string
 */
#define SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, type, _typename)                \
  int shcoll_##_typename##_fcollect_##_algo(                                   \
      shmem_team_t team, type *dest, const type *source, size_t nelems) {      \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    /* Allocate pSync from symmetric heap */                                   \
    long *pSync = shmem_malloc(SHCOLL_COLLECT_SYNC_SIZE * sizeof(long));       \
    if (!pSync)                                                                \
      return -1;                                                               \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_COLLECT_SYNC_SIZE; i++) {                       \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    /* Ensure all PEs have initialized pSync */                                \
    shmem_team_sync(team);                                                     \
    /* Zero out destination buffer */                                          \
    memset(dest, 0, sizeof(type) * nelems * PE_size);                          \
    /* Perform fcollect */                                                     \
    fcollect_helper_##_algo(dest, source,                                      \
                            sizeof(type) * nelems, /* total bytes per PE */    \
                            PE_start, logPE_stride, PE_size, pSync);           \
    /* Ensure collection is complete */                                        \
    shmem_team_sync(team);                                                     \
    /* Reset pSync before freeing */                                           \
    for (int i = 0; i < SHCOLL_COLLECT_SYNC_SIZE; i++) {                       \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    shmem_team_sync(team);                                                     \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

/* @formatter:off */

/**
 * Macro to define fcollect functions for all supported data types
 *
 * @param _algo Algorithm name
 */
#define DEFINE_SHCOLL_FCOLLECT_TYPES(_algo)                                    \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, float, float)                         \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, double, double)                       \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, long double, longdouble)              \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, unsigned char, uchar)                 \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, char, char)                           \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, signed char, schar)                   \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, short, short)                         \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, int, int)                             \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, long, long)                           \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, long long, longlong)                  \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, unsigned short, ushort)               \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, unsigned int, uint)                   \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, unsigned long, ulong)                 \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, unsigned long long, ulonglong)        \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, int8_t, int8)                         \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, int16_t, int16)                       \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, int32_t, int32)                       \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, int64_t, int64)                       \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, uint8_t, uint8)                       \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, uint16_t, uint16)                     \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, uint32_t, uint32)                     \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, uint64_t, uint64)                     \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, size_t, size)                         \
  SHCOLL_FCOLLECT_TYPE_DEFINITION(_algo, ptrdiff_t, ptrdiff)

/* @formatter:on */

DEFINE_SHCOLL_FCOLLECT_TYPES(linear)
DEFINE_SHCOLL_FCOLLECT_TYPES(all_linear)
DEFINE_SHCOLL_FCOLLECT_TYPES(all_linear1)
DEFINE_SHCOLL_FCOLLECT_TYPES(rec_dbl)
DEFINE_SHCOLL_FCOLLECT_TYPES(ring)
DEFINE_SHCOLL_FCOLLECT_TYPES(bruck)
DEFINE_SHCOLL_FCOLLECT_TYPES(bruck_no_rotate)
DEFINE_SHCOLL_FCOLLECT_TYPES(bruck_signal)
DEFINE_SHCOLL_FCOLLECT_TYPES(bruck_inplace)
DEFINE_SHCOLL_FCOLLECT_TYPES(neighbor_exchange)

/**
 * @brief Macro to declare fcollectmem implementations for different algorithms
 */
#define SHCOLL_FCOLLECTMEM_DEFINITION(_algo)                                   \
  int shcoll_fcollectmem_##_algo(shmem_team_t team, void *dest,                \
                                 const void *source, size_t nelems) {          \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    /* Allocate pSync from symmetric heap */                                   \
    long *pSync = shmem_malloc(SHCOLL_COLLECT_SYNC_SIZE * sizeof(long));       \
    if (!pSync)                                                                \
      return -1;                                                               \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_COLLECT_SYNC_SIZE; i++) {                       \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    /* Ensure all PEs have initialized pSync */                                \
    shmem_team_sync(team);                                                     \
    /* Zero out destination buffer */                                          \
    memset(dest, 0, nelems *PE_size);                                          \
    /* Perform fcollect */                                                     \
    fcollect_helper_##_algo(dest, source, nelems, PE_start, logPE_stride,      \
                            PE_size, pSync);                                   \
    /* Ensure collection is complete */                                        \
    shmem_team_sync(team);                                                     \
    /* Reset pSync before freeing */                                           \
    for (int i = 0; i < SHCOLL_COLLECT_SYNC_SIZE; i++) {                       \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    shmem_team_sync(team);                                                     \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

SHCOLL_FCOLLECTMEM_DEFINITION(linear)
SHCOLL_FCOLLECTMEM_DEFINITION(all_linear)
SHCOLL_FCOLLECTMEM_DEFINITION(all_linear1)
SHCOLL_FCOLLECTMEM_DEFINITION(rec_dbl)
SHCOLL_FCOLLECTMEM_DEFINITION(ring)
SHCOLL_FCOLLECTMEM_DEFINITION(bruck)
SHCOLL_FCOLLECTMEM_DEFINITION(bruck_no_rotate)
SHCOLL_FCOLLECTMEM_DEFINITION(bruck_signal)
SHCOLL_FCOLLECTMEM_DEFINITION(bruck_inplace)
SHCOLL_FCOLLECTMEM_DEFINITION(neighbor_exchange)
