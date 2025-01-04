/**
 * @file alltoall.c
 * @brief Implementation of all-to-all collective operations
 *
 * This file contains implementations of all-to-all collective operations using
 * different algorithms and synchronization methods:
 * - Shift exchange
 * - XOR pairwise exchange
 * - Color pairwise exchange
 *
 * Each algorithm has variants using different synchronization:
 * - Barrier-based
 * - Signal-based
 * - Counter-based
 */

/*
 * For license: see LICENSE file at top-level
 */

#include "shcoll.h"
#include "shcoll/compat.h"

#include <string.h>
#include <limits.h>
#include <assert.h>

/**
 * @brief Calculate edge color for color pairwise exchange algorithm
 *
 * @param i Current iteration index
 * @param me Current PE index
 * @param npes Total number of PEs
 * @return Edge color value
 */
inline static int edge_color(int i, int me, int npes) {
  int chr_idx;
  int v;

  chr_idx = npes % 2 == 1 ? npes : npes - 1;
  if (me < chr_idx) {
    v = (i + chr_idx - me) % chr_idx;
  } else {
    v = i % 2 == 1 ? (((i + chr_idx) / 2) % chr_idx) : i / 2;
  }

  if (npes % 2 == 1 && v == me) {
    return -1;
  } else if (v == me) {
    return chr_idx;
  } else {
    return v;
  }
}

/** @brief Number of rounds between synchronizations */
static int alltoall_rounds_sync = INT32_MAX;

/**
 * @brief Set number of rounds between synchronizations for alltoalls
 * @param rounds_sync Number of rounds between synchronizations
 */
void shcoll_set_alltoalls_round_sync(int rounds_sync) {
  alltoall_rounds_sync = rounds_sync;
}

/**
 * @brief Helper macro to define barrier-based alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to determine peer PE
 * @param _cond Additional condition for participation
 */
