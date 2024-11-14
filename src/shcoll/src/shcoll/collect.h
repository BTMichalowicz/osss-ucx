#ifndef _SHCOLL_COLLECT_H
#define _SHCOLL_COLLECT_H 1

#include <shmem/teams.h>

#define SHCOLL_COLLECT_DECLARATION(_algo, _type, _typename)                    \
  int shcoll_##_typename##_collect_##_algo(                                    \
      shmem_team_t team, _type *dest, const _type *source, size_t nelems);

/* Define all supported data types */
#define DECLARE_FOR_ALL_TYPES_COLLECT(_algo)                                   \
  SHCOLL_COLLECT_DECLARATION(_algo, float, float)                              \
  SHCOLL_COLLECT_DECLARATION(_algo, double, double)                            \
  SHCOLL_COLLECT_DECLARATION(_algo, long double, longdouble)                   \
  SHCOLL_COLLECT_DECLARATION(_algo, char, char)                                \
  SHCOLL_COLLECT_DECLARATION(_algo, signed char, schar)                        \
  SHCOLL_COLLECT_DECLARATION(_algo, short, short)                              \
  SHCOLL_COLLECT_DECLARATION(_algo, int, int)                                  \
  SHCOLL_COLLECT_DECLARATION(_algo, long, long)                                \
  SHCOLL_COLLECT_DECLARATION(_algo, long long, longlong)                       \
  SHCOLL_COLLECT_DECLARATION(_algo, unsigned char, uchar)                      \
  SHCOLL_COLLECT_DECLARATION(_algo, unsigned short, ushort)                    \
  SHCOLL_COLLECT_DECLARATION(_algo, unsigned int, uint)                        \
  SHCOLL_COLLECT_DECLARATION(_algo, unsigned long, ulong)                      \
  SHCOLL_COLLECT_DECLARATION(_algo, unsigned long long, ulonglong)             \
  SHCOLL_COLLECT_DECLARATION(_algo, int8_t, int8)                              \
  SHCOLL_COLLECT_DECLARATION(_algo, int16_t, int16)                            \
  SHCOLL_COLLECT_DECLARATION(_algo, int32_t, int32)                            \
  SHCOLL_COLLECT_DECLARATION(_algo, int64_t, int64)                            \
  SHCOLL_COLLECT_DECLARATION(_algo, uint8_t, uint8)                            \
  SHCOLL_COLLECT_DECLARATION(_algo, uint16_t, uint16)                          \
  SHCOLL_COLLECT_DECLARATION(_algo, uint32_t, uint32)                          \
  SHCOLL_COLLECT_DECLARATION(_algo, uint64_t, uint64)                          \
  SHCOLL_COLLECT_DECLARATION(_algo, size_t, size)                              \
  SHCOLL_COLLECT_DECLARATION(_algo, ptrdiff_t, ptrdiff)

/* Declare for all algorithms */
DECLARE_FOR_ALL_TYPES_COLLECT(linear)
DECLARE_FOR_ALL_TYPES_COLLECT(all_linear)
DECLARE_FOR_ALL_TYPES_COLLECT(all_linear1)
DECLARE_FOR_ALL_TYPES_COLLECT(rec_dbl)
DECLARE_FOR_ALL_TYPES_COLLECT(rec_dbl_signal)
DECLARE_FOR_ALL_TYPES_COLLECT(ring)
DECLARE_FOR_ALL_TYPES_COLLECT(bruck)
DECLARE_FOR_ALL_TYPES_COLLECT(bruck_no_rotate)

#endif /* ! _SHCOLL_COLLECT_H */









// #ifndef _SHCOLL_COLLECT_H
// #define _SHCOLL_COLLECT_H 1

// #define SHCOLL_COLLECT_DECLARATION(_algo, _size)                               \
//   void shcoll_collect##_size##_##_algo(                                        \
//       void *dest, const void *source, size_t nelems, int PE_start,             \
//       int logPE_stride, int PE_size, long *pSync);

// SHCOLL_COLLECT_DECLARATION(linear, 32)
// SHCOLL_COLLECT_DECLARATION(linear, 64)

// SHCOLL_COLLECT_DECLARATION(all_linear, 32)
// SHCOLL_COLLECT_DECLARATION(all_linear, 64)

// SHCOLL_COLLECT_DECLARATION(all_linear1, 32)
// SHCOLL_COLLECT_DECLARATION(all_linear1, 64)

// SHCOLL_COLLECT_DECLARATION(rec_dbl, 32)
// SHCOLL_COLLECT_DECLARATION(rec_dbl, 64)

// SHCOLL_COLLECT_DECLARATION(rec_dbl_signal, 32)
// SHCOLL_COLLECT_DECLARATION(rec_dbl_signal, 64)

// SHCOLL_COLLECT_DECLARATION(ring, 32)
// SHCOLL_COLLECT_DECLARATION(ring, 64)

// SHCOLL_COLLECT_DECLARATION(bruck, 32)
// SHCOLL_COLLECT_DECLARATION(bruck, 64)

// SHCOLL_COLLECT_DECLARATION(bruck_no_rotate, 32)
// SHCOLL_COLLECT_DECLARATION(bruck_no_rotate, 64)

// #endif /* ! _SHCOLL_COLLECT_H */
