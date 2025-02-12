#ifndef _TYPED_H
#define _TYPED_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <shcoll.h>
#include <shmem/teams.h>
#include <shmem.h>

#include "collectives/defaults.h"

// FIXME: make sure all the types in the pshmem interface as well as the
//        API are correct

/**
 * @defgroup typed_collectives Typed Collectives
 * @brief Typed team-based collectives are collectives that operate on a
 * specific data type.
 * @{
 */

/****************************************************************************/
#ifdef ENABLE_PSHMEM
#pragma weak shmem_int_alltoall = pshmem_int_alltoall
#define shmem_int_alltoall pshmem_int_alltoall
#pragma weak shmem_long_alltoall = pshmem_long_alltoall
#define shmem_long_alltoall pshmem_long_alltoall
#pragma weak shmem_longlong_alltoall = pshmem_longlong_alltoall
#define shmem_longlong_alltoall pshmem_longlong_alltoall
#pragma weak shmem_float_alltoall = pshmem_float_alltoall
#define shmem_float_alltoall pshmem_float_alltoall
#pragma weak shmem_double_alltoall = pshmem_double_alltoall
#define shmem_double_alltoall pshmem_double_alltoall
#pragma weak shmem_longdouble_alltoall = pshmem_longdouble_alltoall
#define shmem_longdouble_alltoall pshmem_longdouble_alltoall
#pragma weak shmem_uint_alltoall = pshmem_uint_alltoall
#define shmem_uint_alltoall pshmem_uint_alltoall
#pragma weak shmem_ulong_alltoall = pshmem_ulong_alltoall
#define shmem_ulong_alltoall pshmem_ulong_alltoall
#pragma weak shmem_ulonglong_alltoall = pshmem_ulonglong_alltoall
#define shmem_ulonglong_alltoall pshmem_ulonglong_alltoall
#pragma weak shmem_int32_alltoall = pshmem_int32_alltoall
#define shmem_int32_alltoall pshmem_int32_alltoall
#pragma weak shmem_int64_alltoall = pshmem_int64_alltoall
#define shmem_int64_alltoall pshmem_int64_alltoall
#pragma weak shmem_uint32_alltoall = pshmem_uint32_alltoall
#define shmem_uint32_alltoall pshmem_uint32_alltoall
#pragma weak shmem_uint64_alltoall = pshmem_uint64_alltoall
#define shmem_uint64_alltoall pshmem_uint64_alltoall
#pragma weak shmem_size_alltoall = pshmem_size_alltoall
#define shmem_size_alltoall pshmem_size_alltoall
#pragma weak shmem_ptrdiff_alltoall = pshmem_ptrdiff_alltoall
#define shmem_ptrdiff_alltoall pshmem_ptrdiff_alltoall
#endif /* ENABLE_PSHMEM */

// FIXME: COLLECTIVES_DEFAULT_ALLTOALL is not working because it's a string
// literal

#define SHIM_ALLTOALL_DECLARE(_typename, _type, _algo)                         \
  int shmem_##_typename##_alltoall(shmem_team_t team, _type *dest,             \
                                   const _type *source, size_t nelems) {       \
    shcoll_##_typename##_alltoall_##_algo(team, dest, source, nelems);         \
  }

