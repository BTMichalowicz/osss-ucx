/**
 * @file fcollect.h
 * @brief Header file for fixed-size collect collective operations
 *
 * This header declares the interfaces for fixed-size collect collective
 * operations using different algorithms and synchronization methods:
 * - Linear
 * - All linear
 * - Recursive doubling
 * - Ring
 * - Bruck
 * - Neighbor exchange
 *
 * Some algorithms have variants using different synchronization:
 * - Signal-based
 */

#ifndef _SHCOLL_FCOLLECT_H
#define _SHCOLL_FCOLLECT_H 1

#include <shmem/teams.h>

/**
 * @brief Macro to declare type-specific fcollect implementation
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, _type, _typename)             \
  int shcoll_##_typename##_fcollect_##_algo(                                   \
      shmem_team_t team, _type *dest, const _type *source, size_t nelems);

/**
 * @brief Macro to declare fcollect implementations for all supported types
 *
 * @param _algo Algorithm name to generate declarations for
 */
#define DECLARE_FCOLLECT_TYPES(_algo)                                          \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, float, float)                       \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, double, double)                     \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, long double, longdouble)            \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, char, char)                         \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, signed char, schar)                 \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, short, short)                       \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, int, int)                           \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, long, long)                         \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, long long, longlong)                \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, unsigned char, uchar)               \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, unsigned short, ushort)             \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, unsigned int, uint)                 \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, unsigned long, ulong)               \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, unsigned long long, ulonglong)      \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, int8_t, int8)                       \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, int16_t, int16)                     \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, int32_t, int32)                     \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, int64_t, int64)                     \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, uint8_t, uint8)                     \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, uint16_t, uint16)                   \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, uint32_t, uint32)                   \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, uint64_t, uint64)                   \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, size_t, size)                       \
  SHCOLL_TYPED_FCOLLECT_DECLARATION(_algo, ptrdiff_t, ptrdiff)

/* Declare all algorithm variants */
DECLARE_FCOLLECT_TYPES(linear)
DECLARE_FCOLLECT_TYPES(all_linear)
DECLARE_FCOLLECT_TYPES(all_linear1)
DECLARE_FCOLLECT_TYPES(rec_dbl)
DECLARE_FCOLLECT_TYPES(rec_dbl_signal)
DECLARE_FCOLLECT_TYPES(ring)
DECLARE_FCOLLECT_TYPES(bruck)
DECLARE_FCOLLECT_TYPES(bruck_no_rotate)
DECLARE_FCOLLECT_TYPES(bruck_signal)
DECLARE_FCOLLECT_TYPES(bruck_inplace)
DECLARE_FCOLLECT_TYPES(neighbor_exchange)

/**
 * @brief Macro to declare type-specific fcollect memory implementation
 *
 * @param _algo Algorithm name
 */
#define SHCOLL_FCOLLECTMEM_DECLARATION(_algo)                                  \
  int shcoll_fcollectmem_##_algo(shmem_team_t team, void *dest,                \
                                 const void *source, size_t nelems);

/* Declare all algorithm variants */
SHCOLL_FCOLLECTMEM_DECLARATION(linear)
SHCOLL_FCOLLECTMEM_DECLARATION(all_linear)
SHCOLL_FCOLLECTMEM_DECLARATION(all_linear1)
SHCOLL_FCOLLECTMEM_DECLARATION(rec_dbl)
SHCOLL_FCOLLECTMEM_DECLARATION(rec_dbl_signal)
SHCOLL_FCOLLECTMEM_DECLARATION(ring)
SHCOLL_FCOLLECTMEM_DECLARATION(bruck)
SHCOLL_FCOLLECTMEM_DECLARATION(bruck_no_rotate)
SHCOLL_FCOLLECTMEM_DECLARATION(bruck_signal)
SHCOLL_FCOLLECTMEM_DECLARATION(bruck_inplace)
SHCOLL_FCOLLECTMEM_DECLARATION(neighbor_exchange)

/*
 * @brief Macro to declare sized fcollect implementations
 *
 * @param _algo Algorithm name
 * @param _size Size in bits
 */
#define SHCOLL_SIZED_FCOLLECT_DECLARATION(_algo, _size)                        \
  void shcoll_fcollect##_size##_##_algo(                                       \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size, long *pSync);

/* Declare sized variants for each algorithm */
SHCOLL_SIZED_FCOLLECT_DECLARATION(linear, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(linear, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(all_linear, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(all_linear, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(all_linear1, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(all_linear1, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(rec_dbl, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(rec_dbl, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(rec_dbl_signal, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(rec_dbl_signal, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(ring, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(ring, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(bruck, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(bruck, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(bruck_no_rotate, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(bruck_no_rotate, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(bruck_signal, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(bruck_signal, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(bruck_inplace, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(bruck_inplace, 64)

SHCOLL_SIZED_FCOLLECT_DECLARATION(neighbor_exchange, 32)
SHCOLL_SIZED_FCOLLECT_DECLARATION(neighbor_exchange, 64)

#endif /* ! _SHCOLL_FCOLLECT_H */
