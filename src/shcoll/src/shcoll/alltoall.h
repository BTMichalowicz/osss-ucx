/**
 * @file alltoall.h
 * @brief Header file for all-to-all collective operations
 *
 * This header declares the interfaces for all-to-all collective operations
 * using different algorithms and synchronization methods:
 * - Shift exchange
 * - XOR pairwise exchange
 * - Color pairwise exchange
 *
 * Each algorithm has variants using different synchronization:
 * - Barrier-based
 * - Signal-based
 * - Counter-based
 */

#ifndef _SHCOLL_ALLTOALL_H
#define _SHCOLL_ALLTOALL_H 1

#include <shmem/teams.h>
#include <shmem/api_types.h>
#include "shmemu.h"

/**
 * @brief Macro to declare type-specific alltoall implementation
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, _type, _typename)             \
  int shcoll_##_typename##_alltoall_##_algo(                                   \
      shmem_team_t team, _type *dest, const _type *source, size_t nelems);

/**
 * @brief Macro to declare alltoall implementations for all supported types
 *
 * @param _algo Algorithm name to generate declarations for
 */
#define DECLARE_ALLTOALL_TYPES(_type, _typename)                               \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(shift_exchange_barrier, _type, _typename)  \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(shift_exchange_counter, _type, _typename)  \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(shift_exchange_signal, _type, _typename)   \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(xor_pairwise_exchange_barrier, _type,      \
                                    _typename)                                 \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(xor_pairwise_exchange_counter, _type,      \
                                    _typename)                                 \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(xor_pairwise_exchange_signal, _type,       \
                                    _typename)                                 \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(color_pairwise_exchange_barrier, _type,    \
                                    _typename)                                 \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(color_pairwise_exchange_counter, _type,    \
                                    _typename)                                 \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(color_pairwise_exchange_signal, _type,     \
                                    _typename)

SHMEM_STANDARD_RMA_TYPE_TABLE(DECLARE_ALLTOALL_TYPES)
#undef DECLARE_ALLTOALL_TYPES

/**
 * @brief Macro to declare generic alltoallmem implementations
 *
 * @param _algo Algorithm name to generate declarations for
 */
#define SHCOLL_ALLTOALLMEM_DECLARATION(_algo)                                  \
  int shcoll_alltoallmem_##_algo(shmem_team_t team, void *dest,                \
                                 const void *source, size_t nelems);

SHCOLL_ALLTOALLMEM_DECLARATION(shift_exchange_barrier)
SHCOLL_ALLTOALLMEM_DECLARATION(shift_exchange_counter)
SHCOLL_ALLTOALLMEM_DECLARATION(shift_exchange_signal)
SHCOLL_ALLTOALLMEM_DECLARATION(xor_pairwise_exchange_barrier)
SHCOLL_ALLTOALLMEM_DECLARATION(xor_pairwise_exchange_counter)
SHCOLL_ALLTOALLMEM_DECLARATION(xor_pairwise_exchange_signal)
SHCOLL_ALLTOALLMEM_DECLARATION(color_pairwise_exchange_barrier)
SHCOLL_ALLTOALLMEM_DECLARATION(color_pairwise_exchange_counter)
SHCOLL_ALLTOALLMEM_DECLARATION(color_pairwise_exchange_signal)

/**
 * @brief Macro to declare sized alltoall implementations
 *
 * @param _algo Algorithm name
 * @param _size Size in bits
 */
#define SHCOLL_SIZED_ALLTOALL_DECLARATION(_algo, _size)                        \
  void shcoll_alltoall##_size##_##_algo(                                       \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size, long *pSync);

/* Declare sized variants for each algorithm */
SHCOLL_SIZED_ALLTOALL_DECLARATION(shift_exchange_barrier, 32)
SHCOLL_SIZED_ALLTOALL_DECLARATION(shift_exchange_barrier, 64)

SHCOLL_SIZED_ALLTOALL_DECLARATION(shift_exchange_counter, 32)
SHCOLL_SIZED_ALLTOALL_DECLARATION(shift_exchange_counter, 64)

SHCOLL_SIZED_ALLTOALL_DECLARATION(shift_exchange_signal, 32)
SHCOLL_SIZED_ALLTOALL_DECLARATION(shift_exchange_signal, 64)

SHCOLL_SIZED_ALLTOALL_DECLARATION(xor_pairwise_exchange_barrier, 32)
SHCOLL_SIZED_ALLTOALL_DECLARATION(xor_pairwise_exchange_barrier, 64)

SHCOLL_SIZED_ALLTOALL_DECLARATION(xor_pairwise_exchange_counter, 32)
SHCOLL_SIZED_ALLTOALL_DECLARATION(xor_pairwise_exchange_counter, 64)

SHCOLL_SIZED_ALLTOALL_DECLARATION(xor_pairwise_exchange_signal, 32)
SHCOLL_SIZED_ALLTOALL_DECLARATION(xor_pairwise_exchange_signal, 64)

SHCOLL_SIZED_ALLTOALL_DECLARATION(color_pairwise_exchange_barrier, 32)
SHCOLL_SIZED_ALLTOALL_DECLARATION(color_pairwise_exchange_barrier, 64)

SHCOLL_SIZED_ALLTOALL_DECLARATION(color_pairwise_exchange_counter, 32)
SHCOLL_SIZED_ALLTOALL_DECLARATION(color_pairwise_exchange_counter, 64)

SHCOLL_SIZED_ALLTOALL_DECLARATION(color_pairwise_exchange_signal, 32)
SHCOLL_SIZED_ALLTOALL_DECLARATION(color_pairwise_exchange_signal, 64)

#endif /* ! _SHCOLL_ALLTOALL_H */
