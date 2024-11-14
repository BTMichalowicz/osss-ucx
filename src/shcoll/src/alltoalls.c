/**
 * @file alltoalls.c
 * @brief Implementation of strided all-to-all collective operations
 *
 * This file contains implementations of strided all-to-all collective
 * operations using different algorithms and synchronization methods:
 * - Shift exchange
 * - XOR pairwise exchange
 * - Color pairwise exchange
 *
 * Each algorithm has variants using different synchronization:
 * - Barrier-based
 * - Signal-based
 * - Counter-based
 */

/* For license: see LICENSE file at top-level */

#include "shcoll.h"
#include "shcoll/compat.h"

#include <string.h>
#include <limits.h>
#include <assert.h>

/**
 * @brief Helper macro to define barrier-based alltoalls implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to determine peer PE
 * @param _cond Additional condition for participation
 */
#define ALLTOALLS_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)               \
  inline static int alltoalls_helper_##_algo##_barrier(                        \
      void *dest, const void *source, ptrdiff_t dst_stride,                    \
      ptrdiff_t sst_stride, size_t nelems, int PE_start, int logPE_stride,     \
      int PE_size) {                                                           \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    uint8_t *dest_ptr = (uint8_t *)dest;                                       \
    const uint8_t *source_ptr = (const uint8_t *)source;                       \
                                                                               \
    /* Get my index in the active set */                                       \
    int me_as = (me - PE_start) / stride;                                      \
    if (me_as < 0 || me_as >= PE_size || _cond == 0) {                         \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Send my data to each PE */                                              \
    for (int i = 0; i < PE_size; i++) {                                        \
      int peer = PE_start + i * stride;                                        \
      const uint8_t *src = source_ptr + i * sst_stride;                        \
      uint8_t *dst = dest_ptr + me_as * dst_stride;                            \
      shmem_putmem(dst, src, nelems, peer);                                    \
    }                                                                          \
                                                                               \
    shmem_fence();                                                             \
    shmem_barrier_all();                                                       \
                                                                               \
    return 0;                                                                  \
  }

// FIXME: test to make sure this works
/**
 * @brief Helper macro to define signal-based alltoalls implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to determine peer PE
 * @param _cond Additional condition for participation
 */
#define ALLTOALLS_HELPER_SIGNAL_DEFINITION(_algo, _peer, _cond)                \
  inline static int alltoalls_helper_##_algo##_signal(                         \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size) {            \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    uint8_t *dest_ptr = (uint8_t *)dest;                                       \
    const uint8_t *source_ptr = (uint8_t *)source;                             \
    uint64_t *signal_ptr;                                                      \
                                                                               \
    /* Get my index in the active set */                                       \
    int me_as = (me - PE_start) / stride;                                      \
    if (me_as < 0 || me_as >= PE_size || _cond == 0) {                         \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Copy local data with stride */                                          \
    for (size_t i = 0; i < nelems; i++) {                                      \
      memcpy(dest_ptr + (me_as * dst + i) * nelems,                            \
             source_ptr + (me_as * sst + i) * nelems, nelems);                 \
    }                                                                          \
                                                                               \
    /* Exchange data with other PEs using signal */                            \
    for (int i = 0; i < PE_size; ++i) {                                        \
      if (i == me_as)                                                          \
        continue;                                                              \
                                                                               \
      const int peer_as = _peer(i, me_as, PE_size);                            \
      const int peer = PE_start + peer_as * stride;                            \
                                                                               \
      /* Ensure signal address is properly aligned */                          \
      signal_ptr = (uint64_t *)(dest_ptr + (me_as * dst + 1) * nelems);        \
                                                                               \
      /* Put data to peer with stride and signal */                            \
      shmem_putmem_signal_nbi(dest_ptr + me_as * dst * nelems,                 \
                              source_ptr + peer_as * sst * nelems, nelems,     \
                              signal_ptr, SHCOLL_SYNC_VALUE + 1,               \
                              SHMEM_SIGNAL_SET, peer);                         \
    }                                                                          \
                                                                               \
    /* Wait for all signals */                                                 \
    shmem_team_sync(SHMEM_TEAM_WORLD);                                         \
    return 0;                                                                  \
  }

// FIXME: test to make sure this works
/**
 * @brief Helper macro to define counter-based alltoalls implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to determine peer PE
 * @param _cond Additional condition for participation
 */
#define ALLTOALLS_HELPER_COUNTER_DEFINITION(_algo, _peer, _cond)               \
  inline static int alltoalls_helper_##_algo##_counter(                        \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size) {            \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    uint8_t *dest_ptr = (uint8_t *)dest;                                       \
    const uint8_t *source_ptr = (uint8_t *)source;                             \
                                                                               \
    /* Get my index in the active set */                                       \
    int me_as = (me - PE_start) / stride;                                      \
    if (me_as < 0 || me_as >= PE_size || _cond == 0) {                         \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Copy local data with stride */                                          \
    for (size_t i = 0; i < nelems; i++) {                                      \
      memcpy(dest_ptr + (me_as * dst + i) * nelems,                            \
             source_ptr + (me_as * sst + i) * nelems, nelems);                 \
    }                                                                          \
                                                                               \
    /* Exchange data with counter-based synchronization */                     \
    static long counter = 0;                                                   \
    for (int i = 0; i < PE_size; ++i) {                                        \
      if (i == me_as)                                                          \
        continue;                                                              \
                                                                               \
      const int peer_as = _peer(i, me_as, PE_size);                            \
      const int peer = PE_start + peer_as * stride;                            \
                                                                               \
      shmem_putmem_nbi(dest_ptr + me_as * dst * nelems,                        \
                       source_ptr + peer_as * sst * nelems, nelems, peer);     \
      shmem_fence();                                                           \
      shmem_long_atomic_inc(&counter, peer);                                   \
    }                                                                          \
                                                                               \
    /* Wait for all counters */                                                \
    shmem_long_wait_until(&counter, SHMEM_CMP_EQ, PE_size - 1);                \
    counter = 0;                                                               \
    return 0;                                                                  \
  }

/** @brief Calculate peer PE using shift pattern */
#define SHIFT_PEER(I, ME, NPES) (((ME) + (I)) % (NPES))
/** @brief Calculate peer PE using XOR pattern */
#define XOR_PEER(I, ME, NPES) ((ME) ^ (I))
/** @brief Calculate peer PE using color pattern */
#define COLOR_PEER(I, ME, NPES) ((((ME) / 2) * 2 + 1 - ((ME) % 2)) ^ (I))
#define XOR_COND 1
#define COLOR_COND ((me_as % 2) == 0 || (me_as + 1) < PE_size)

// clang-format off
ALLTOALLS_HELPER_BARRIER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_HELPER_COUNTER_DEFINITION(shift_exchange, SHIFT_PEER, 1)

ALLTOALLS_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALLS_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)

