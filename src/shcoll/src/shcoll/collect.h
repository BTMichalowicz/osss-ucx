/**
 * @file collect.h
 * @brief Header file for collect collective operations
 *
 * This header declares the interfaces for collect collective operations
 * using different algorithms and synchronization methods:
 * - Linear
 * - All linear
 * - Recursive doubling
 * - Ring
 * - Bruck
 * - Simple
 *
 * Some algorithms have variants using different synchronization:
 * - Signal-based
 */

#ifndef _SHCOLL_COLLECT_H
#define _SHCOLL_COLLECT_H 1

#include <shmem/teams.h>

/**
 * @brief Macro to declare type-specific collect implementation
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_TYPED_COLLECT_DECLARATION(_algo, _type, _typename)              \
  int shcoll_##_typename##_collect_##_algo(                                    \
      shmem_team_t team, _type *dest, const _type *source, size_t nelems);

/**
 * @brief Macro to declare collect implementations for all supported types
 *
 * @param _algo Algorithm name to generate declarations for
 */
#define DECLARE_COLLECT_TYPES(_algo)                                           \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, float, float)                        \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, double, double)                      \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, long double, longdouble)             \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, char, char)                          \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, signed char, schar)                  \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, short, short)                        \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, int, int)                            \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, long, long)                          \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, long long, longlong)                 \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, unsigned char, uchar)                \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, unsigned short, ushort)              \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, unsigned int, uint)                  \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, unsigned long, ulong)                \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, unsigned long long, ulonglong)       \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, int8_t, int8)                        \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, int16_t, int16)                      \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, int32_t, int32)                      \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, int64_t, int64)                      \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, uint8_t, uint8)                      \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, uint16_t, uint16)                    \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, uint32_t, uint32)                    \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, uint64_t, uint64)                    \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, size_t, size)                        \
  SHCOLL_TYPED_COLLECT_DECLARATION(_algo, ptrdiff_t, ptrdiff)

/* Declare all algorithm variants */
DECLARE_COLLECT_TYPES(linear)
DECLARE_COLLECT_TYPES(all_linear)
DECLARE_COLLECT_TYPES(all_linear1)
DECLARE_COLLECT_TYPES(rec_dbl)
DECLARE_COLLECT_TYPES(rec_dbl_signal)
DECLARE_COLLECT_TYPES(ring)
DECLARE_COLLECT_TYPES(bruck)
DECLARE_COLLECT_TYPES(bruck_no_rotate)
DECLARE_COLLECT_TYPES(simple)

/**
 * @brief Macro to declare sized collect implementations
 *
 * @param _algo Algorithm name
 * @param _size Size in bits
 */
#define SHCOLL_SIZED_COLLECT_DECLARATION(_algo, _size)                         \
  void shcoll_collect##_size##_##_algo(                                        \
      void *dest, const void *source, size_t nelems, int PE_start,             \
      int logPE_stride, int PE_size, long *pSync);

/* Declare sized variants for each algorithm */
SHCOLL_SIZED_COLLECT_DECLARATION(linear, 32)
SHCOLL_SIZED_COLLECT_DECLARATION(linear, 64)

SHCOLL_SIZED_COLLECT_DECLARATION(all_linear, 32)
SHCOLL_SIZED_COLLECT_DECLARATION(all_linear, 64)

SHCOLL_SIZED_COLLECT_DECLARATION(all_linear1, 32)
SHCOLL_SIZED_COLLECT_DECLARATION(all_linear1, 64)

SHCOLL_SIZED_COLLECT_DECLARATION(rec_dbl, 32)
SHCOLL_SIZED_COLLECT_DECLARATION(rec_dbl, 64)

SHCOLL_SIZED_COLLECT_DECLARATION(rec_dbl_signal, 32)
SHCOLL_SIZED_COLLECT_DECLARATION(rec_dbl_signal, 64)

SHCOLL_SIZED_COLLECT_DECLARATION(ring, 32)
SHCOLL_SIZED_COLLECT_DECLARATION(ring, 64)

SHCOLL_SIZED_COLLECT_DECLARATION(bruck, 32)
SHCOLL_SIZED_COLLECT_DECLARATION(bruck, 64)

SHCOLL_SIZED_COLLECT_DECLARATION(bruck_no_rotate, 32)
SHCOLL_SIZED_COLLECT_DECLARATION(bruck_no_rotate, 64)

SHCOLL_SIZED_COLLECT_DECLARATION(simple, 32)
SHCOLL_SIZED_COLLECT_DECLARATION(simple, 64)

#endif /* ! _SHCOLL_COLLECT_H */