#define SHIM_ALLTOALL_TYPE(_algo)                                              \
  SHIM_ALLTOALL_DECLARE(float, float, _algo)                                   \
  SHIM_ALLTOALL_DECLARE(double, double, _algo)                                 \
  SHIM_ALLTOALL_DECLARE(longdouble, long double, _algo)                        \
  SHIM_ALLTOALL_DECLARE(char, char, _algo)                                     \
  SHIM_ALLTOALL_DECLARE(schar, signed char, _algo)                             \
  SHIM_ALLTOALL_DECLARE(short, short, _algo)                                   \
  SHIM_ALLTOALL_DECLARE(int, int, _algo)                                       \
  SHIM_ALLTOALL_DECLARE(long, long, _algo)                                     \
  SHIM_ALLTOALL_DECLARE(longlong, long long, _algo)                            \
  SHIM_ALLTOALL_DECLARE(uchar, unsigned char, _algo)                           \
  SHIM_ALLTOALL_DECLARE(ushort, unsigned short, _algo)                         \
  SHIM_ALLTOALL_DECLARE(uint, unsigned int, _algo)                             \
  SHIM_ALLTOALL_DECLARE(ulong, unsigned long, _algo)                           \
  SHIM_ALLTOALL_DECLARE(ulonglong, unsigned long long, _algo)                  \
  SHIM_ALLTOALL_DECLARE(int8, int8_t, _algo)                                   \
  SHIM_ALLTOALL_DECLARE(int16, int16_t, _algo)                                 \
  SHIM_ALLTOALL_DECLARE(int32, int32_t, _algo)                                 \
  SHIM_ALLTOALL_DECLARE(int64, int64_t, _algo)                                 \
  SHIM_ALLTOALL_DECLARE(uint8, uint8_t, _algo)                                 \
  SHIM_ALLTOALL_DECLARE(uint16, uint16_t, _algo)                               \
  SHIM_ALLTOALL_DECLARE(uint32, uint32_t, _algo)                               \
  SHIM_ALLTOALL_DECLARE(uint64, uint64_t, _algo)                               \
  SHIM_ALLTOALL_DECLARE(size, size_t, _algo)                                   \
  SHIM_ALLTOALL_DECLARE(ptrdiff, ptrdiff_t, _algo)

/****************************************************************************/

#ifdef ENABLE_PSHMEM
#pragma weak shmem_int_alltoalls = pshmem_int_alltoalls
#define shmem_int_alltoalls pshmem_int_alltoalls
#pragma weak shmem_long_alltoalls = pshmem_long_alltoalls
#define shmem_long_alltoalls pshmem_long_alltoalls
#pragma weak shmem_longlong_alltoalls = pshmem_longlong_alltoalls
#define shmem_longlong_alltoalls pshmem_longlong_alltoalls
#pragma weak shmem_float_alltoalls = pshmem_float_alltoalls
#define shmem_float_alltoalls pshmem_float_alltoalls
#pragma weak shmem_double_alltoalls = pshmem_double_alltoalls
#define shmem_double_alltoalls pshmem_double_alltoalls
#pragma weak shmem_longdouble_alltoalls = pshmem_longdouble_alltoalls
#define shmem_longdouble_alltoalls pshmem_longdouble_alltoalls
#pragma weak shmem_uint_alltoalls = pshmem_uint_alltoalls
#define shmem_uint_alltoalls pshmem_uint_alltoalls
#pragma weak shmem_ulong_alltoalls = pshmem_ulong_alltoalls
#define shmem_ulong_alltoalls pshmem_ulong_alltoalls
#pragma weak shmem_ulonglong_alltoalls = pshmem_ulonglong_alltoalls
#define shmem_ulonglong_alltoalls pshmem_ulonglong_alltoalls
#pragma weak shmem_int32_alltoalls = pshmem_int32_alltoalls
#define shmem_int32_alltoalls pshmem_int32_alltoalls
#pragma weak shmem_int64_alltoalls = pshmem_int64_alltoalls
#define shmem_int64_alltoalls pshmem_int64_alltoalls
#pragma weak shmem_uint32_alltoalls = pshmem_uint32_alltoalls
#define shmem_uint32_alltoalls pshmem_uint32_alltoalls
#pragma weak shmem_uint64_alltoalls = pshmem_uint64_alltoalls
#define shmem_uint64_alltoalls pshmem_uint64_alltoalls
#pragma weak shmem_size_alltoalls = pshmem_size_alltoalls
#define shmem_size_alltoalls pshmem_size_alltoalls
#pragma weak shmem_ptrdiff_alltoalls = pshmem_ptrdiff_alltoalls
#define shmem_ptrdiff_alltoalls pshmem_ptrdiff_alltoalls
#endif /* ENABLE_PSHMEM */

#define SHIM_ALLTOALLS_DECLARE(_typename, _type, _algo)                        \
  int shmem_##_typename##_alltoalls(shmem_team_t team, _type *dest,            \
                                    const _type *source, ptrdiff_t dst,        \
                                    ptrdiff_t sst, size_t nelems) {            \
    shcoll_##_typename##_alltoalls_##_algo(team, dest, source, dst, sst,       \
                                           nelems);                            \
  }

