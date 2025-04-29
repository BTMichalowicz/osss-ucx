/**
 * @file alltoalls.c
 * @brief Implementation of strided all-to-all collective operations
 * @author Srdan Milakovic
 * @date 5/21/18
 * @edited Michael Beebe (1/17/2025)
 *
 * This file implements various algorithms for the strided all-to-all collective
 * operation (alltoalls) in OpenSHMEM. The following algorithms are implemented:
 *
 * - Shift exchange: Each PE exchanges data with other PEs by shifting through
 * the PE list
 * - XOR pairwise exchange: PEs exchange data using XOR pattern for
 * communication pairs
 * - Color pairwise exchange: PEs exchange data using edge coloring algorithm
 *
 * Each algorithm has two synchronization variants:
 * - Barrier-based: Uses barriers for synchronization
 * - Counter-based: Uses atomic counters for synchronization
 *
 * The implementations support:
 * - Different data sizes (32-bit and 64-bit)
 * - Different data types (integer, floating point, etc)
 * - Non-blocking variants (_nbi suffix)
 * - Team-based collectives
 */

#include "shcoll.h"
#include "shcoll/compat.h"

#include "shmem.h"

#include <limits.h>
#include <assert.h>
#include <string.h>

#include <stdio.h>

/**
 * @brief Calculate edge color for color-based exchange algorithm
 *
 * @param i Current iteration index
 * @param me Current PE number
 * @param npes Total number of PEs
 * @return Edge color value determining communication partner
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

/** @brief Calculate peer PE using shift pattern */
#define SHIFT_PEER(I, ME, NPES) (((ME) + (I)) % (NPES))

/** @brief Calculate peer PE using XOR pattern */
#define XOR_PEER(I, ME, NPES) ((I) ^ (ME))

/** @brief Condition for XOR pattern (power of 2 PEs) */
#define XOR_COND (((PE_size - 1) & PE_size) == 0)

/** @brief Calculate peer PE using color pattern */
#define COLOR_PEER(I, ME, NPES) edge_color(I, ME, NPES)

/** @brief Condition for color pattern (even PE or PE+1 < total PEs) */
#define COLOR_COND ((me_as % 2) == 0 || (me_as + 1) < PE_size)

/**
 * @brief Helper macro to define barrier-based alltoalls for different sizes
 *
 * @param _algo Algorithm name (shift_exchange, xor_pairwise_exchange, etc)
 * @param _size Data size in bits (32 or 64)
 * @param _peer Peer calculation macro (SHIFT_PEER, XOR_PEER, COLOR_PEER)
 * @param _cond Condition for algorithm applicability
 * @param _nbi Optional non-blocking suffix
 */
