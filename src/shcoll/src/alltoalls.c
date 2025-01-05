/**
 * @file alltoalls.c
 * @brief Implementation of strided all-to-all collective operations
 * @details
 * This file implements various algorithms for strided all-to-all collective
 * operations:
 * - Shift exchange
 * - XOR pairwise exchange
 * - Color pairwise exchange
 *
 * Each algorithm has variants using different synchronization methods:
 * - Barrier-based
 * - Signal-based
 * - Counter-based
 *
 * @author Probably OpenSHMEM team
 * @copyright For license: see LICENSE file at top-level
 */

#include "shcoll.h"
#include "shcoll/compat.h"
#include <string.h>
#include <limits.h>
#include <assert.h>

/**
 * @brief Calculate edge color for color pairwise exchange algorithm
 *
 * @param i Current round number
 * @param me Current PE number
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
static int alltoalls_rounds_sync = INT32_MAX;

/**
 * @brief Set number of rounds between synchronizations
 */
void shcoll_set_alltoalls_rounds_sync(int rounds_sync) {
  alltoalls_rounds_sync = rounds_sync;
}

/**
 * @brief Helper macro to define barrier-based alltoalls implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to determine peer PE
 * @param _cond Additional condition for participation
 */
#define ALLTOALLS_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)               \
  inline static void alltoalls_helper_##_algo##_barrier(                       \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
      long *pSync) {                                                           \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    /* Get my index in the active set */                                       \
    const int me_as = (me - PE_start) / stride;                                \
                                                                               \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * dst;                    \
    void const *source_ptr = ((uint8_t *)source) + me_as * sst;                \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    assert(_cond);                                                             \
                                                                               \
    memcpy(dest_ptr, source_ptr, nelems);                                      \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      source_ptr = ((uint8_t *)source) + peer_as * sst;                        \
                                                                               \
      shmem_putmem_nbi(dest_ptr, source_ptr, nelems,                           \
                       PE_start + peer_as * stride);                           \
                                                                               \
      if (i % alltoalls_rounds_sync == 0) {                                    \
        shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);  \
      }                                                                        \
    }                                                                          \
                                                                               \
    shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);      \
  }

/**
 * @brief Helper macro to define counter-based alltoalls implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to determine peer PE
 * @param _cond Additional condition for participation
 */
#define ALLTOALLS_HELPER_COUNTER_DEFINITION(_algo, _peer, _cond)               \
  inline static void alltoalls_helper_##_algo##_counter(                       \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
      long *pSync) {                                                           \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    /* Get my index in the active set */                                       \
    const int me_as = (me - PE_start) / stride;                                \
                                                                               \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * dst;                    \
    void const *source_ptr;                                                    \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    assert(_cond);                                                             \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      source_ptr = ((uint8_t *)source) + peer_as * sst;                        \
                                                                               \
      shmem_putmem_nbi(dest_ptr, source_ptr, nelems,                           \
                       PE_start + peer_as * stride);                           \
    }                                                                          \
                                                                               \
    source_ptr = ((uint8_t *)source) + me_as * sst;                            \
    memcpy(dest_ptr, source_ptr, nelems);                                      \
                                                                               \
    shmem_fence();                                                             \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      shmem_long_atomic_inc(pSync, PE_start + peer_as * stride);               \
    }                                                                          \
                                                                               \
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ,                                 \
                          SHCOLL_SYNC_VALUE + PE_size - 1);                    \
    shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);                                \
  }

/**
 * @brief Helper macro to define signal-based alltoalls implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to determine peer PE
 * @param _cond Additional condition for participation
 */