#define SHIM_ALLTOALLS_TYPE(_algo)                                             \
  SHIM_ALLTOALLS_DECLARE(float, float, _algo)                                  \
  SHIM_ALLTOALLS_DECLARE(double, double, _algo)                                \
  SHIM_ALLTOALLS_DECLARE(longdouble, long double, _algo)                       \
  SHIM_ALLTOALLS_DECLARE(char, char, _algo)                                    \
  SHIM_ALLTOALLS_DECLARE(schar, signed char, _algo)                            \
  SHIM_ALLTOALLS_DECLARE(short, short, _algo)                                  \
  SHIM_ALLTOALLS_DECLARE(int, int, _algo)                                      \
  SHIM_ALLTOALLS_DECLARE(long, long, _algo)                                    \
  SHIM_ALLTOALLS_DECLARE(longlong, long long, _algo)                           \
  SHIM_ALLTOALLS_DECLARE(uchar, unsigned char, _algo)                          \
  SHIM_ALLTOALLS_DECLARE(ushort, unsigned short, _algo)                        \
  SHIM_ALLTOALLS_DECLARE(uint, unsigned int, _algo)                            \
  SHIM_ALLTOALLS_DECLARE(ulong, unsigned long, _algo)                          \
  SHIM_ALLTOALLS_DECLARE(ulonglong, unsigned long long, _algo)                 \
  SHIM_ALLTOALLS_DECLARE(int8, int8_t, _algo)                                  \
  SHIM_ALLTOALLS_DECLARE(int16, int16_t, _algo)                                \
  SHIM_ALLTOALLS_DECLARE(int32, int32_t, _algo)                                \
  SHIM_ALLTOALLS_DECLARE(int64, int64_t, _algo)                                \
  SHIM_ALLTOALLS_DECLARE(uint8, uint8_t, _algo)                                \
  SHIM_ALLTOALLS_DECLARE(uint16, uint16_t, _algo)                              \
  SHIM_ALLTOALLS_DECLARE(uint32, uint32_t, _algo)                              \
  SHIM_ALLTOALLS_DECLARE(uint64, uint64_t, _algo)                              \
  SHIM_ALLTOALLS_DECLARE(size, size_t, _algo)                                  \
  SHIM_ALLTOALLS_DECLARE(ptrdiff, ptrdiff_t, _algo)

/****************************************************************************/

#ifdef ENABLE_PSHMEM
#pragma weak shmem_float_collect = pshmem_float_collect
#define shmem_float_collect pshmem_float_collect
#pragma weak shmem_double_collect = pshmem_double_collect
#define shmem_double_collect pshmem_double_collect
#pragma weak shmem_longdouble_collect = pshmem_longdouble_collect
#define shmem_longdouble_collect pshmem_longdouble_collect
#pragma weak shmem_char_collect = pshmem_char_collect
#define shmem_char_collect pshmem_char_collect
#pragma weak shmem_schar_collect = pshmem_schar_collect
#define shmem_schar_collect pshmem_schar_collect
#pragma weak shmem_short_collect = pshmem_short_collect
#define shmem_short_collect pshmem_short_collect
#pragma weak shmem_int_collect = pshmem_int_collect
#define shmem_int_collect pshmem_int_collect
#pragma weak shmem_long_collect = pshmem_long_collect
#define shmem_long_collect pshmem_long_collect
#pragma weak shmem_longlong_collect = pshmem_longlong_collect
#define shmem_longlong_collect pshmem_longlong_collect
#pragma weak shmem_uchar_collect = pshmem_uchar_collect
#define shmem_uchar_collect pshmem_uchar_collect
#pragma weak shmem_ushort_collect = pshmem_ushort_collect
#define shmem_ushort_collect pshmem_ushort_collect
#pragma weak shmem_uint_collect = pshmem_uint_collect
#define shmem_uint_collect pshmem_uint_collect
#pragma weak shmem_ulong_collect = pshmem_ulong_collect
#define shmem_ulong_collect pshmem_ulong_collect
#pragma weak shmem_ulonglong_collect = pshmem_ulonglong_collect
#define shmem_ulonglong_collect pshmem_ulonglong_collect
#pragma weak shmem_int8_collect = pshmem_int8_collect
#define shmem_int8_collect pshmem_int8_collect
#pragma weak shmem_int16_collect = pshmem_int16_collect
#define shmem_int16_collect pshmem_int16_collect
#pragma weak shmem_int32_collect = pshmem_int32_collect
#define shmem_int32_collect pshmem_int32_collect
#pragma weak shmem_int64_collect = pshmem_int64_collect
#define shmem_int64_collect pshmem_int64_collect
#pragma weak shmem_uint8_collect = pshmem_uint8_collect
#define shmem_uint8_collect pshmem_uint8_collect
#pragma weak shmem_uint16_collect = pshmem_uint16_collect
#define shmem_uint16_collect pshmem_uint16_collect
#pragma weak shmem_uint32_collect = pshmem_uint32_collect
#define shmem_uint32_collect pshmem_uint32_collect
#pragma weak shmem_uint64_collect = pshmem_uint64_collect
#define shmem_uint64_collect pshmem_uint64_collect
#pragma weak shmem_size_collect = pshmem_size_collect
#define shmem_size_collect pshmem_size_collect
#pragma weak shmem_ptrdiff_collect = pshmem_ptrdiff_collect
#define shmem_ptrdiff_collect pshmem_ptrdiff_collect
#endif /* ENABLE_PSHMEM */

