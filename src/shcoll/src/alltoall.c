/**
 * @file alltoall.c
 * @brief Implementation of all-to-all collective operations
 *
 * This file implements various algorithms for all-to-all collective operations:
 * - Shift exchange
 * - XOR pairwise exchange
 * - Color pairwise exchange
 *
 * Each algorithm has variants using different synchronization methods:
 * - Barrier-based
 * - Signal-based
 * - Counter-based
 *
 * @copyright For license: see LICENSE file at top-level
 */

#include "shcoll.h"
#include "shcoll/compat.h"

#include <string.h>
#include <limits.h>
#include <assert.h>

#include <stdio.h>

/**
 * @brief Calculate edge color for color pairwise exchange algorithm
 *
 * @param i Current round number
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
 * @brief Set number of rounds between synchronizations for alltoall operations
 * @param rounds_sync Number of rounds between synchronizations
 */
void shcoll_set_alltoall_round_sync(int rounds_sync) {
  alltoall_rounds_sync = rounds_sync;
}

/**
 * @brief Helper macro to define barrier-based alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to calculate peer PE
 * @param _cond Condition that must be satisfied
 */
#define ALLTOALL_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)                \
  inline static void alltoall_helper_##_algo##_barrier(                        \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size, long *pSync) {                            \
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
    assert(_cond);                                                             \
                                                                               \
    memcpy(dest_ptr, source_ptr, nelems);                                      \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      source_ptr = ((uint8_t *)source) + peer_as * nelems;                     \
                                                                               \
      shmem_putmem_nbi(dest_ptr, source_ptr, nelems,                           \
                       PE_start + peer_as * stride);                           \
                                                                               \
      if (i % alltoall_rounds_sync == 0) {                                     \
        /* TODO: change to auto shcoll barrier */                              \
        shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);  \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* TODO: change to auto shcoll barrier */                                  \
    shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);      \
  }

/**
 * @brief Helper macro to define counter-based alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to calculate peer PE
 * @param _cond Condition that must be satisfied
 */
#define ALLTOALL_HELPER_COUNTER_DEFINITION(_algo, _peer, _cond)                \
  inline static void alltoall_helper_##_algo##_counter(                        \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size, long *pSync) {                            \
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
    assert(_cond);                                                             \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      source_ptr = ((uint8_t *)source) + peer_as * nelems;                     \
                                                                               \
      shmem_putmem_nbi(dest_ptr, source_ptr, nelems,                           \
                       PE_start + peer_as * stride);                           \
    }                                                                          \
                                                                               \
    source_ptr = ((uint8_t *)source) + me_as * nelems;                         \
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
 * @brief Helper macro to define signal-based alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _peer Function to calculate peer PE
 * @param _cond Condition that must be satisfied
 */
#define ALLTOALL_HELPER_SIGNAL_DEFINITION(_algo, _peer, _cond)                 \
  inline static void alltoall_helper_##_algo##_signal(                         \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size, long *pSync) {                            \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    /* Get my index in the active set */                                       \
    const int me_as = (me - PE_start) / stride;                                \
                                                                               \
    void *const dest_ptr = ((uint8_t *)dest) + me_as * nelems;                 \
    void const *source_ptr;                                                    \
                                                                               \
    assert(_cond);                                                             \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      source_ptr = ((uint8_t *)source) + peer_as * nelems;                     \
                                                                               \
      shmem_putmem_signal_nb(dest_ptr, source_ptr, nelems, pSync + i - 1,      \
                             SHCOLL_SYNC_VALUE + 1,                            \
                             PE_start + peer_as * stride, NULL);               \
    }                                                                          \
                                                                               \
    source_ptr = ((uint8_t *)source) + me_as * nelems;                         \
    memcpy(dest_ptr, source_ptr, nelems);                                      \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      shmem_long_wait_until(pSync + i - 1, SHMEM_CMP_GT, SHCOLL_SYNC_VALUE);   \
      shmem_long_p(pSync + i - 1, SHCOLL_SYNC_VALUE, me);                      \
    }                                                                          \
  }

// @formatter:off

