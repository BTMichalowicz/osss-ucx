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
 * @param _algo Algorithm name to generate declarations for
 */
#define DECLARE_ALLTOALLS_TYPES(_algo)                                         \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, float, float)                      \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, double, double)                    \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, long double, longdouble)           \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, char, char)                        \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, signed char, schar)                \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, short, short)                      \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, int, int)                          \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, long, long)                        \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, long long, longlong)               \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, unsigned char, uchar)              \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, unsigned short, ushort)            \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, unsigned int, uint)                \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, unsigned long, ulong)              \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, unsigned long long, ulonglong)     \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, int8_t, int8)                      \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, int16_t, int16)                    \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, int32_t, int32)                    \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, int64_t, int64)                    \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, uint8_t, uint8)                    \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, uint16_t, uint16)                  \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, uint32_t, uint32)                  \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, uint64_t, uint64)                  \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, size_t, size)                      \
  SHCOLL_TYPED_ALLTOALLS_DECLARATION(_algo, ptrdiff_t, ptrdiff)

/* Declare all algorithm variants */
DECLARE_ALLTOALLS_TYPES(shift_exchange_barrier)
DECLARE_ALLTOALLS_TYPES(shift_exchange_counter)
DECLARE_ALLTOALLS_TYPES(xor_pairwise_exchange_barrier)
DECLARE_ALLTOALLS_TYPES(xor_pairwise_exchange_counter)
DECLARE_ALLTOALLS_TYPES(color_pairwise_exchange_barrier)
DECLARE_ALLTOALLS_TYPES(color_pairwise_exchange_counter)

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