#define SHIM_COLLECT_DECLARE(_typename, _type, _algo)                          \
  int shmem_##_typename##_collect(shmem_team_t team, _type *dest,              \
                                  const _type *source, size_t nelems) {        \
    shcoll_##_typename##_collect_##_algo(team, dest, source, nelems);          \
  }

#define SHIM_COLLECT_TYPE(_algo)                                               \
  SHIM_COLLECT_DECLARE(float, float, _algo)                                    \
  SHIM_COLLECT_DECLARE(double, double, _algo)                                  \
  SHIM_COLLECT_DECLARE(longdouble, long double, _algo)                         \
  SHIM_COLLECT_DECLARE(char, char, _algo)                                      \
  SHIM_COLLECT_DECLARE(schar, signed char, _algo)                              \
  SHIM_COLLECT_DECLARE(short, short, _algo)                                    \
  SHIM_COLLECT_DECLARE(int, int, _algo)                                        \
  SHIM_COLLECT_DECLARE(long, long, _algo)                                      \
  SHIM_COLLECT_DECLARE(longlong, long long, _algo)                             \
  SHIM_COLLECT_DECLARE(uchar, unsigned char, _algo)                            \
  SHIM_COLLECT_DECLARE(ushort, unsigned short, _algo)                          \
  SHIM_COLLECT_DECLARE(uint, unsigned int, _algo)                              \
  SHIM_COLLECT_DECLARE(ulong, unsigned long, _algo)                            \
  SHIM_COLLECT_DECLARE(ulonglong, unsigned long long, _algo)                   \
  SHIM_COLLECT_DECLARE(int8, int8_t, _algo)                                    \
  SHIM_COLLECT_DECLARE(int16, int16_t, _algo)                                  \
  SHIM_COLLECT_DECLARE(int32, int32_t, _algo)                                  \
  SHIM_COLLECT_DECLARE(int64, int64_t, _algo)                                  \
  SHIM_COLLECT_DECLARE(uint8, uint8_t, _algo)                                  \
  SHIM_COLLECT_DECLARE(uint16, uint16_t, _algo)                                \
  SHIM_COLLECT_DECLARE(uint32, uint32_t, _algo)                                \
  SHIM_COLLECT_DECLARE(uint64, uint64_t, _algo)                                \
  SHIM_COLLECT_DECLARE(size, size_t, _algo)                                    \
  SHIM_COLLECT_DECLARE(ptrdiff, ptrdiff_t, _algo)

/****************************************************************************/