#define ALLTOALL_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)                \
  inline static int alltoall_helper_##_algo##_barrier(                         \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size) {                                         \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    /* Get my index in the active set */                                       \
    const int me_as = (me - PE_start) / stride;                                \
                                                                               \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * nelems;                 \
    void const *source_ptr = ((uint8_t *)source) + me_as * nelems;             \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    /* Validate the condition */                                               \
    if (!(_cond)) {                                                            \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Copy own data to destination */                                         \
    memcpy(dest_ptr, source_ptr, nelems);                                      \
                                                                               \
    /* Perform non-blocking put operations to all peers */                     \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      source_ptr = ((uint8_t *)source) + peer_as * nelems;                     \
                                                                               \
      /* Ensure peer_as is within valid range */                               \
      if (peer_as < 0 || peer_as >= PE_size) {                                 \
        return -1;                                                             \
      }                                                                        \
                                                                               \
      /* Perform non-blocking put */                                           \
      shmem_putmem_nbi(dest_ptr, source_ptr, nelems,                           \
                       PE_start + peer_as * stride);                           \
                                                                               \
      /* Periodically synchronize to ensure progress */                        \
      if (i % alltoall_rounds_sync == 0) {                                     \
        if (shmem_team_sync(SHMEM_TEAM_WORLD) != SHMEM_SUCCESS) {              \
          return -1;                                                           \
        }                                                                      \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* Final synchronization after all puts */                                 \
    if (shmem_team_sync(SHMEM_TEAM_WORLD) != SHMEM_SUCCESS) {                  \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* If all operations are successful */                                     \
    return 0;                                                                  \
  }

// FIXME: test to make sure this works
/**
 * @brief Helper macro to define counter-based alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to determine peer PE
 * @param _cond Additional condition for participation
 */
#define ALLTOALL_HELPER_COUNTER_DEFINITION(_algo, _peer, _cond)                \
  inline static int alltoall_helper_##_algo##_counter(                         \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size) {                                         \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    /* Get my index in the active set */                                       \
    const int me_as = (me - PE_start) / stride;                                \
                                                                               \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * nelems;                 \
    void const *source_ptr;                                                    \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    /* Validate the condition */                                               \
    if (!(_cond)) {                                                            \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
                                                                               \
      /* Ensure peer_as is within valid range */                               \
      if (peer_as < 0 || peer_as >= PE_size) {                                 \
        return -1;                                                             \
      }                                                                        \
                                                                               \
      source_ptr = ((uint8_t *)source) + peer_as * nelems;                     \
                                                                               \
      /* Perform non-blocking put */                                           \
      shmem_putmem_nbi(dest_ptr, source_ptr, nelems,                           \
                       PE_start + peer_as * stride);                           \
    }                                                                          \
                                                                               \
    source_ptr = ((uint8_t *)source) + me_as * nelems;                         \
    memcpy(dest_ptr, source_ptr, nelems);                                      \
                                                                               \
    /* Ensure all non-blocking operations are complete */                      \
    shmem_fence();                                                             \
                                                                               \
    /* Final synchronization */                                                \
    if (shmem_team_sync(SHMEM_TEAM_WORLD) != SHMEM_SUCCESS) {                  \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* If all operations are successful */                                     \
    return 0;                                                                  \
  }

// FIXME: test to make sure this works
/**
 * @brief Helper macro to define signal-based alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to determine peer PE
 * @param _cond Additional condition for participation
 */
#define ALLTOALL_HELPER_SIGNAL_DEFINITION(_algo, _peer, _cond)                 \
  inline static int alltoall_helper_##_algo##_signal(                          \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size) {                                         \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    /* Get my index in the active set */                                       \
    const int me_as = (me - PE_start) / stride;                                \
                                                                               \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * nelems;                 \
    void const *source_ptr;                                                    \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    /* Validate the condition */                                               \
    if (!(_cond)) {                                                            \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
                                                                               \
      /* Ensure peer_as is within valid range */                               \
      if (peer_as < 0 || peer_as >= PE_size) {                                 \
        return -1;                                                             \
      }                                                                        \
                                                                               \
      source_ptr = ((uint8_t *)source) + peer_as * nelems;                     \
                                                                               \
      /* Perform non-blocking signal put */                                    \
      shmem_putmem_signal_nb(dest_ptr, source_ptr, nelems, NULL,               \
                             SHCOLL_SYNC_VALUE + 1,                            \
                             PE_start + peer_as * stride, NULL);               \
    }                                                                          \
                                                                               \
    source_ptr = ((uint8_t *)source) + me_as * nelems;                         \
    memcpy(dest_ptr, source_ptr, nelems);                                      \
                                                                               \
    /* Final synchronization */                                                \
    if (shmem_team_sync(SHMEM_TEAM_WORLD) != SHMEM_SUCCESS) {                  \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* If all operations are successful */                                     \
    return 0;                                                                  \
  }

// @formatter:off

/** @brief Calculate peer PE using shift pattern */
#define SHIFT_PEER(I, ME, NPES) (((ME) + (I)) % (NPES))
ALLTOALL_HELPER_BARRIER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALL_HELPER_COUNTER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALL_HELPER_SIGNAL_DEFINITION(shift_exchange, SHIFT_PEER,
                                  PE_size - 1 <= SHCOLL_ALLTOALL_SYNC_SIZE)

/** @brief Calculate peer PE using XOR pattern */
#define XOR_PEER(I, ME, NPES) ((I) ^ (ME))
#define XOR_COND (((PE_size - 1) & PE_size) == 0)

ALLTOALL_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALL_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALL_HELPER_SIGNAL_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                  XOR_COND &&PE_size - 1 <=
                                      SHCOLL_ALLTOALL_SYNC_SIZE)

/** @brief Calculate peer PE using color pattern */
#define COLOR_PEER(I, ME, NPES) edge_color(I, ME, NPES)
#define COLOR_COND (PE_size % 2 == 0)

ALLTOALL_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                   COLOR_COND)
ALLTOALL_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                   COLOR_COND)
ALLTOALL_HELPER_SIGNAL_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                  (PE_size - 1 <= SHCOLL_ALLTOALL_SYNC_SIZE) &&
                                      COLOR_COND)

