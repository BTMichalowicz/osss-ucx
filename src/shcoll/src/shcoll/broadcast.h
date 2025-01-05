#ifndef _SHCOLL_BROADCAST_H
#define _SHCOLL_BROADCAST_H 1

#include <shmem/teams.h>

void shcoll_set_broadcast_tree_degree(int tree_degree);
void shcoll_set_broadcast_knomial_tree_radix_barrier(int tree_radix);

/**
 * @brief Macro to declare type-specific broadcast implementation
 */
#define SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, _type, _typename)            \
  int shcoll_##_typename##_broadcast_##_algo(shmem_team_t team, _type *dest,   \
                                             const _type *source,              \
                                             size_t nelems, int PE_root);

/**
 * @brief Macro to declare broadcast implementations for all supported types
 */
#define DECLARE_BROADCAST_TYPES(_algo)                                         \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, float, float)                      \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, double, double)                    \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, long double, longdouble)           \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, char, char)                        \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, signed char, schar)                \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, short, short)                      \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, int, int)                          \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, long, long)                        \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, long long, longlong)               \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, unsigned char, uchar)              \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, unsigned short, ushort)            \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, unsigned int, uint)                \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, unsigned long, ulong)              \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, unsigned long long, ulonglong)     \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, int8_t, int8)                      \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, int16_t, int16)                    \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, int32_t, int32)                    \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, int64_t, int64)                    \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, uint8_t, uint8)                    \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, uint16_t, uint16)                  \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, uint32_t, uint32)                  \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, uint64_t, uint64)                  \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, size_t, size)                      \
  SHCOLL_TYPED_BROADCAST_DECLARATION(_algo, ptrdiff_t, ptrdiff)

/* Declare all typed algorithm variants */
DECLARE_BROADCAST_TYPES(linear)
DECLARE_BROADCAST_TYPES(complete_tree)
DECLARE_BROADCAST_TYPES(binomial_tree)
DECLARE_BROADCAST_TYPES(knomial_tree)
DECLARE_BROADCAST_TYPES(knomial_tree_signal)
DECLARE_BROADCAST_TYPES(scatter_collect)

/**
 * @brief Macro to declare type-specific broadcast memory implementation
 *
 * @param _algo Algorithm name
 */
#define SHCOLL_BROADCASTMEM_DECLARATION(_algo)                                 \
  int shcoll_broadcastmem_##_algo(shmem_team_t team, void *dest,               \
                                  const void *source, size_t nelems);

/* Declare all algorithm variants */
SHCOLL_BROADCASTMEM_DECLARATION(linear)
SHCOLL_BROADCASTMEM_DECLARATION(complete_tree)
SHCOLL_BROADCASTMEM_DECLARATION(binomial_tree)
SHCOLL_BROADCASTMEM_DECLARATION(knomial_tree)
SHCOLL_BROADCASTMEM_DECLARATION(knomial_tree_signal)
SHCOLL_BROADCASTMEM_DECLARATION(scatter_collect)

/**
 * @brief Macro to declare sized broadcast implementations
 */
#define SHCOLL_SIZED_BROADCAST_DECLARATION(_algo, _size)                       \
  void shcoll_broadcast##_size##_##_algo(                                      \
      void *dest, const void *source, size_t nelems, int PE_root,              \
      int PE_start, int logPE_stride, int PE_size, long *pSync);

/* Declare sized variants for each algorithm */
SHCOLL_SIZED_BROADCAST_DECLARATION(linear, 8)
SHCOLL_SIZED_BROADCAST_DECLARATION(linear, 16)
SHCOLL_SIZED_BROADCAST_DECLARATION(linear, 32)
SHCOLL_SIZED_BROADCAST_DECLARATION(linear, 64)

SHCOLL_SIZED_BROADCAST_DECLARATION(complete_tree, 8)
SHCOLL_SIZED_BROADCAST_DECLARATION(complete_tree, 16)
SHCOLL_SIZED_BROADCAST_DECLARATION(complete_tree, 32)
SHCOLL_SIZED_BROADCAST_DECLARATION(complete_tree, 64)

SHCOLL_SIZED_BROADCAST_DECLARATION(binomial_tree, 8)
SHCOLL_SIZED_BROADCAST_DECLARATION(binomial_tree, 16)
SHCOLL_SIZED_BROADCAST_DECLARATION(binomial_tree, 32)
SHCOLL_SIZED_BROADCAST_DECLARATION(binomial_tree, 64)

SHCOLL_SIZED_BROADCAST_DECLARATION(knomial_tree, 8)
SHCOLL_SIZED_BROADCAST_DECLARATION(knomial_tree, 16)
SHCOLL_SIZED_BROADCAST_DECLARATION(knomial_tree, 32)
SHCOLL_SIZED_BROADCAST_DECLARATION(knomial_tree, 64)

SHCOLL_SIZED_BROADCAST_DECLARATION(knomial_tree_signal, 8)
SHCOLL_SIZED_BROADCAST_DECLARATION(knomial_tree_signal, 16)
SHCOLL_SIZED_BROADCAST_DECLARATION(knomial_tree_signal, 32)
SHCOLL_SIZED_BROADCAST_DECLARATION(knomial_tree_signal, 64)

SHCOLL_SIZED_BROADCAST_DECLARATION(scatter_collect, 8)
SHCOLL_SIZED_BROADCAST_DECLARATION(scatter_collect, 16)
SHCOLL_SIZED_BROADCAST_DECLARATION(scatter_collect, 32)
SHCOLL_SIZED_BROADCAST_DECLARATION(scatter_collect, 64)

#endif /* ! _SHCOLL_BROADCAST_H */