#ifdef ENABLE_PSHMEM
#pragma weak shmem_float_fcollect = pshmem_float_fcollect
#define shmem_float_fcollect pshmem_float_fcollect
#pragma weak shmem_double_fcollect = pshmem_double_fcollect
#define shmem_double_fcollect pshmem_double_fcollect
#pragma weak shmem_longdouble_fcollect = pshmem_longdouble_fcollect
#define shmem_longdouble_fcollect pshmem_longdouble_fcollect
#pragma weak shmem_char_fcollect = pshmem_char_fcollect
#define shmem_char_fcollect pshmem_char_fcollect
#pragma weak shmem_schar_fcollect = pshmem_schar_fcollect
#define shmem_schar_fcollect pshmem_schar_fcollect
#pragma weak shmem_short_fcollect = pshmem_short_fcollect
#define shmem_short_fcollect pshmem_short_fcollect
#pragma weak shmem_int_fcollect = pshmem_int_fcollect
#define shmem_int_fcollect pshmem_int_fcollect
#pragma weak shmem_long_fcollect = pshmem_long_fcollect
#define shmem_long_fcollect pshmem_long_fcollect
#pragma weak shmem_longlong_fcollect = pshmem_longlong_fcollect
#define shmem_longlong_fcollect pshmem_longlong_fcollect
#pragma weak shmem_uchar_fcollect = pshmem_uchar_fcollect
#define shmem_uchar_fcollect pshmem_uchar_fcollect
#pragma weak shmem_ushort_fcollect = pshmem_ushort_fcollect
#define shmem_ushort_fcollect pshmem_ushort_fcollect
#pragma weak shmem_uint_fcollect = pshmem_uint_fcollect
#define shmem_uint_fcollect pshmem_uint_fcollect
#pragma weak shmem_ulong_fcollect = pshmem_ulong_fcollect
#define shmem_ulong_fcollect pshmem_ulong_fcollect
#pragma weak shmem_ulonglong_fcollect = pshmem_ulonglong_fcollect
#define shmem_ulonglong_fcollect pshmem_ulonglong_fcollect
#pragma weak shmem_int8_fcollect = pshmem_int8_fcollect
#define shmem_int8_fcollect pshmem_int8_fcollect
#pragma weak shmem_int16_fcollect = pshmem_int16_fcollect
#define shmem_int16_fcollect pshmem_int16_fcollect
#pragma weak shmem_int32_fcollect = pshmem_int32_fcollect
#define shmem_int32_fcollect pshmem_int32_fcollect
#pragma weak shmem_int64_fcollect = pshmem_int64_fcollect
#define shmem_int64_fcollect pshmem_int64_fcollect
#pragma weak shmem_uint8_fcollect = pshmem_uint8_fcollect
#define shmem_uint8_fcollect pshmem_uint8_fcollect
#pragma weak shmem_uint16_fcollect = pshmem_uint16_fcollect
#define shmem_uint16_fcollect pshmem_uint16_fcollect
#pragma weak shmem_uint32_fcollect = pshmem_uint32_fcollect
#define shmem_uint32_fcollect pshmem_uint32_fcollect
#pragma weak shmem_uint64_fcollect = pshmem_uint64_fcollect
#define shmem_uint64_fcollect pshmem_uint64_fcollect
#pragma weak shmem_size_fcollect = pshmem_size_fcollect
#define shmem_size_fcollect pshmem_size_fcollect
#pragma weak shmem_ptrdiff_fcollect = pshmem_ptrdiff_fcollect
#define shmem_ptrdiff_fcollect pshmem_ptrdiff_fcollect
#endif /* ENABLE_PSHMEM */

#define SHIM_FCOLLECT_DECLARE(_typename, _type, _algo)                         \
  int shmem_##_typename##_fcollect(shmem_team_t team, _type *dest,             \
                                   const _type *source, size_t nelems) {       \
    shcoll_##_typename##_fcollect_##_algo(team, dest, source, nelems);         \
  }