/** @brief Peer calculation for shift exchange algorithm */
#define SHIFT_PEER(I, ME, NPES) (((ME) + (I)) % (NPES))
ALLTOALL_HELPER_BARRIER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALL_HELPER_COUNTER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALL_HELPER_SIGNAL_DEFINITION(shift_exchange, SHIFT_PEER,
                                  PE_size - 1 <= SHCOLL_ALLTOALL_SYNC_SIZE)

/** @brief Peer calculation for XOR exchange algorithm */
#define XOR_PEER(I, ME, NPES) ((I) ^ (ME))
#define XOR_COND (((PE_size - 1) & PE_size) == 0)

ALLTOALL_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALL_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, XOR_PEER, XOR_COND)
ALLTOALL_HELPER_SIGNAL_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                  XOR_COND &&PE_size - 1 <=
                                      SHCOLL_ALLTOALL_SYNC_SIZE)

/** @brief Peer calculation for color exchange algorithm */
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
 * @brief Helper macro to define SIZE alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _size Size in bits
 */
#define SHCOLL_ALLTOALL_SIZE_DEFINITION(_algo, _size)                          \
  void shcoll_alltoall##_size##_##_algo(                                       \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size, long *pSync) {                            \
    /* Check initialization */                                                 \
    SHMEMU_CHECK_INIT();                                                       \
                                                                               \
    /* Check PE parameters */                                                  \
    SHMEMU_CHECK_POSITIVE(PE_size, "PE_size");                                 \
    SHMEMU_CHECK_NON_NEGATIVE(PE_start, "PE_start");                           \
    SHMEMU_CHECK_NON_NEGATIVE(logPE_stride, "logPE_stride");                   \
                                                                               \
    /* Check active set range */                                               \
    const int stride = 1 << logPE_stride;                                      \
    const int max_pe = PE_start + (PE_size - 1) * stride;                      \
    if (max_pe >= shmem_n_pes()) {                                             \
      shmemu_fatal("In %s(), active set PE range [%d, %d] exceeds number of PEs (%d)", \
                   __func__, PE_start, max_pe, shmem_n_pes());                 \
      return;                                                                  \
    }                                                                          \
                                                                               \
    /* Check buffer symmetry */                                                \
    SHMEMU_CHECK_SYMMETRIC(dest, (_size) / (CHAR_BIT) * nelems * PE_size);     \
    SHMEMU_CHECK_SYMMETRIC(source, (_size) / (CHAR_BIT) * nelems * PE_size);   \
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_ALLTOALL_SYNC_SIZE);   \
                                                                               \
    /* Check for overlap between source and destination */                     \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source,                                  \
                               (_size) / (CHAR_BIT) * nelems * PE_size,        \
                               (_size) / (CHAR_BIT) * nelems * PE_size);       \
                                                                               \
    alltoall_helper_##_algo(dest, source, (_size) / (CHAR_BIT) * nelems,       \
                            PE_start, logPE_stride, PE_size, pSync);           \
  }

// @formatter:off

SHCOLL_ALLTOALL_SIZE_DEFINITION(shift_exchange_barrier, 32)
SHCOLL_ALLTOALL_SIZE_DEFINITION(shift_exchange_barrier, 64)

SHCOLL_ALLTOALL_SIZE_DEFINITION(shift_exchange_counter, 32)
SHCOLL_ALLTOALL_SIZE_DEFINITION(shift_exchange_counter, 64)

SHCOLL_ALLTOALL_SIZE_DEFINITION(shift_exchange_signal, 32)
SHCOLL_ALLTOALL_SIZE_DEFINITION(shift_exchange_signal, 64)

