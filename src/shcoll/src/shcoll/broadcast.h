#ifndef _SHCOLL_BROADCAST_H
#define _SHCOLL_BROADCAST_H 1

#include <shmem/teams.h>

void shcoll_set_broadcast_tree_degree(int tree_degree);
void shcoll_set_broadcast_knomial_tree_radix_barrier(int tree_radix);

#define SHCOLL_BROADCAST_DECLARATION(_algo, _type, _typename)                  \
  int shcoll_##_typename##_broadcast_##_algo(                                  \
      shmem_team_t team, _type *dest, const _type *source, size_t nelems);

/* Define all supported data types */
#define DECLARE_FOR_ALL_TYPES_BROADCAST(_algo)                                 \
  SHCOLL_BROADCAST_DECLARATION(_algo, float, float)                            \
  SHCOLL_BROADCAST_DECLARATION(_algo, double, double)                          \
  SHCOLL_BROADCAST_DECLARATION(_algo, long double, longdouble)                 \
  SHCOLL_BROADCAST_DECLARATION(_algo, char, char)                              \
  SHCOLL_BROADCAST_DECLARATION(_algo, signed char, schar)                      \
  SHCOLL_BROADCAST_DECLARATION(_algo, short, short)                            \
  SHCOLL_BROADCAST_DECLARATION(_algo, int, int)                                \
  SHCOLL_BROADCAST_DECLARATION(_algo, long, long)                              \
  SHCOLL_BROADCAST_DECLARATION(_algo, long long, longlong)                     \
  SHCOLL_BROADCAST_DECLARATION(_algo, unsigned char, uchar)                    \
  SHCOLL_BROADCAST_DECLARATION(_algo, unsigned short, ushort)                  \
  SHCOLL_BROADCAST_DECLARATION(_algo, unsigned int, uint)                      \
  SHCOLL_BROADCAST_DECLARATION(_algo, unsigned long, ulong)                    \
  SHCOLL_BROADCAST_DECLARATION(_algo, unsigned long long, ulonglong)           \
  SHCOLL_BROADCAST_DECLARATION(_algo, int8_t, int8)                            \
  SHCOLL_BROADCAST_DECLARATION(_algo, int16_t, int16)                          \
  SHCOLL_BROADCAST_DECLARATION(_algo, int32_t, int32)                          \
  SHCOLL_BROADCAST_DECLARATION(_algo, int64_t, int64)                          \
  SHCOLL_BROADCAST_DECLARATION(_algo, uint8_t, uint8)                          \
  SHCOLL_BROADCAST_DECLARATION(_algo, uint16_t, uint16)                        \
  SHCOLL_BROADCAST_DECLARATION(_algo, uint32_t, uint32)                        \
  SHCOLL_BROADCAST_DECLARATION(_algo, uint64_t, uint64)                        \
  SHCOLL_BROADCAST_DECLARATION(_algo, size_t, size)                            \
  SHCOLL_BROADCAST_DECLARATION(_algo, ptrdiff_t, ptrdiff)

/* Declare for all algorithms */
DECLARE_FOR_ALL_TYPES_BROADCAST(linear)
DECLARE_FOR_ALL_TYPES_BROADCAST(complete_tree)
DECLARE_FOR_ALL_TYPES_BROADCAST(binomial_tree)
DECLARE_FOR_ALL_TYPES_BROADCAST(knomial_tree)
DECLARE_FOR_ALL_TYPES_BROADCAST(knomial_tree_signal)
DECLARE_FOR_ALL_TYPES_BROADCAST(scatter_collect)

#endif /* ! _SHCOLL_BROADCAST_H */