#define SHIM_FCOLLECT_TYPE(_algo)                                              \
  SHIM_FCOLLECT_DECLARE(float, float, _algo)                                   \
  SHIM_FCOLLECT_DECLARE(double, double, _algo)                                 \
  SHIM_FCOLLECT_DECLARE(longdouble, long double, _algo)                        \
  SHIM_FCOLLECT_DECLARE(char, char, _algo)                                     \
  SHIM_FCOLLECT_DECLARE(schar, signed char, _algo)                             \
  SHIM_FCOLLECT_DECLARE(short, short, _algo)                                   \
  SHIM_FCOLLECT_DECLARE(int, int, _algo)                                       \
  SHIM_FCOLLECT_DECLARE(long, long, _algo)                                     \
  SHIM_FCOLLECT_DECLARE(longlong, long long, _algo)                            \
  SHIM_FCOLLECT_DECLARE(uchar, unsigned char, _algo)                           \
  SHIM_FCOLLECT_DECLARE(ushort, unsigned short, _algo)                         \
  SHIM_FCOLLECT_DECLARE(uint, unsigned int, _algo)                             \
  SHIM_FCOLLECT_DECLARE(ulong, unsigned long, _algo)                           \
  SHIM_FCOLLECT_DECLARE(ulonglong, unsigned long long, _algo)                  \
  SHIM_FCOLLECT_DECLARE(int8, int8_t, _algo)                                   \
  SHIM_FCOLLECT_DECLARE(int16, int16_t, _algo)                                 \
  SHIM_FCOLLECT_DECLARE(int32, int32_t, _algo)                                 \
  SHIM_FCOLLECT_DECLARE(int64, int64_t, _algo)                                 \
  SHIM_FCOLLECT_DECLARE(uint8, uint8_t, _algo)                                 \
  SHIM_FCOLLECT_DECLARE(uint16, uint16_t, _algo)                               \
  SHIM_FCOLLECT_DECLARE(uint32, uint32_t, _algo)                               \
  SHIM_FCOLLECT_DECLARE(uint64, uint64_t, _algo)                               \
  SHIM_FCOLLECT_DECLARE(size, size_t, _algo)                                   \
  SHIM_FCOLLECT_DECLARE(ptrdiff, ptrdiff_t, _algo)

/****************************************************************************/

#ifdef ENABLE_PSHMEM
#pragma weak shmem_float_broadcast = pshmem_float_broadcast
#define shmem_float_broadcast pshmem_float_broadcast
#pragma weak shmem_double_broadcast = pshmem_double_broadcast
#define shmem_double_broadcast pshmem_double_broadcast
#pragma weak shmem_longdouble_broadcast = pshmem_longdouble_broadcast
#define shmem_longdouble_broadcast pshmem_longdouble_broadcast
#pragma weak shmem_char_broadcast = pshmem_char_broadcast
#define shmem_char_broadcast pshmem_char_broadcast
#pragma weak shmem_schar_broadcast = pshmem_schar_broadcast
#define shmem_schar_broadcast pshmem_schar_broadcast
#pragma weak shmem_short_broadcast = pshmem_short_broadcast
#define shmem_short_broadcast pshmem_short_broadcast
#pragma weak shmem_int_broadcast = pshmem_int_broadcast
#define shmem_int_broadcast pshmem_int_broadcast
#pragma weak shmem_long_broadcast = pshmem_long_broadcast
#define shmem_long_broadcast pshmem_long_broadcast
#pragma weak shmem_longlong_broadcast = pshmem_longlong_broadcast
#define shmem_longlong_broadcast pshmem_longlong_broadcast
#pragma weak shmem_uchar_broadcast = pshmem_uchar_broadcast
#define shmem_uchar_broadcast pshmem_uchar_broadcast
#pragma weak shmem_ushort_broadcast = pshmem_ushort_broadcast
#define shmem_ushort_broadcast pshmem_ushort_broadcast
#pragma weak shmem_uint_broadcast = pshmem_uint_broadcast
#define shmem_uint_broadcast pshmem_uint_broadcast
#pragma weak shmem_ulong_broadcast = pshmem_ulong_broadcast
#define shmem_ulong_broadcast pshmem_ulong_broadcast
#pragma weak shmem_ulonglong_broadcast = pshmem_ulonglong_broadcast
#define shmem_ulonglong_broadcast pshmem_ulonglong_broadcast
#pragma weak shmem_int8_broadcast = pshmem_int8_broadcast
#define shmem_int8_broadcast pshmem_int8_broadcast
#pragma weak shmem_int16_broadcast = pshmem_int16_broadcast
#define shmem_int16_broadcast pshmem_int16_broadcast
#pragma weak shmem_int32_broadcast = pshmem_int32_broadcast
#define shmem_int32_broadcast pshmem_int32_broadcast
#pragma weak shmem_int64_broadcast = pshmem_int64_broadcast
#define shmem_int64_broadcast pshmem_int64_broadcast
#pragma weak shmem_uint8_broadcast = pshmem_uint8_broadcast
#define shmem_uint8_broadcast pshmem_uint8_broadcast
#pragma weak shmem_uint16_broadcast = pshmem_uint16_broadcast
#define shmem_uint16_broadcast pshmem_uint16_broadcast
#pragma weak shmem_uint32_broadcast = pshmem_uint32_broadcast
#define shmem_uint32_broadcast pshmem_uint32_broadcast
#pragma weak shmem_uint64_broadcast = pshmem_uint64_broadcast
#define shmem_uint64_broadcast pshmem_uint64_broadcast
#pragma weak shmem_size_broadcast = pshmem_size_broadcast
#define shmem_size_broadcast pshmem_size_broadcast
#pragma weak shmem_ptrdiff_broadcast = pshmem_ptrdiff_broadcast
#define shmem_ptrdiff_broadcast pshmem_ptrdiff_broadcast
#endif /* ENABLE_PSHMEM */