#define ALLTOALLS_HELPER_SIGNAL_DEFINITION(_algo, _peer, _cond)                \
  inline static void alltoalls_helper_##_algo##_signal(                        \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
      long *pSync) {                                                           \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    /* Get my index in the active set */                                       \
    const int me_as = (me - PE_start) / stride;                                \
                                                                               \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * dst;                    \
    void const *source_ptr;                                                    \
                                                                               \
    assert(_cond);                                                             \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      source_ptr = ((uint8_t *)source) + peer_as * sst;                        \
                                                                               \
      shmem_putmem_signal_nb(dest_ptr, source_ptr, nelems, pSync + i - 1,      \
                             SHCOLL_SYNC_VALUE + 1,                            \
                             PE_start + peer_as * stride, NULL);               \
    }                                                                          \
                                                                               \
    source_ptr = ((uint8_t *)source) + me_as * sst;                            \
    memcpy(dest_ptr, source_ptr, nelems);                                      \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      shmem_long_wait_until(pSync + i - 1, SHMEM_CMP_GT, SHCOLL_SYNC_VALUE);   \
      shmem_long_p(pSync + i - 1, SHCOLL_SYNC_VALUE, me);                      \
    }                                                                          \
  }

/** @brief Peer calculation for shift exchange algorithm */
#define SHIFT_PEER(I, ME, NPES) (((ME) + (I)) % (NPES))
ALLTOALLS_HELPER_BARRIER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_HELPER_COUNTER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(shift_exchange, SHIFT_PEER,
                                   PE_size - 1 <= SHCOLL_ALLTOALLS_SYNC_SIZE)

/** @brief Peer calculation for XOR exchange algorithm */
#define XOR_PEER(I, ME, NPES) ((I) ^ (ME))
#define XOR_COND (((PE_size - 1) & PE_size) == 0)

ALLTOALLS_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALLS_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                   XOR_COND &&PE_size - 1 <=
                                       SHCOLL_ALLTOALLS_SYNC_SIZE)

/** @brief Peer calculation for color exchange algorithm */
#define COLOR_PEER(I, ME, NPES) edge_color(I, ME, NPES)
#define COLOR_COND ((me_as % 2) == 0 || (me_as + 1) < PE_size)

ALLTOALLS_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                    COLOR_COND)
ALLTOALLS_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                    COLOR_COND)
ALLTOALLS_HELPER_SIGNAL_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                   COLOR_COND &&PE_size - 1 <=
                                       SHCOLL_ALLTOALLS_SYNC_SIZE)

/**
 * @brief Helper macro to define sized alltoalls implementations
 *
 * @param _algo Algorithm name
 * @param _size Size in bits
 */
#define SHCOLL_ALLTOALLS_SIZE_DEFINITION(_algo, _size)                         \
  void shcoll_alltoalls##_size##_##_algo(                                      \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
      long *pSync) {                                                           \
    alltoalls_helper_##_algo(dest, source, dst, sst,                           \
                             (_size) / (CHAR_BIT) * nelems, PE_start,          \
                             logPE_stride, PE_size, pSync);                    \
  }

// @formatter:off

