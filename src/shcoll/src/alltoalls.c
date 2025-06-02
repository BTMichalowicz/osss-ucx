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
#include "shmemc.h"         /* For shmemc_team_h */
#include "shmemu.h"         /* For SHMEMU_CHECK_* macros */
#include "shcoll/barrier.h" /* For shcoll_barrier_binomial_tree */

#include <limits.h>
#include <assert.h>
#include <string.h>
#include <math.h> /* For log2 */

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
#define COLOR_COND (PE_size % 2 == 0) /* Use PE_size, not me_as */

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
      void *dest, const void *source, ptrdiff_t dst_stride,                    \
      ptrdiff_t sst_stride, size_t nelems, int PE_start, int logPE_stride,     \
      int PE_size, long *pSync) {                                              \
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
    const int me_as = (me - PE_start) / stride; /* My index in active set */   \
                                                                               \
    /* Cast to char* for byte arithmetic */                                    \
    char *dest_ptr = (char *)dest;                                             \
    const char *source_ptr = (const char *)source;                             \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    assert(_cond);                                                             \
                                                                               \
    /* 1. Local copy using strides */                                          \
    for (size_t k = 0; k < nelems; ++k) {                                      \
      memcpy(                                                                  \
          dest_ptr + (me_as * nelems + k) * dst_stride * element_size_bytes,   \
          source_ptr + (me_as * nelems + k) * sst_stride * element_size_bytes, \
          dst_stride * element_size_bytes);                                    \
    }                                                                          \
                                                                               \
    /* 2. Initiate non-blocking puts to other PEs */                           \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      int target_pe = PE_start + peer_as * stride;                             \
      /* Source for PE 'peer_as' contribution, dest is block 'me_as' */        \
      const char *current_source =                                             \
          source_ptr + (peer_as * nelems) * sst_stride * element_size_bytes;   \
      char *current_dest =                                                     \
          dest_ptr + (me_as * nelems) * dst_stride * element_size_bytes;       \
                                                                               \
      /* Use putmem for potentially non-contiguous strides */                  \
      for (size_t k = 0; k < nelems; ++k) {                                    \
        shmem_putmem_nbi(current_dest + k * dst_stride * element_size_bytes,   \
                         current_source + k * sst_stride * element_size_bytes, \
                         dst_stride * element_size_bytes, target_pe);          \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* 3. Barrier to ensure completion of puts */                              \
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
      void *dest, const void *source, ptrdiff_t dst_stride,                    \
      ptrdiff_t sst_stride, size_t nelems, int PE_start, int logPE_stride,     \
      int PE_size, long *pSync) {                                              \
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
    const int me_as = (me - PE_start) / stride; /* My index in active set */   \
    const size_t TSIZE = element_size_bytes;                                   \
                                                                               \
    /* Cast to char* for byte arithmetic */                                    \
    char *dest_ptr = (char *)dest;                                             \
    const char *source_ptr = (const char *)source;                             \
                                                                               \
    int i;                                                                     \
    int peer_as;                                                               \
                                                                               \
    assert(_cond);                                                             \
                                                                               \
    /* 1. Initiate non-blocking puts to other PEs */                           \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      int target_pe = PE_start + peer_as * stride;                             \
      const char *current_source =                                             \
          source_ptr + (peer_as * nelems) * sst_stride * TSIZE;                \
      char *current_dest = dest_ptr + (me_as * nelems) * dst_stride * TSIZE;   \
      /* Use putmem for potentially non-contiguous strides */                  \
      for (size_t k = 0; k < nelems; ++k) {                                    \
        shmem_putmem_nbi(current_dest + k * dst_stride * TSIZE,                \
                         current_source + k * sst_stride * TSIZE,              \
                         dst_stride * TSIZE, target_pe);                       \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* 2. Local copy using strides */                                          \
    for (size_t k = 0; k < nelems; ++k) {                                      \
      memcpy(dest_ptr + (me_as * nelems + k) * dst_stride * TSIZE,             \
             source_ptr + (me_as * nelems + k) * sst_stride * TSIZE,           \
             dst_stride * TSIZE);                                              \
    }                                                                          \
                                                                               \
    shmem_fence(); /* Ensure local copy is visible and puts initiated */       \
                                                                               \
    /* 3. Signal completion to peers using atomic increments on pSync[0] */    \
    /*    Each PE increments the counter on all *other* PEs */                 \
    for (i = 1; i < PE_size; i++) {                                            \
      peer_as = _peer(i, me_as, PE_size);                                      \
      shmem_long_atomic_inc(pSync, PE_start + peer_as * stride);               \
    }                                                                          \
                                                                               \
    /* 4. Wait for all PEs to signal completion */                             \
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ,                                 \
                          SHCOLL_SYNC_VALUE + PE_size - 1);                    \
    /* Reset own pSync value for future use */                                 \
    shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);                                \
  }