SHCOLL_ALLTOALL_SIZE_DEFINITION(xor_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALL_SIZE_DEFINITION(xor_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALL_SIZE_DEFINITION(xor_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALL_SIZE_DEFINITION(xor_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALL_SIZE_DEFINITION(xor_pairwise_exchange_signal, 32)
SHCOLL_ALLTOALL_SIZE_DEFINITION(xor_pairwise_exchange_signal, 64)

SHCOLL_ALLTOALL_SIZE_DEFINITION(color_pairwise_exchange_counter, 32)
SHCOLL_ALLTOALL_SIZE_DEFINITION(color_pairwise_exchange_counter, 64)

SHCOLL_ALLTOALL_SIZE_DEFINITION(color_pairwise_exchange_barrier, 32)
SHCOLL_ALLTOALL_SIZE_DEFINITION(color_pairwise_exchange_barrier, 64)

SHCOLL_ALLTOALL_SIZE_DEFINITION(color_pairwise_exchange_signal, 32)
SHCOLL_ALLTOALL_SIZE_DEFINITION(color_pairwise_exchange_signal, 64)

// @formatter:on

/**
 * @brief Helper macro to define typed alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, _type, _typename)               \
  int shcoll_##_typename##_alltoall_##_algo(                                   \
      shmem_team_t team, _type *dest, const _type *source, size_t nelems) {    \
    /* Check initialization */                                                  \
    SHMEMU_CHECK_INIT();                                                        \
                                                                                \
    /* Check team validity */                                                   \
    SHMEMU_CHECK_TEAM_VALID(team);                                              \
                                                                                \
    /* Get team parameters */                                                   \
    const int PE_size = shmem_team_n_pes(team);                                 \
                                                                                \
    /* Check buffer symmetry */                                                 \
    SHMEMU_CHECK_SYMMETRIC(dest, sizeof(_type) * nelems * PE_size);             \
    SHMEMU_CHECK_SYMMETRIC(source, sizeof(_type) * nelems * PE_size);           \
                                                                                \
    /* Check for overlap between source and destination */                      \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source,                                   \
                              sizeof(_type) * nelems * PE_size,                 \
                              sizeof(_type) * nelems * PE_size);                \
                                                                                \
    /* Get team parameters */                                                  \
    /* TODO: use shmem_translate PE to the team's PE 0  */                     \
    const int PE_start = 0;                                                    \
    const int logPE_stride = 0;                                                \
                                                                               \
    /* Allocate pSync from symmetric heap */                                   \
    long *pSync = shmem_malloc(SHCOLL_ALLTOALL_SYNC_SIZE * sizeof(long));      \
    if (!pSync) {                                                              \
      shmemu_fatal("In %s(), failed to allocate pSync array",                  \
                   __func__);                                                   \
      return -1;                                                                \
    }                                                                          \
                                                                               \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_ALLTOALL_SYNC_SIZE; i++) {                      \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
                                                                               \
    /* Ensure all PEs have initialized pSync */                                \
    shmem_team_sync(team);                                                     \
                                                                               \
    /* Perform alltoall */                                                     \
    alltoall_helper_##_algo(dest, source, nelems * sizeof(_type), PE_start,    \
                            logPE_stride, PE_size, pSync);                     \
                                                                               \
    /* Ensure alltoall completion */                                           \
    shmem_team_sync(team);                                                     \
                                                                               \
    /* Reset pSync before freeing */                                           \
    for (int i = 0; i < SHCOLL_ALLTOALL_SYNC_SIZE; i++) {                      \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    shmem_team_sync(team);                                                     \
                                                                               \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

/**
 * @brief Helper macro to define alltoall implementations for all types
 *
 * @param _algo Algorithm name
 */
#define DEFINE_SHCOLL_ALLTOALL_TYPES(_algo)                                    \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, float, float)                         \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, double, double)                       \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, long double, longdouble)              \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, char, char)                           \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, signed char, schar)                   \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, short, short)                         \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, int, int)                             \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, long, long)                           \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, long long, longlong)                  \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, unsigned char, uchar)                 \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, unsigned short, ushort)               \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, unsigned int, uint)                   \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, unsigned long, ulong)                 \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, unsigned long long, ulonglong)        \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, int8_t, int8)                         \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, int16_t, int16)                       \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, int32_t, int32)                       \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, int64_t, int64)                       \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, uint8_t, uint8)                       \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, uint16_t, uint16)                     \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, uint32_t, uint32)                     \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, uint64_t, uint64)                     \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, size_t, size)                         \
  SHCOLL_ALLTOALL_TYPE_DEFINITION(_algo, ptrdiff_t, ptrdiff)

// @formatter:off

DEFINE_SHCOLL_ALLTOALL_TYPES(shift_exchange_barrier)
DEFINE_SHCOLL_ALLTOALL_TYPES(shift_exchange_counter)
DEFINE_SHCOLL_ALLTOALL_TYPES(shift_exchange_signal)

DEFINE_SHCOLL_ALLTOALL_TYPES(xor_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALL_TYPES(xor_pairwise_exchange_counter)
DEFINE_SHCOLL_ALLTOALL_TYPES(xor_pairwise_exchange_signal)

DEFINE_SHCOLL_ALLTOALL_TYPES(color_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALL_TYPES(color_pairwise_exchange_counter)
DEFINE_SHCOLL_ALLTOALL_TYPES(color_pairwise_exchange_signal)

/**
 * @brief Helper macro to define alltoallmem implementations
 *
 * @param _algo Algorithm name
 */
#define SHCOLL_ALLTOALLMEM_DEFINITION(_algo)                                   \
  int shcoll_alltoallmem_##_algo(shmem_team_t team, void *dest,                \
                                 const void *source, size_t nelems) {          \
    /* Check initialization */                                                 \
    SHMEMU_CHECK_INIT();                                                       \
                                                                               \
    /* Check team validity */                                                  \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
                                                                               \
    /* Check for NULL pointers */                                              \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
                                                                               \
    /* Get team parameters */                                                  \
    const int PE_size = shmem_team_n_pes(team);                                \
                                                                               \
    /* Check buffer symmetry */                                                \
    SHMEMU_CHECK_SYMMETRIC(dest, nelems * PE_size);                            \
    SHMEMU_CHECK_SYMMETRIC(source, nelems * PE_size);                          \
                                                                               \
    /* Check for overlap between source and destination */                      \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source,                                  \
                               nelems * PE_size,                               \
                               nelems * PE_size);                              \
                                                                               \
    /* Get team parameters */                                                  \
    /* TODO: use shmem_translate PE to the team's PE 0  */                     \
    const int PE_start = 0; /* Teams use 0-based contiguous numbering */       \
    const int logPE_stride = 0;                                                \
                                                                               \
    /* Allocate pSync from symmetric heap */                                   \
    long *pSync = shmem_malloc(SHCOLL_ALLTOALL_SYNC_SIZE * sizeof(long));      \
    if (!pSync) {                                                              \
      shmemu_fatal("In %s(), failed to allocate pSync array",                  \
                   __func__);                                                  \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_ALLTOALL_SYNC_SIZE; i++) {                      \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
                                                                               \
    /* Ensure all PEs have initialized pSync */                                \
    shmem_team_sync(team);                                                     \
                                                                               \
    /* Perform alltoall */                                                     \
    alltoall_helper_##_algo(dest, source, nelems, PE_start, logPE_stride,      \
                            PE_size, pSync);                                   \
                                                                               \
    /* Ensure alltoall completion */                                           \
    shmem_team_sync(team);                                                     \
                                                                               \
    /* Reset pSync before freeing */                                           \
    for (int i = 0; i < SHCOLL_ALLTOALL_SYNC_SIZE; i++) {                      \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    shmem_team_sync(team);                                                     \
                                                                               \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

SHCOLL_ALLTOALLMEM_DEFINITION(shift_exchange_barrier)
SHCOLL_ALLTOALLMEM_DEFINITION(shift_exchange_counter)
SHCOLL_ALLTOALLMEM_DEFINITION(shift_exchange_signal)
SHCOLL_ALLTOALLMEM_DEFINITION(xor_pairwise_exchange_barrier)
SHCOLL_ALLTOALLMEM_DEFINITION(xor_pairwise_exchange_counter)
SHCOLL_ALLTOALLMEM_DEFINITION(xor_pairwise_exchange_signal)
SHCOLL_ALLTOALLMEM_DEFINITION(color_pairwise_exchange_barrier)
SHCOLL_ALLTOALLMEM_DEFINITION(color_pairwise_exchange_counter)
SHCOLL_ALLTOALLMEM_DEFINITION(color_pairwise_exchange_signal)

// @formatter:on