#define ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION(_algo, _size, _peer, _cond,   \
                                                 _nbi)                         \
  void shcoll_alltoalls##_size##_##_algo##_barrier##_nbi(                      \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
      long *pSync) {                                                           \
    /* Sanity Checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_POSITIVE(PE_size, "PE_size");                                 \
    SHMEMU_CHECK_NON_NEGATIVE(PE_start, "PE_start");                           \
    SHMEMU_CHECK_NON_NEGATIVE(logPE_stride, "logPE_stride");                   \
    SHMEMU_CHECK_ACTIVE_SET_RANGE(PE_start, logPE_stride, PE_size);            \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
    SHMEMU_CHECK_NULL(pSync, "pSync");                                         \
    const size_t element_size_bytes = (_size) / CHAR_BIT;                      \
    const size_t total_extent_bytes = element_size_bytes * nelems * PE_size;   \
    SHMEMU_CHECK_SYMMETRIC(dest, total_extent_bytes);                          \
    SHMEMU_CHECK_SYMMETRIC(source, total_extent_bytes);                        \
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_ALLTOALL_SYNC_SIZE);   \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, total_extent_bytes,              \
                                total_extent_bytes);                           \
                                                                               \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    /* Get my index in the active set */                                       \
    /* const int me_as = (me - PE_start) / stride; */                          \
                                                                               \
    void *const dest_ptr =                                                     \
        ((uint8_t *)dest) + me * dst * nelems * ((_size) / CHAR_BIT);          \
    void const *source_ptr =                                                   \
        ((uint8_t *)source) + me * sst * nelems * ((_size) / CHAR_BIT);        \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    assert(_cond);                                                             \
                                                                               \
    for (i = 0; i < nelems; i++) {                                             \
      *(((uint##_size##_t *)dest_ptr) + i * dst) =                             \
          *(((uint##_size##_t *)source_ptr) + i * sst);                        \
    }                                                                          \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me, PE_size);                                         \
      source_ptr =                                                             \
          ((uint8_t *)source) + peer_as * sst * nelems * ((_size) / CHAR_BIT); \
                                                                               \
      shmem_iput##_size##_nbi(dest_ptr, source_ptr, dst, sst, nelems,          \
                              PE_start + peer_as * stride);                    \
    }                                                                          \
                                                                               \
    /* TODO: change to auto shcoll barrier */                                  \
    shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);      \
  }

/**
 * @brief Helper macro to define counter-based alltoalls for different sizes
 *
 * @param _algo Algorithm name (shift_exchange, xor_pairwise_exchange, etc)
 * @param _size Data size in bits (32 or 64)
 * @param _peer Peer calculation macro (SHIFT_PEER, XOR_PEER, COLOR_PEER)
 * @param _cond Condition for algorithm applicability
 * @param _nbi Optional non-blocking suffix
 */
#define ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION(_algo, _size, _peer, _cond,   \
                                                 _nbi)                         \
  void shcoll_alltoalls##_size##_##_algo##_counter##_nbi(                      \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
      long *pSync) {                                                           \
    /* Sanity Checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_POSITIVE(PE_size, "PE_size");                                 \
    SHMEMU_CHECK_NON_NEGATIVE(PE_start, "PE_start");                           \
    SHMEMU_CHECK_NON_NEGATIVE(logPE_stride, "logPE_stride");                   \
    SHMEMU_CHECK_ACTIVE_SET_RANGE(PE_start, logPE_stride, PE_size);            \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
    SHMEMU_CHECK_NULL(pSync, "pSync");                                         \
    const size_t element_size_bytes = (_size) / CHAR_BIT;                      \
    const size_t total_extent_bytes = element_size_bytes * nelems * PE_size;   \
    SHMEMU_CHECK_SYMMETRIC(dest, total_extent_bytes);                          \
    SHMEMU_CHECK_SYMMETRIC(source, total_extent_bytes);                        \
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_ALLTOALL_SYNC_SIZE);   \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, total_extent_bytes,              \
                                total_extent_bytes);                           \
                                                                               \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    /* Get my index in the active set */                                       \
    const int me_as = (me - PE_start) / stride;                                \
                                                                               \
    void *const dest_ptr =                                                     \
        ((uint8_t *)dest) + me * dst * nelems * ((_size) / CHAR_BIT);          \
    void const *source_ptr =                                                   \
        ((uint8_t *)source) + me * sst * nelems * ((_size) / CHAR_BIT);        \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    assert(_cond);                                                             \
                                                                               \
    for (i = 0; i < nelems; i++) {                                             \
      *(((uint##_size##_t *)dest_ptr) + i * dst) =                             \
          *(((uint##_size##_t *)source_ptr) + i * sst);                        \
    }                                                                          \
                                                                               \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me, PE_size);                                         \
      source_ptr =                                                             \
          ((uint8_t *)source) + peer_as * sst * nelems * ((_size) / CHAR_BIT); \
                                                                               \
      shmem_iput##_size##_nbi(dest_ptr, source_ptr, dst, sst, nelems,          \
                              PE_start + peer_as * stride);                    \
    }                                                                          \
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
 * @brief Helper macro to define alltoalls for 32-bit and 64-bit sizes
 *
 * @param _macro Base macro to use (barrier or counter variant)
 * @param _algo Algorithm name
 * @param _peer Peer calculation macro
 * @param _cond Algorithm condition
 * @param _nbi Optional non-blocking suffix
 */
#define SHCOLL_ALLTOALLS_SIZE_DEFINITION(_macro, _algo, _peer, _cond, _nbi)    \
  _macro(_algo, 32, _peer, _cond, _nbi) _macro(_algo, 64, _peer, _cond, _nbi)

// @formatter:off

/**
 * @brief Generate alltoalls implementations for shift exchange algorithm
 */
SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION,
                                 shift_exchange, SHIFT_PEER, 1, )
SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION,
                                 shift_exchange, SHIFT_PEER, 1, _nbi)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION,
                                 shift_exchange, SHIFT_PEER, 1, )
SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION,
                                 shift_exchange, SHIFT_PEER, 1, _nbi)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION,
                                 xor_pairwise_exchange, SHIFT_PEER, XOR_COND, )
SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION,
                                 xor_pairwise_exchange, SHIFT_PEER, XOR_COND,
                                 _nbi)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION,
                                 xor_pairwise_exchange, SHIFT_PEER, XOR_COND, )
SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION,
                                 xor_pairwise_exchange, SHIFT_PEER, XOR_COND,
                                 _nbi)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION,
                                 color_pairwise_exchange, SHIFT_PEER, 1, )
SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION,
                                 color_pairwise_exchange, SHIFT_PEER, 1, _nbi)

SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION,
                                 color_pairwise_exchange, SHIFT_PEER, 1, )
SHCOLL_ALLTOALLS_SIZE_DEFINITION(ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION,
                                 color_pairwise_exchange, SHIFT_PEER, 1, _nbi)

/**
 * @brief Helper macro to define barrier-based team alltoalls implementations
 *
 * This macro defines a helper function that implements the barrier-based
 * alltoalls operation for teams. It handles the data movement and
 * synchronization using barriers. The function copies data between team members
 * based on the specified algorithm.
 *
 * @param _algo Algorithm name to use in the function name
 * @param _peer Macro/function to calculate peer PE for exchanges
 * @param _cond Additional condition that must be met for PE participation
 */
#define ALLTOALLS_TEAM_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)          \
  inline static int alltoalls_team_helper_##_algo##_barrier(                   \
      void *dest, const void *source, ptrdiff_t dst_stride,                    \
      ptrdiff_t sst_stride, size_t nelems, int PE_start, int logPE_stride,     \
      int PE_size) {                                                           \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    int me_as = (me - PE_start) / stride;                                      \
    if (me_as < 0 || me_as >= PE_size || _cond == 0) {                         \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Allocate pSync from the symmetric heap */                               \
    long *pSync = shmem_malloc(SHCOLL_ALLTOALL_SYNC_SIZE * sizeof(long));      \
    if (pSync == NULL) {                                                       \
      return -1;                                                               \
    }                                                                          \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_ALLTOALL_SYNC_SIZE; i++) {                      \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
                                                                               \
    for (size_t i = 0; i < nelems; i++) {                                      \
      /* Loop over each peer in the team */                                    \
      for (int pe = 0; pe < PE_size; pe++) {                                   \
        int peer = PE_start + pe * stride;                                     \
        /* Compute the source pointer for element i from peer 'pe' */          \
        const char *src =                                                      \
            (const char *)source + pe * nelems * sst_stride + i * sst_stride;  \
        /* Compute the destination pointer for element i in this PE's block */ \
        char *dst =                                                            \
            (char *)dest + me_as * nelems * dst_stride + i * dst_stride;       \
        shmem_putmem_nbi(dst, src, dst_stride, peer);                          \
      }                                                                        \
    }                                                                          \
                                                                               \
    shmem_fence();                                                             \
    shmem_barrier_all();                                                       \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

/**
 * @brief Helper macro to define counter-based team alltoalls implementations
 *
 * This macro defines a helper function that implements the counter-based
 * alltoalls operation for teams. It handles the data movement and
 * synchronization using atomic counters instead of barriers. The function
 * copies data between team members based on the specified algorithm.
 *
 * @param _algo Algorithm name to use in the function name
 * @param _peer Macro/function to calculate peer PE for exchanges
 * @param _cond Additional condition that must be met for PE participation

 TODO: not yet tested and probably doesn't work
 */
#define ALLTOALLS_TEAM_HELPER_COUNTER_DEFINITION(_algo, _peer, _cond)          \
  inline static int alltoalls_team_helper_##_algo##_counter(                   \
      void *dest, const void *source, ptrdiff_t dst_stride,                    \
      ptrdiff_t sst_stride, size_t nelems, int PE_start, int logPE_stride,     \
      int PE_size) {                                                           \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    int me_as = (me - PE_start) / stride;                                      \
    if (me_as < 0 || me_as >= PE_size || _cond == 0) {                         \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    /* Copy local data: each team member's block is contiguous */              \
    memcpy((char *)dest + me_as * nelems * dst_stride,                         \
           (const char *)source + me_as * nelems * sst_stride,                 \
           nelems * dst_stride);                                               \
                                                                               \
    /* Use a static counter (shared among calls) for atomic synchronization */ \
    static long counter = 0;                                                   \
                                                                               \
    /* For each remote PE (i != me_as), exchange data element-by-element */    \
    for (int i = 0; i < PE_size; i++) {                                        \
      if (i == me_as)                                                          \
        continue;                                                              \
      int peer = PE_start + i * stride;                                        \
      for (size_t j = 0; j < nelems; j++) {                                    \
        const char *src =                                                      \
            (const char *)source + i * nelems * sst_stride + j * sst_stride;   \
        char *dst =                                                            \
            (char *)dest + me_as * nelems * dst_stride + j * dst_stride;       \
        shmem_putmem_nbi(dst, src, dst_stride, peer);                          \
      }                                                                        \
      shmem_fence();                                                           \
      shmem_long_atomic_inc(&counter, peer);                                   \
    }                                                                          \
                                                                               \
    /* Wait until the counter reaches (PE_size - 1) */                         \
    shmem_long_wait_until(&counter, SHMEM_CMP_EQ, PE_size - 1);                \
    counter = 0;                                                               \
                                                                               \
    return 0;                                                                  \
  }

/* Generate barrier-based helpers for each algorithm */
ALLTOALLS_TEAM_HELPER_BARRIER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_TEAM_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                         XOR_COND)
ALLTOALLS_TEAM_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                         COLOR_COND)