// @formatter:on

/**
 * @brief Define type-specific alltoall implementation
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_ALLTOALL_DEFINITION(_algo, _type, _typename)                    \
  int shcoll_##_typename##_alltoall_##_algo(                                   \
      shmem_team_t team, _type *dest, const _type *source, size_t nelems) {    \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    int ret = alltoall_helper_##_algo(dest, source, sizeof(_type) * nelems,    \
                                      PE_start, logPE_stride, PE_size);        \
    if (ret != 0) {                                                            \
      return -1;                                                               \
    }                                                                          \
    return 0;                                                                  \
  }

// @formatter:off

/**
 * @brief Define alltoall implementations for all supported types
 *
 * @param _algo Algorithm name to generate implementations for
 */
#define DEFINE_SHCOLL_ALLTOALL_TYPES(_algo)                                    \
  SHCOLL_ALLTOALL_DEFINITION(_algo, float, float)                              \
  SHCOLL_ALLTOALL_DEFINITION(_algo, double, double)                            \
  SHCOLL_ALLTOALL_DEFINITION(_algo, long double, longdouble)                   \
  SHCOLL_ALLTOALL_DEFINITION(_algo, char, char)                                \
  SHCOLL_ALLTOALL_DEFINITION(_algo, signed char, schar)                        \
  SHCOLL_ALLTOALL_DEFINITION(_algo, short, short)                              \
  SHCOLL_ALLTOALL_DEFINITION(_algo, int, int)                                  \
  SHCOLL_ALLTOALL_DEFINITION(_algo, long, long)                                \
  SHCOLL_ALLTOALL_DEFINITION(_algo, long long, longlong)                       \
  SHCOLL_ALLTOALL_DEFINITION(_algo, unsigned char, uchar)                      \
  SHCOLL_ALLTOALL_DEFINITION(_algo, unsigned short, ushort)                    \
  SHCOLL_ALLTOALL_DEFINITION(_algo, unsigned int, uint)                        \
  SHCOLL_ALLTOALL_DEFINITION(_algo, unsigned long, ulong)                      \
  SHCOLL_ALLTOALL_DEFINITION(_algo, unsigned long long, ulonglong)             \
  SHCOLL_ALLTOALL_DEFINITION(_algo, int8_t, int8)                              \
  SHCOLL_ALLTOALL_DEFINITION(_algo, int16_t, int16)                            \
  SHCOLL_ALLTOALL_DEFINITION(_algo, int32_t, int32)                            \
  SHCOLL_ALLTOALL_DEFINITION(_algo, int64_t, int64)                            \
  SHCOLL_ALLTOALL_DEFINITION(_algo, uint8_t, uint8)                            \
  SHCOLL_ALLTOALL_DEFINITION(_algo, uint16_t, uint16)                          \
  SHCOLL_ALLTOALL_DEFINITION(_algo, uint32_t, uint32)                          \
  SHCOLL_ALLTOALL_DEFINITION(_algo, uint64_t, uint64)                          \
  SHCOLL_ALLTOALL_DEFINITION(_algo, size_t, size)                              \
  SHCOLL_ALLTOALL_DEFINITION(_algo, ptrdiff_t, ptrdiff)

/* Define for all algorithms */
DEFINE_SHCOLL_ALLTOALL_TYPES(shift_exchange_barrier)
DEFINE_SHCOLL_ALLTOALL_TYPES(shift_exchange_counter)
DEFINE_SHCOLL_ALLTOALL_TYPES(shift_exchange_signal)
DEFINE_SHCOLL_ALLTOALL_TYPES(xor_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALL_TYPES(xor_pairwise_exchange_counter)
DEFINE_SHCOLL_ALLTOALL_TYPES(xor_pairwise_exchange_signal)
DEFINE_SHCOLL_ALLTOALL_TYPES(color_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALL_TYPES(color_pairwise_exchange_counter)
DEFINE_SHCOLL_ALLTOALL_TYPES(color_pairwise_exchange_signal)

// @formatter:on