/**
 * @brief Helper macro to define alltoalls for 32-bit and 64-bit sizes
 *
 * @param _macro Base macro to use (barrier or counter variant)
 * @param _algo Algorithm name
 * @param _peer Peer calculation macro
 * @param _cond Condition for algorithm applicability
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

/* Generate 8-bit versions specifically for _mem functions */
ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION(shift_exchange, 8, SHIFT_PEER, 1, )
ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION(shift_exchange, 8, SHIFT_PEER, 1, )
ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, 8, SHIFT_PEER,
                                         XOR_COND, )
ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, 8, SHIFT_PEER,
                                         XOR_COND, )
ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, 8, SHIFT_PEER,
                                         1, )
ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, 8, SHIFT_PEER,
                                         1, )
ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION(shift_exchange, 8, SHIFT_PEER, 1, _nbi)
ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION(shift_exchange, 8, SHIFT_PEER, 1, _nbi)
ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION(xor_pairwise_exchange, 8, SHIFT_PEER,
                                         XOR_COND, _nbi)
ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION(xor_pairwise_exchange, 8, SHIFT_PEER,
                                         XOR_COND, _nbi)
ALLTOALLS_SIZE_HELPER_BARRIER_DEFINITION(color_pairwise_exchange, 8, SHIFT_PEER,
                                         1, _nbi)
ALLTOALLS_SIZE_HELPER_COUNTER_DEFINITION(color_pairwise_exchange, 8, SHIFT_PEER,
                                         1, _nbi)

