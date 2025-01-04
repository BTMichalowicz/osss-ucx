/**
 * @file fcollect.c
 * @brief Implementation of various fcollect (fixed-size collect) algorithms for
 * SHMEM
 *
 * This file contains multiple implementations of the fcollect collective
 * operation, which collects fixed-size data from all processing elements (PEs)
 * in a team.
 */

#include "shcoll.h"
#include "shcoll/compat.h"
#include "../tests/util/debug.h"
#include "util/rotate.h"

#include <limits.h>
#include <string.h>
#include <assert.h>

#include <shmem/teams.h>
#include <shcoll/common.h>

/**
 * Calculate ceiling of log base 2 of x
 */
static inline int ceil_log2(int x) {
  int r = 0;
  x--;
  while (x > 0) {
    x >>= 1;
    r++;
  }
  return r;
}

/**
 * Linear fcollect algorithm where each PE sends data to next PE in a ring
 * pattern
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @note pSync should have at least 2 elements
 */
inline static void fcollect_helper_linear(void *dest, const void *source,
                                          size_t nbytes, int PE_start,
                                          int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;
  int peer = PE_start + ((me_as + 1) % PE_size) * stride;
  int data_block = me_as;

  memcpy((char *)dest + data_block * nbytes, source, nbytes);
  shmem_team_sync(SHMEM_TEAM_WORLD);

  for (int i = 1; i < PE_size; i++) {
    shmem_putmem_nbi((char *)dest + data_block * nbytes,
                     (char *)dest + data_block * nbytes, nbytes, peer);
    shmem_fence();

    data_block = (data_block - 1 + PE_size) % PE_size;
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }
}

/**
 * All-to-all linear fcollect algorithm where each PE sends its data to all
 * other PEs
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 */
inline static void fcollect_helper_all_linear(void *dest, const void *source,
                                              size_t nbytes, int PE_start,
                                              int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;

  int i;
  int target;

  /* Send my data to all other PEs */
  for (i = 1; i < PE_size; i++) {
    target = PE_start + ((i + me_as) % PE_size) * stride;
    shmem_putmem_nbi((char *)dest + me_as * nbytes, source, nbytes, target);
  }

  /* Copy my own data locally */
  memcpy((char *)dest + me_as * nbytes, source, nbytes);

  shmem_fence();
  shmem_team_sync(SHMEM_TEAM_WORLD);
}

/**
 * All-to-all linear fcollect algorithm with binomial tree barrier
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 */
inline static void fcollect_helper_all_linear1(void *dest, const void *source,
                                               size_t nbytes, int PE_start,
                                               int logPE_stride, int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;

  int i;
  int target;

  /* Send my data to all other PEs */
  for (i = 1; i < PE_size; i++) {
    target = PE_start + ((i + me_as) % PE_size) * stride;
    shmem_putmem_nbi((char *)dest + me_as * nbytes, source, nbytes, target);
  }

  /* Copy my own data locally */
  memcpy((char *)dest + me_as * nbytes, source, nbytes);

  /* Use team sync */
  shmem_team_sync(SHMEM_TEAM_WORLD);
}

/**
 * Recursive doubling fcollect algorithm
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @note PE_size must be a power of 2
 */
inline static void fcollect_helper_rec_dbl(void *dest, const void *source,
                                           size_t nbytes, int PE_start,
                                           int logPE_stride, int PE_size) {
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
    data_block &= ~mask;
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }
}

/**
 * Ring-based fcollect algorithm
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @note pSync should have at least 1 element
 */
inline static void fcollect_helper_ring(void *dest, const void *source,
                                        size_t nbytes, int PE_start,
                                        int logPE_stride, int PE_size) {
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
    data_block = (data_block - 1 + PE_size) % PE_size;
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }
}

/**
 * Bruck's fcollect algorithm
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @note pSync should have at least ⌈log(max_rank)⌉ elements
 */
inline static void fcollect_helper_bruck(void *dest, const void *source,
                                         size_t nbytes, int PE_start,
                                         int logPE_stride, int PE_size) {
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
    sent_bytes += distance * nbytes;
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }

  rotate(dest, total_nbytes, me_as * nbytes);
}

/**
 * Bruck's fcollect algorithm without final rotation step
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @note pSync should have at least ⌈log(max_rank)⌉ elements
 */
inline static void
fcollect_helper_bruck_no_rotate(void *dest, const void *source, size_t nbytes,
                                int PE_start, int logPE_stride, int PE_size) {
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
    sent_bytes += distance * nbytes;
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }
}

/**
 * Bruck's fcollect algorithm with signaling
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @note pSync should have at least ⌈log(max_rank)⌉ elements
 */
inline static void fcollect_helper_bruck_signal(void *dest, const void *source,
                                                size_t nbytes, int PE_start,
                                                int logPE_stride, int PE_size) {
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
    sent_bytes += distance * nbytes;
    shmem_team_sync(SHMEM_TEAM_WORLD);
  }

  rotate(dest, total_nbytes, me_as * nbytes);
}

/**
 * In-place variant of Bruck's fcollect algorithm
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @note pSync should have at least ⌈log(max_rank)⌉ elements
 */