/* Generate counter-based helpers for each algorithm */
ALLTOALLS_TEAM_HELPER_COUNTER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_TEAM_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                         XOR_COND)
ALLTOALLS_TEAM_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                         COLOR_COND)

// @formatter:on

/**
 * @brief Helper macro to define type-specific alltoalls implementation
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, _type, _typename)              \
  int shcoll_##_typename##_alltoalls_##_algo(                                  \
      shmem_team_t team, _type *dest, const _type *source, ptrdiff_t dst,      \
      ptrdiff_t sst, size_t nelems) {                                          \
    /* Sanity Checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
                                                                               \
    const int PE_size = shmem_team_n_pes(team);                                \
    const size_t total_extent_bytes = sizeof(_type) * nelems * PE_size;        \
    SHMEMU_CHECK_SYMMETRIC(dest, total_extent_bytes);                          \
    SHMEMU_CHECK_SYMMETRIC(source, total_extent_bytes);                        \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, total_extent_bytes,              \
                                total_extent_bytes);                           \
                                                                               \
    /* Note: PE_start and logPE_stride are 0 for teams */                      \
    int PE_start = 0;                                                          \
    int logPE_stride = 0;                                                      \
    /* Convert element strides to byte strides and pass nelems as count */     \
    int ret = alltoalls_team_helper_##_algo(dest, source, dst * sizeof(_type), \
                                            sst * sizeof(_type), nelems,       \
                                            PE_start, logPE_stride, PE_size);  \
                                                                               \
    if (ret != 0) {                                                            \
      /* The helper function itself should have called shmemu_fatal on error   \
       */                                                                      \
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

DEFINE_SHCOLL_ALLTOALLS_TYPES(shift_exchange_barrier)
DEFINE_SHCOLL_ALLTOALLS_TYPES(shift_exchange_counter)
DEFINE_SHCOLL_ALLTOALLS_TYPES(xor_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALLS_TYPES(xor_pairwise_exchange_counter)
DEFINE_SHCOLL_ALLTOALLS_TYPES(color_pairwise_exchange_barrier)
DEFINE_SHCOLL_ALLTOALLS_TYPES(color_pairwise_exchange_counter)

/**
 * @brief Define generic memory alltoalls implementations
 *
 * @param _algo Algorithm name
 * @param _sync Synchronization type (barrier or counter)
 */
#define SHCOLL_ALLTOALLSMEM_DEFINITION(_algo)                                  \
  int shcoll_alltoallsmem_##_algo(shmem_team_t team, void *dest,               \
                                  const void *source, ptrdiff_t dst,           \
                                  ptrdiff_t sst, size_t nelems) {              \
    /* Sanity Checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
                                                                               \
    const int PE_size = shmem_team_n_pes(team);                                \
    /* Buffer Checks - nelems is already total bytes per PE for mem version */ \
    const size_t total_extent_bytes = nelems * PE_size;                        \
    SHMEMU_CHECK_SYMMETRIC(dest, total_extent_bytes);                          \
    SHMEMU_CHECK_SYMMETRIC(source, total_extent_bytes);                        \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, total_extent_bytes,              \
                                total_extent_bytes);                           \
                                                                               \
    /* Note: PE_start and logPE_stride are 0 for teams */                      \
    int PE_start = 0;                                                          \
    int logPE_stride = 0;                                                      \
    /* For memory version, dst and sst are already in bytes */                 \
    int ret = alltoalls_team_helper_##_algo(dest, source, dst, sst, nelems,    \
                                            PE_start, logPE_stride, PE_size);  \
                                                                               \
    if (ret != 0) {                                                            \
      /* The helper function itself should have called shmemu_fatal on error   \
       */                                                                      \
      return -1;                                                               \
    }                                                                          \
    return 0;                                                                  \
  }

/* Define the actual functions */
SHCOLL_ALLTOALLSMEM_DEFINITION(shift_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(shift_exchange_counter)

SHCOLL_ALLTOALLSMEM_DEFINITION(xor_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(xor_pairwise_exchange_counter)

SHCOLL_ALLTOALLSMEM_DEFINITION(color_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(color_pairwise_exchange_counter)