SHCOLL_ALLTOALLS_SIZE_DEFINITION(shift_exchange_barrier, 32)
SHCOLL_ALLTOALLS_SIZE_DEFINITION(shift_exchange_barrier, 64)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(shift_exchange_counter, 32)
SHCOLL_ALLTOALLS_SIZE_DEFINITION(shift_exchange_counter, 64)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(shift_exchange_signal, 32)
SHCOLL_ALLTOALLS_SIZE_DEFINITION(shift_exchange_signal, 64)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(xor_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALLS_SIZE_DEFINITION(xor_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(xor_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALLS_SIZE_DEFINITION(xor_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(xor_pairwise_exchange_signal, 32)
SHCOLL_ALLTOALLS_SIZE_DEFINITION(xor_pairwise_exchange_signal, 64)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(color_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALLS_SIZE_DEFINITION(color_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(color_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALLS_SIZE_DEFINITION(color_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(color_pairwise_exchange_signal, 32)
SHCOLL_ALLTOALLS_SIZE_DEFINITION(color_pairwise_exchange_signal, 64)

// @formatter:on

/**
 * @brief Helper macro to define typed alltoalls implementations
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 FIXME: how should we set logPE_stride?
 */
#define SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, _type, _typename)              \
  int shcoll_##_typename##_alltoalls_##_algo(                                  \
      shmem_team_t team, _type *dest, const _type *source, ptrdiff_t dst,      \
      ptrdiff_t sst, size_t nelems) {                                          \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    /* Allocate pSync from symmetric heap */                                   \
    long *pSync = shmem_malloc(SHCOLL_ALLTOALLS_SYNC_SIZE * sizeof(long));     \
    if (!pSync)                                                                \
      return -1;                                                               \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_ALLTOALLS_SYNC_SIZE; i++) {                     \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    /* Ensure all PEs have initialized pSync */                                \
    shmem_team_sync(team);                                                     \
    /* Perform alltoalls */                                                    \
    alltoalls_helper_##_algo(dest, source, dst * sizeof(_type),                \
                             sst * sizeof(_type), nelems * sizeof(_type),      \
                             PE_start, logPE_stride, PE_size, pSync);          \
    /* Cleanup */                                                              \
    shmem_team_sync(team);                                                     \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

/**
 * @brief Helper macro to define alltoalls implementations for all types
 *
 * @param _algo Algorithm name to generate implementations for
 */
#define DEFINE_SHCOLL_ALLTOALLS_TYPES(_algo)                                   \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, float, float)                        \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, double, double)                      \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, long double, longdouble)             \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, char, char)                          \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, signed char, schar)                  \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, short, short)                        \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, int, int)                            \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, long, long)                          \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, long long, longlong)                 \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, unsigned char, uchar)                \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, unsigned short, ushort)              \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, unsigned int, uint)                  \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, unsigned long, ulong)                \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, unsigned long long, ulonglong)       \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, int8_t, int8)                        \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, int16_t, int16)                      \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, int32_t, int32)                      \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, int64_t, int64)                      \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, uint8_t, uint8)                      \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, uint16_t, uint16)                    \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, uint32_t, uint32)                    \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, uint64_t, uint64)                    \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, size_t, size)                        \
  SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, ptrdiff_t, ptrdiff)

// @formatter:off

DEFINE_SHCOLL_ALLTOALLS_TYPES(shift_exchange_barrier)
DEFINE_SHCOLL_ALLTOALLS_TYPES(shift_exchange_counter)
DEFINE_SHCOLL_ALLTOALLS_TYPES(shift_exchange_signal)

DEFINE_SHCOLL_ALLTOALLS_TYPES(xor_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALLS_TYPES(xor_pairwise_exchange_counter)
DEFINE_SHCOLL_ALLTOALLS_TYPES(xor_pairwise_exchange_signal)

DEFINE_SHCOLL_ALLTOALLS_TYPES(color_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALLS_TYPES(color_pairwise_exchange_counter)
DEFINE_SHCOLL_ALLTOALLS_TYPES(color_pairwise_exchange_signal)

// @formatter:on

/**
 * @brief Helper macro to define alltoalls implementations for all types
 *
 * @param _algo Algorithm name to generate implementations for
 TODO: implement alltoallsmem
 */
#define SHCOLL_ALLTOALLSMEM_DEFINITION(_algo)                                 \
  int shcoll_alltoallsmem_##_algo(shmem_team_t team, void *dest,               \
                                  const void *source, ptrdiff_t dst,           \
                                  ptrdiff_t sst, size_t nelems) {              \
    return 0;                                                                  \
  }

SHCOLL_ALLTOALLSMEM_DEFINITION(shift_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(shift_exchange_counter)
SHCOLL_ALLTOALLSMEM_DEFINITION(shift_exchange_signal)
SHCOLL_ALLTOALLSMEM_DEFINITION(xor_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(xor_pairwise_exchange_counter)
SHCOLL_ALLTOALLSMEM_DEFINITION(xor_pairwise_exchange_signal)
SHCOLL_ALLTOALLSMEM_DEFINITION(color_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(color_pairwise_exchange_counter)
SHCOLL_ALLTOALLSMEM_DEFINITION(color_pairwise_exchange_signal)