#define SHIM_BROADCAST_DECLARE(_typename, _type, _algo)                        \
  int shmem_##_typename##_broadcast(shmem_team_t team, _type *dest,            \
                                    const _type *source, size_t nelems,        \
                                    int PE_root) {                             \
    shcoll_##_typename##_broadcast_##_algo(team, dest, source, nelems,         \
                                           PE_root);                           \
  }

#define SHIM_BROADCAST_TYPE(_algo)                                             \
  SHIM_BROADCAST_DECLARE(float, float, _algo)                                  \
  SHIM_BROADCAST_DECLARE(double, double, _algo)                                \
  SHIM_BROADCAST_DECLARE(longdouble, long double, _algo)                       \
  SHIM_BROADCAST_DECLARE(char, char, _algo)                                    \
  SHIM_BROADCAST_DECLARE(schar, signed char, _algo)                            \
  SHIM_BROADCAST_DECLARE(short, short, _algo)                                  \
  SHIM_BROADCAST_DECLARE(int, int, _algo)                                      \
  SHIM_BROADCAST_DECLARE(long, long, _algo)                                    \
  SHIM_BROADCAST_DECLARE(longlong, long long, _algo)                           \
  SHIM_BROADCAST_DECLARE(uchar, unsigned char, _algo)                          \
  SHIM_BROADCAST_DECLARE(ushort, unsigned short, _algo)                        \
  SHIM_BROADCAST_DECLARE(uint, unsigned int, _algo)                            \
  SHIM_BROADCAST_DECLARE(ulong, unsigned long, _algo)                          \
  SHIM_BROADCAST_DECLARE(ulonglong, unsigned long long, _algo)                 \
  SHIM_BROADCAST_DECLARE(int8, int8_t, _algo)                                  \
  SHIM_BROADCAST_DECLARE(int16, int16_t, _algo)                                \
  SHIM_BROADCAST_DECLARE(int32, int32_t, _algo)                                \
  SHIM_BROADCAST_DECLARE(int64, int64_t, _algo)                                \
  SHIM_BROADCAST_DECLARE(uint8, uint8_t, _algo)                                \
  SHIM_BROADCAST_DECLARE(uint16, uint16_t, _algo)                              \
  SHIM_BROADCAST_DECLARE(uint32, uint32_t, _algo)                              \
  SHIM_BROADCAST_DECLARE(uint64, uint64_t, _algo)                              \
  SHIM_BROADCAST_DECLARE(size, size_t, _algo)                                  \
  SHIM_BROADCAST_DECLARE(ptrdiff, ptrdiff_t, _algo)

/****************************************************************************/

#endif /* _TYPED_H */
