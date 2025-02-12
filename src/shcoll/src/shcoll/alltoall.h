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
#define DECLARE_ALLTOALL_TYPES(_algo)                                          \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, float, float)                       \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, double, double)                     \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, long double, longdouble)            \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, char, char)                         \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, signed char, schar)                 \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, short, short)                       \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, int, int)                           \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, long, long)                         \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, long long, longlong)                \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, unsigned char, uchar)               \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, unsigned short, ushort)             \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, unsigned int, uint)                 \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, unsigned long, ulong)               \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, unsigned long long, ulonglong)      \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, int8_t, int8)                       \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, int16_t, int16)                     \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, int32_t, int32)                     \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, int64_t, int64)                     \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, uint8_t, uint8)                     \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, uint16_t, uint16)                   \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, uint32_t, uint32)                   \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, uint64_t, uint64)                   \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, size_t, size)                       \
  SHCOLL_TYPED_ALLTOALL_DECLARATION(_algo, ptrdiff_t, ptrdiff)

/* Declare all algorithm variants */
DECLARE_ALLTOALL_TYPES(shift_exchange_barrier)
DECLARE_ALLTOALL_TYPES(shift_exchange_counter)
DECLARE_ALLTOALL_TYPES(shift_exchange_signal)
DECLARE_ALLTOALL_TYPES(xor_pairwise_exchange_barrier)
DECLARE_ALLTOALL_TYPES(xor_pairwise_exchange_counter)
DECLARE_ALLTOALL_TYPES(xor_pairwise_exchange_signal)
DECLARE_ALLTOALL_TYPES(color_pairwise_exchange_barrier)
DECLARE_ALLTOALL_TYPES(color_pairwise_exchange_counter)
DECLARE_ALLTOALL_TYPES(color_pairwise_exchange_signal)

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
