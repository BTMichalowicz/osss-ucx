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

// @formatter:on

/**
 * @brief Helper macro to define type-generic barrier-based alltoalls
 *
 * @param _algo Algorithm name
 * @param _peer Peer calculation macro
 * @param _cond Algorithm condition
 */
#define ALLTOALLS_TYPE_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)          \
  inline static int alltoalls_type_helper_##_algo##_barrier(                   \
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

/**
 * @brief Helper macro to define type-generic counter-based alltoalls
 *
 * @param _algo Algorithm name
 * @param _peer Peer calculation macro
 * @param _cond Algorithm condition
 */
#define ALLTOALLS_TYPE_HELPER_COUNTER_DEFINITION(_algo, _peer, _cond)          \
  inline static int alltoalls_type_helper_##_algo##_counter(                   \
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

ALLTOALLS_TYPE_HELPER_BARRIER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_TYPE_HELPER_COUNTER_DEFINITION(shift_exchange, SHIFT_PEER, 1)

ALLTOALLS_TYPE_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                         XOR_COND)
ALLTOALLS_TYPE_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                         XOR_COND)

ALLTOALLS_TYPE_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                         COLOR_COND)
ALLTOALLS_TYPE_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                         COLOR_COND)

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
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    int ret = alltoalls_type_helper_##_algo(                                   \
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
 * @brief Helper macro to define memory-based barrier alltoalls implementations
 *
 * This macro defines a helper function that implements the barrier-based
 * alltoalls operation for raw memory blocks. It handles the data movement and
 * synchronization using barriers.
 *
 * @param _algo Algorithm name to use in the function name
 * @param _peer Macro/function to calculate peer PE for exchanges
 * @param _cond Additional condition that must be met for PE participation
 */
#define ALLTOALLS_MEM_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)           \
  inline static int alltoalls_mem_helper_##_algo##_barrier(                    \
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

// #define ALLTOALLS_MEM_HELPER_BARRIER_DEFINITION(_algo, _peer, _cond)           \
//   void alltoalls_mem_helper_##_algo##_barrier(                                 \
//       void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
//       size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
//       long *pSync) {                                                           \
//     const int stride = 1 << logPE_stride;                                      \
//     const int me = shmem_my_pe();                                              \
//     const int me_as = (me - PE_start) / stride;                                \
//                                                                                \
//     /* Copy local data */                                                      \
//     memcpy(dest, source, nelems);                                              \
//                                                                                \
//     /* Exchange data with other PEs */                                         \
//     for (int i = 1; i < PE_size; i++) {                                        \
//       int peer_as = _peer(i, me_as, PE_size);                                  \
//       int peer = PE_start + peer_as * stride;                                  \
//                                                                                \
//       shmem_putmem_nbi(dest, (uint8_t *)source + peer_as * nelems,            \
//                        nelems, peer);                                          \
//     }                                                                          \
//                                                                                \
//     shmem_fence();                                                             \
//     shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);      \
//   }

/**
 * @brief Helper macro to define memory-based counter alltoalls implementations
 *
 * This macro defines a helper function that implements the counter-based
 * alltoalls operation for raw memory blocks. It handles the data movement and
 * synchronization using atomic counters instead of barriers.
 *
 * The counter-based approach allows for more asynchronous communication
 * compared to barriers, as PEs only need to wait for their specific exchanges
 * to complete rather than global synchronization.
 *
 * @param _algo Algorithm name to use in the function name
 * @param _peer Macro/function to calculate peer PE for exchanges
 * @param _cond Additional condition that must be met for PE participation

 FIXME: needs to return 0 if succesfful, -1 otherwise

 FIXME: not tested, probably doesn't work
 */
#define ALLTOALLS_MEM_HELPER_COUNTER_DEFINITION(_algo, _peer, _cond)           \
  inline static int alltoalls_mem_helper_##_algo##_counter(                    \
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
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    static long pSync[SHCOLL_ALLTOALLS_SYNC_SIZE]; /* Add pSync array */       \
                                                                               \
    for (int i = 0; i < SHCOLL_ALLTOALLS_SYNC_SIZE; i++) {                       \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
                                                                               \
    int ret = alltoalls_mem_helper_##_algo(dest, source, dst, sst, nelems,     \
                                           PE_start, logPE_stride, PE_size);   \
                                                                               \
    if (ret != 0) {                                                            \
      return -1;                                                               \
    }                                                                          \
    return 0;                                                                  \
  }

// #define SHCOLL_ALLTOALLSMEM_DEFINITION(_algo)                                  \
//   int shcoll_alltoallsmem_##_algo(shmem_team_t team, void *dest,               \
//                                   const void *source, ptrdiff_t dst,           \
//                                   ptrdiff_t sst, size_t nelems) {              \
//     long *pSync = shmem_malloc(SHCOLL_ALLTOALLS_SYNC_SIZE * sizeof(long));     \
//     if (!pSync)                                                                \
//       return -1;                                                               \
//                                                                                \
//     for (int i = 0; i < SHCOLL_ALLTOALLS_SYNC_SIZE; i++) {                     \
//       pSync[i] = SHCOLL_SYNC_VALUE;                                            \
//     }                                                                          \
//                                                                                \
//     shmem_barrier_all();                                                       \
//                                                                                \
//     const int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);   \
//     const int logPE_stride = 0;                                                \
//     const int PE_size = shmem_team_n_pes(team);                                \
//                                                                                \
//     alltoalls_mem_helper_##_algo(dest, source, dst, sst, nelems, PE_start,     \
//                                  logPE_stride, PE_size, pSync);                \
//                                                                                \
//     shmem_barrier_all();                                                       \
//     shmem_free(pSync);                                                         \
//     return 0;                                                                  \
//   }

/* Define helpers for each algorithm */
ALLTOALLS_MEM_HELPER_BARRIER_DEFINITION(shift_exchange, SHIFT_PEER, 1)
ALLTOALLS_MEM_HELPER_COUNTER_DEFINITION(shift_exchange, SHIFT_PEER, 1)

ALLTOALLS_MEM_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                        XOR_COND)
ALLTOALLS_MEM_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, XOR_PEER,
                                        XOR_COND)

ALLTOALLS_MEM_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                        COLOR_COND)
ALLTOALLS_MEM_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, COLOR_PEER,
                                        COLOR_COND)

/* Define the actual functions */
SHCOLL_ALLTOALLSMEM_DEFINITION(shift_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(shift_exchange_counter)
SHCOLL_ALLTOALLSMEM_DEFINITION(xor_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(xor_pairwise_exchange_counter)
SHCOLL_ALLTOALLSMEM_DEFINITION(color_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(color_pairwise_exchange_counter)