inline static void fcollect_helper_bruck_inplace(void *dest, const void *source,
                                                 size_t nbytes, int PE_start,
                                                 int logPE_stride,
                                                 int PE_size) {
  const int stride = 1 << logPE_stride;
  const int me = shmem_my_pe();
  const int me_as = (me - PE_start) / stride;

  /* Copy my data to my position */
  memcpy((char *)dest + me_as * nbytes, source, nbytes);
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Each PE broadcasts its data to all others */
  for (int i = 0; i < PE_size; i++) {
    if (i != me_as) {
      int target_pe = PE_start + i * stride;
      shmem_putmem((char *)dest + me_as * nbytes, (char *)dest + me_as * nbytes,
                   nbytes, target_pe);
    }
  }

  shmem_team_sync(SHMEM_TEAM_WORLD);
}

/**
 * Neighbor exchange fcollect algorithm for even number of PEs
 *
 * @param dest Symmetric destination buffer
 * @param source Local source buffer containing data to collect
 * @param nbytes Number of bytes to collect from each PE
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @note pSync should have at least 2 elements
 * @note PE_size must be even
 */
inline static void
fcollect_helper_neighbor_exchange(void *dest, const void *source, size_t nbytes,
                                  int PE_start, int logPE_stride, int PE_size) {
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
  shmem_team_sync(SHMEM_TEAM_WORLD);

  /* Remaining npes/2 - 1 rounds */
  for (i = 1; i < PE_size / 2; i++) {
    parity = (i % 2) ? 1 : 0;
    data = ((char *)dest) + send_offset[parity] * nbytes;

    /* Send data */
    shmem_putmem_nbi(data, data, 2 * nbytes, neighbor_pe[parity]);
    shmem_fence();

    /* Calculate offset for the next round */
    send_offset[parity] = (send_offset[parity] + send_offset_diff) % PE_size;
    send_offset_diff = PE_size - send_offset_diff;

    shmem_team_sync(SHMEM_TEAM_WORLD);
  }
}

// FIXME: should logPE_stride be something else?
/* TODO: use teams-based logic if we use this macro */

/**
 * Macro to define fcollect functions for different data types
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_FCOLLECT_DEFINITION(_algo, _type, _typename)                    \
  int shcoll_##_typename##_fcollect_##_algo(                                   \
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
    fcollect_helper_##_algo(dest, source, sizeof(_type) * nelems, PE_start,    \
                            logPE_stride, PE_size);                            \
    return 0;                                                                  \
  }

/**
 * Macro to define fcollect functions for all supported data types
 *
 * @param _algo Algorithm name
 */
#define DEFINE_SHCOLL_FCOLLECT_TYPES(_algo)                                    \
  SHCOLL_FCOLLECT_DEFINITION(_algo, float, float)                              \
  SHCOLL_FCOLLECT_DEFINITION(_algo, double, double)                            \
  SHCOLL_FCOLLECT_DEFINITION(_algo, long double, longdouble)                   \
  SHCOLL_FCOLLECT_DEFINITION(_algo, char, char)                                \
  SHCOLL_FCOLLECT_DEFINITION(_algo, signed char, schar)                        \
  SHCOLL_FCOLLECT_DEFINITION(_algo, short, short)                              \
  SHCOLL_FCOLLECT_DEFINITION(_algo, int, int)                                  \
  SHCOLL_FCOLLECT_DEFINITION(_algo, long, long)                                \
  SHCOLL_FCOLLECT_DEFINITION(_algo, long long, longlong)                       \
  SHCOLL_FCOLLECT_DEFINITION(_algo, unsigned char, uchar)                      \
  SHCOLL_FCOLLECT_DEFINITION(_algo, unsigned short, ushort)                    \
  SHCOLL_FCOLLECT_DEFINITION(_algo, unsigned int, uint)                        \
  SHCOLL_FCOLLECT_DEFINITION(_algo, unsigned long, ulong)                      \
  SHCOLL_FCOLLECT_DEFINITION(_algo, unsigned long long, ulonglong)             \
  SHCOLL_FCOLLECT_DEFINITION(_algo, int8_t, int8)                              \
  SHCOLL_FCOLLECT_DEFINITION(_algo, int16_t, int16)                            \
  SHCOLL_FCOLLECT_DEFINITION(_algo, int32_t, int32)                            \
  SHCOLL_FCOLLECT_DEFINITION(_algo, int64_t, int64)                            \
  SHCOLL_FCOLLECT_DEFINITION(_algo, uint8_t, uint8)                            \
  SHCOLL_FCOLLECT_DEFINITION(_algo, uint16_t, uint16)                          \
  SHCOLL_FCOLLECT_DEFINITION(_algo, uint32_t, uint32)                          \
  SHCOLL_FCOLLECT_DEFINITION(_algo, uint64_t, uint64)                          \
  SHCOLL_FCOLLECT_DEFINITION(_algo, size_t, size)                              \
  SHCOLL_FCOLLECT_DEFINITION(_algo, ptrdiff_t, ptrdiff)

/* Define fcollect functions for all algorithms and data types */
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
