/**
 * @file alltoalls.h
 * @brief Header file for strided all-to-all collective operations
 *
 * This header declares the interfaces for strided all-to-all collective
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

#ifndef _SHCOLL_ALLTOALLS_H
#define _SHCOLL_ALLTOALLS_H 1

#include <shmem/teams.h>
#include <shmem/api_types.h>

/**
 * @brief Macro to declare type-specific strided alltoall implementation
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, _type, _typename)            \
  int shcoll_##_typename##_alltoalls_##_algo(                                  \
      shmem_team_t team, _type *dest, const _type *source, ptrdiff_t dst,      \
      ptrdiff_t sst, size_t nelems);

/**
 * @brief Macro to declare strided alltoall implementations for all supported
 * types
 *
 * @param _type Data type
 * @param _typename Type name string
 */
#define DECLARE_ALLTOALLS_TYPES(_type, _typename)                              \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(shift_exchange_barrier, _type, _typename) \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(shift_exchange_counter, _type, _typename) \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier, _type,     \
                                     _typename)                                \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter, _type,     \
                                     _typename)                                \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier, _type,   \
                                     _typename)                                \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter, _type,   \
                                     _typename)

SHMEM_STANDARD_RMA_TYPE_TABLE(DECLARE_ALLTOALLS_TYPES)
#undef DECLARE_ALLTOALLS_TYPES

/**
 * @brief Macro to declare type-specific strided alltoall memory implementation
 *
 * @param _algo Algorithm name
 */
#define SHCOLL_ALLTOALLSMEM_DECLARATION(_algo)                                 \
  int shcoll_alltoallsmem_##_algo(shmem_team_t team, void *dest,               \
                                  const void *source, ptrdiff_t dst,           \
                                  ptrdiff_t sst, size_t nelems);

/* Declare all algorithm variants */
SHCOLL_ALLTOALLSMEM_DECLARATION(shift_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DECLARATION(shift_exchange_counter)
SHCOLL_ALLTOALLSMEM_DECLARATION(xor_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DECLARATION(xor_pairwise_exchange_counter)
SHCOLL_ALLTOALLSMEM_DECLARATION(color_pairwise_exchange_barrier)
SHCOLL_ALLTOALLSMEM_DECLARATION(color_pairwise_exchange_counter)

/**
 * @brief Macro to declare sized strided alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _size Size in bits
 */
#define SHCOLL_SIZED_ALLTOALLS_DECLARATION(_algo, _size)                       \
  void shcoll_alltoalls##_size##_##_algo(                                      \
      void *dest, const void *source, ptrdiff_t dst, ptrdiff_t sst,            \
      size_t nelems, int PE_start, int logPE_stride, int PE_size,              \
      long *pSync);

/* Declare sized variants for each algorithm */
SHCOLL_SIZED_ALLTOALLS_DECLARATION(shift_exchange_barrier, 32)
SHCOLL_SIZED_ALLTOALLS_DECLARATION(shift_exchange_barrier, 64)

SHCOLL_SIZED_ALLTOALLS_DECLARATION(shift_exchange_counter, 32)
SHCOLL_SIZED_ALLTOALLS_DECLARATION(shift_exchange_counter, 64)

SHCOLL_SIZED_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier, 32)
SHCOLL_SIZED_ALLTOALLS_DECLARATION(xor_pairwise_exchange_barrier, 64)

SHCOLL_SIZED_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter, 32)
SHCOLL_SIZED_ALLTOALLS_DECLARATION(xor_pairwise_exchange_counter, 64)

SHCOLL_SIZED_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier, 32)
SHCOLL_SIZED_ALLTOALLS_DECLARATION(color_pairwise_exchange_barrier, 64)

SHCOLL_SIZED_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter, 32)
SHCOLL_SIZED_ALLTOALLS_DECLARATION(color_pairwise_exchange_counter, 64)

#endif /* ! _SHCOLL_ALLTOALLS_H */