ALLTOALLS_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, COLOR_PEER, COLOR_COND)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(color_pairwise_exchange, COLOR_PEER, COLOR_COND)
ALLTOALLS_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, COLOR_PEER, COLOR_COND)
// clang-format on

/**
 * @brief Define type-specific alltoalls implementation
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string

 FIXME: is the stride being used correctly? Do we need the stide arg?
 */
#define SHCOLL_ALLTOALLS_DEFINITION(_algo, _type, _typename)                   \
  int shcoll_##_typename##_alltoalls_##_algo(                                  \
      shmem_team_t team, _type *dest, const _type *source, ptrdiff_t dst,      \
      ptrdiff_t sst, size_t nelems) {                                          \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    int ret = alltoalls_helper_##_algo(                                        \
        dest, source, dst * sizeof(_type), sst * sizeof(_type),                \
        nelems * sizeof(_type), PE_start, logPE_stride, PE_size);              \
    if (ret != 0) {                                                            \
      return -1;                                                               \
    }                                                                          \
    return 0;                                                                  \
  }

/**
 * @brief Define alltoalls implementations for all supported types
 *
 * @param _algo Algorithm name to generate implementations for
 */
#define DEFINE_SHCOLL_ALLTOALLS_TYPES(_algo)                                   \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, float, float)                             \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, double, double)                           \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, long double, longdouble)                  \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, char, char)                               \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, signed char, schar)                       \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, short, short)                             \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, int, int)                                 \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, long, long)                               \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, long long, longlong)                      \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, unsigned char, uchar)                     \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, unsigned short, ushort)                   \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, unsigned int, uint)                       \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, unsigned long, ulong)                     \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, unsigned long long, ulonglong)            \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, int8_t, int8)                             \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, int16_t, int16)                           \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, int32_t, int32)                           \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, int64_t, int64)                           \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, uint8_t, uint8)                           \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, uint16_t, uint16)                         \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, uint32_t, uint32)                         \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, uint64_t, uint64)                         \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, size_t, size)                             \
  SHCOLL_ALLTOALLS_DEFINITION(_algo, ptrdiff_t, ptrdiff)

DEFINE_SHCOLL_ALLTOALLS_TYPES(shift_exchange_barrier)
DEFINE_SHCOLL_ALLTOALLS_TYPES(shift_exchange_counter)
DEFINE_SHCOLL_ALLTOALLS_TYPES(shift_exchange_signal)
DEFINE_SHCOLL_ALLTOALLS_TYPES(xor_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALLS_TYPES(xor_pairwise_exchange_counter)
DEFINE_SHCOLL_ALLTOALLS_TYPES(xor_pairwise_exchange_signal)
DEFINE_SHCOLL_ALLTOALLS_TYPES(color_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALLS_TYPES(color_pairwise_exchange_counter)
DEFINE_SHCOLL_ALLTOALLS_TYPES(color_pairwise_exchange_signal)