/**
 * @brief Helper macro to define type-specific alltoalls implementation
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_ALLTOALLS_TYPE_DEFINITION(_algo, _type, _typename)               \
  int shcoll_##_typename##_alltoalls_##_algo(                                   \
      shmem_team_t team, _type *dest, const _type *source, ptrdiff_t dst,       \
      ptrdiff_t sst, size_t nelems) {                                           \
    /* Sanity Checks */                                                         \
    SHMEMU_CHECK_INIT();                                                        \
    SHMEMU_CHECK_TEAM_VALID(team);                                              \
    /* Need shmemc.h */                                                         \
    shmemc_team_h team_h = (shmemc_team_h)team; /* Cast to internal handle */   \
    SHMEMU_CHECK_NULL(dest, "dest");                                            \
    SHMEMU_CHECK_NULL(source, "source");                                        \
                                                                                \
    /* Get team parameters */                                                   \
    const int PE_size = team_h->nranks;                                         \
    const int PE_start = team_h->start;         /* Use stored start */          \
    const int stride = team_h->stride;          /* Use stored stride */         \
    SHMEMU_CHECK_TEAM_STRIDE(stride, __func__); /* Check stride if DEBUG */     \
    /* Calculate log2 stride */                                                 \
    int logPE_stride = (stride > 0) ? (int)log2((double)stride) : 0;            \
    const size_t element_size_bytes = sizeof(_type);                            \
    const size_t total_extent_bytes = element_size_bytes * nelems * PE_size;    \
    SHMEMU_CHECK_SYMMETRIC(dest, total_extent_bytes);                           \
    SHMEMU_CHECK_SYMMETRIC(source, total_extent_bytes);                         \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, total_extent_bytes,               \
                                total_extent_bytes);                            \
                                                                                \
    /* Allocate pSync from symmetric heap */                                    \
    long *pSync = shmemc_team_get_psync(team_h, SHMEMC_PSYNC_ALLTOALL);         \
    SHMEMU_CHECK_NULL(pSync, "team_h->pSyncs[ALLTOALL]");                       \
                                                                                \
    /* Call the appropriate size-specific helper */ /* Determine size and call  \
                                                     * the correct              \
                                                     * shcoll_alltoalls<size>_* \
                                                     * function                 \
                                                     */                         \
    if (element_size_bytes == 8) {                                              \
      shcoll_alltoalls64_##_algo(dest, source, dst, sst, nelems, PE_start,      \
                                 logPE_stride, PE_size, pSync);                 \
    } else if (element_size_bytes == 4) {                                       \
      shcoll_alltoalls32_##_algo(dest, source, dst, sst, nelems, PE_start,      \
                                 logPE_stride, PE_size, pSync);                 \
    } else {                                                                    \
      /* Fallback: Byte-wise operation for other sizes                          \
      (e.g., long double) */ /* Needs careful implementation
      based on _algo for sync */                    \
      const char *src_bytes = (const char *)source;                             \
      char *dest_bytes = (char *)dest;                                          \
      const ptrdiff_t sst_bytes = sst * element_size_bytes;                     \
      const ptrdiff_t dst_bytes =                                               \
          dst * element_size_bytes; /* const int me_as = (shmem_my_pe() -       \
                                       PE_start) / stride; */                   \
      const int me_as = shmem_team_my_pe(team);                                 \
                                                                                \
      /* 1. Local Copy (byte by byte respecting strides) */                     \
      for (size_t k = 0; k < nelems; ++k) {                                     \
        memcpy(dest_bytes + (me_as * nelems + k) * dst_bytes,                   \
               src_bytes + (me_as * nelems + k) * sst_bytes,                    \
               element_size_bytes);                                             \
      }                                                                         \
                                                                                \
      /* 2. Remote Puts (byte by byte respecting strides) */                    \
      /*    This assumes barrier sync. Counter needs different logic. */        \
      /* TODO: Differentiate barrier vs counter based on _algo name */          \
      for (int i = 1; i < PE_size; ++i) {                                       \
        int peer_as = (me_as + i) % PE_size; /* Example: shift peer */          \
        int target_pe = PE_start + peer_as * stride;                            \
        const char *current_source =                                            \
            src_bytes + (peer_as * nelems) * sst_bytes;                         \
        char *current_dest = dest_bytes + (me_as * nelems) * dst_bytes;         \
        for (size_t k = 0; k < nelems; ++k) {                                   \
          shmem_putmem_nbi(current_dest + k * dst_bytes,                        \
                           current_source + k * sst_bytes, element_size_bytes,  \
                           target_pe);                                          \
        }                                                                       \
      }                                                                         \
                                                                                \
      /* 3. Synchronization (Barrier assumed here) */ /* TODO: Use counter      \
                                                         logic if _algo name    \
                                                         indicates counter */   \
      shcoll_barrier_binomial_tree(PE_start, logPE_stride, PE_size, pSync);     \
    }                                                                           \
                                                                                \
    /* Reset the pSync buffer */                                                \
    shmemc_team_reset_psync(team_h, SHMEMC_PSYNC_ALLTOALL);                     \
    return 0;                                                                   \
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
    /* Check initialization */                                                 \
    SHMEMU_CHECK_INIT();                                                       \
                                                                               \
    /* Check team validity and cast to internal handle */                      \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
    shmemc_team_h team_h = (shmemc_team_h)team; /* Cast to internal handle */  \
                                                                               \
    /* Check for NULL pointers */                                              \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
                                                                               \
    /* Get team parameters */                                                  \
    const int PE_size = team_h->nranks;                                        \
    const int PE_start = team_h->start;         /* Use stored start */         \
    const int stride = team_h->stride;          /* Use stored stride */        \
    SHMEMU_CHECK_TEAM_STRIDE(stride, __func__); /* Check stride if DEBUG */    \
                                                                               \
    /* Check buffer symmetry */                                                \
    size_t element_size_bytes = 0;                                             \
    if (dst != 0 && sst != 0) {                                                \
      /* Calculate element size from strides */                                \
      element_size_bytes = (dst > 0) ? dst : -dst;                             \
    } else {                                                                   \
      /* Default to byte-wise operation */                                     \
      element_size_bytes = 1;                                                  \
    }                                                                          \
                                                                               \
    /* Validate symmetric memory */                                            \
    SHMEMU_CHECK_SYMMETRIC(dest, nelems *element_size_bytes *PE_size);         \
    SHMEMU_CHECK_SYMMETRIC(source, nelems *element_size_bytes *PE_size);       \
                                                                               \
    /* Check for overlap between source and destination */                     \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source,                                  \
                                nelems *element_size_bytes *PE_size,           \
                                nelems *element_size_bytes *PE_size);          \
                                                                               \
    /* Use the pre-allocated pSync buffer from the team structure */           \
    long *pSync = shmemc_team_get_psync(team_h, SHMEMC_PSYNC_ALLTOALL);        \
    SHMEMU_CHECK_NULL(pSync, "team_h->pSyncs[ALLTOALL]");                      \
                                                                               \
    /* Calculate log2 stride, assuming stride is valid */                      \
    int logPE_stride = (stride > 0) ? (int)log2((double)stride) : 0;           \
                                                                               \
    /* Call the appropriate sized implementation based on element size */      \
    if (element_size_bytes == 8) {                                             \
      shcoll_alltoalls64_##_algo(dest, source, dst, sst, nelems, PE_start,     \
                                 logPE_stride, PE_size, pSync);                \
    } else if (element_size_bytes == 4) {                                      \
      shcoll_alltoalls32_##_algo(dest, source, dst, sst, nelems, PE_start,     \
                                 logPE_stride, PE_size, pSync);                \
    } else {                                                                   \
      /* Fallback: Byte-wise operation for other sizes (e.g., long double) */  \
      /* Needs careful implementation based on _algo for sync */               \
      /* For simplicity, we'll use the 8-bit version and adjust nelems */      \
      /* This is less efficient but handles arbitrary element sizes */         \
      shcoll_alltoalls8_##_algo(dest, source, dst, sst, nelems, PE_start,      \
                                logPE_stride, PE_size, pSync);                 \
    }                                                                          \
                                                                               \
    /* Reset the pSync buffer */                                               \
    shmemc_team_reset_psync(team_h, SHMEMC_PSYNC_ALLTOALL);                    \
                                                                               \
    return 0;                                                                  \
  }

/* Define the actual functions */
SHCOLL_ALLTOALLSMEM_DEFINITION(shift_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(shift_exchange_counter)

SHCOLL_ALLTOALLSMEM_DEFINITION(xor_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(xor_pairwise_exchange_counter)

SHCOLL_ALLTOALLSMEM_DEFINITION(color_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DEFINITION(color_pairwise_exchange_counter)