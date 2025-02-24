/**
 * @file reductions.h
 * @brief Header file for OpenSHMEM reduction operations
 * @copyright For license: see LICENSE file at top-level
 *
 * This file contains declarations and macros for implementing OpenSHMEM
 * reduction operations, including both active set-based and team-based
 * reductions. It provides support for various reduction operations (sum,
 * product, min, max, and bitwise operations) across multiple data types.
 */

#ifndef _REDUCTIONS_H
#define _REDUCTIONS_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <shcoll.h>
#include <shmem/teams.h>

#include <shmem.h>

/**
 * @defgroup active_set_reductions Active Set-Based Reductions
 * @brief Reduction operations using active sets of processing elements
 * @{
 */

#ifdef ENABLE_PSHMEM
#pragma weak shmem_complexd_sum_to_all = pshmem_complexd_sum_to_all
#define shmem_complexd_sum_to_all pshmem_complexd_sum_to_all
#pragma weak shmem_complexf_sum_to_all = pshmem_complexf_sum_to_all
#define shmem_complexf_sum_to_all pshmem_complexf_sum_to_all
#pragma weak shmem_double_sum_to_all = pshmem_double_sum_to_all
#define shmem_double_sum_to_all pshmem_double_sum_to_all
#pragma weak shmem_float_sum_to_all = pshmem_float_sum_to_all
#define shmem_float_sum_to_all pshmem_float_sum_to_all
#pragma weak shmem_int_sum_to_all = pshmem_int_sum_to_all
#define shmem_int_sum_to_all pshmem_int_sum_to_all
#pragma weak shmem_long_sum_to_all = pshmem_long_sum_to_all
#define shmem_long_sum_to_all pshmem_long_sum_to_all
#pragma weak shmem_longdouble_sum_to_all = pshmem_longdouble_sum_to_all
#define shmem_longdouble_sum_to_all pshmem_longdouble_sum_to_all
#pragma weak shmem_longlong_sum_to_all = pshmem_longlong_sum_to_all
#define shmem_longlong_sum_to_all pshmem_longlong_sum_to_all
#pragma weak shmem_short_sum_to_all = pshmem_short_sum_to_all
#define shmem_short_sum_to_all pshmem_short_sum_to_all
#pragma weak shmem_complexd_prod_to_all = pshmem_complexd_prod_to_all
#define shmem_complexd_prod_to_all pshmem_complexd_prod_to_all
#pragma weak shmem_complexf_prod_to_all = pshmem_complexf_prod_to_all
#define shmem_complexf_prod_to_all pshmem_complexf_prod_to_all
#pragma weak shmem_double_prod_to_all = pshmem_double_prod_to_all
#define shmem_double_prod_to_all pshmem_double_prod_to_all
#pragma weak shmem_float_prod_to_all = pshmem_float_prod_to_all
#define shmem_float_prod_to_all pshmem_float_prod_to_all
#pragma weak shmem_int_prod_to_all = pshmem_int_prod_to_all
#define shmem_int_prod_to_all pshmem_int_prod_to_all
#pragma weak shmem_long_prod_to_all = pshmem_long_prod_to_all
#define shmem_long_prod_to_all pshmem_long_prod_to_all
#pragma weak shmem_longdouble_prod_to_all = pshmem_longdouble_prod_to_all
#define shmem_longdouble_prod_to_all pshmem_longdouble_prod_to_all
#pragma weak shmem_longlong_prod_to_all = pshmem_longlong_prod_to_all
#define shmem_longlong_prod_to_all pshmem_longlong_prod_to_all
#pragma weak shmem_short_prod_to_all = pshmem_short_prod_to_all
#define shmem_short_prod_to_all pshmem_short_prod_to_all
#pragma weak shmem_int_and_to_all = pshmem_int_and_to_all
#define shmem_int_and_to_all pshmem_int_and_to_all
#pragma weak shmem_long_and_to_all = pshmem_long_and_to_all
#define shmem_long_and_to_all pshmem_long_and_to_all
#pragma weak shmem_longlong_and_to_all = pshmem_longlong_and_to_all
#define shmem_longlong_and_to_all pshmem_longlong_and_to_all
#pragma weak shmem_short_and_to_all = pshmem_short_and_to_all
#define shmem_short_and_to_all pshmem_short_and_to_all
#pragma weak shmem_int_or_to_all = pshmem_int_or_to_all
#define shmem_int_or_to_all pshmem_int_or_to_all
#pragma weak shmem_long_or_to_all = pshmem_long_or_to_all
#define shmem_long_or_to_all pshmem_long_or_to_all
#pragma weak shmem_longlong_or_to_all = pshmem_longlong_or_to_all
#define shmem_longlong_or_to_all pshmem_longlong_or_to_all
#pragma weak shmem_short_or_to_all = pshmem_short_or_to_all
#define shmem_short_or_to_all pshmem_short_or_to_all
#pragma weak shmem_int_xor_to_all = pshmem_int_xor_to_all
#define shmem_int_xor_to_all pshmem_int_xor_to_all
#pragma weak shmem_long_xor_to_all = pshmem_long_xor_to_all
#define shmem_long_xor_to_all pshmem_long_xor_to_all
#pragma weak shmem_longlong_xor_to_all = pshmem_longlong_xor_to_all
#define shmem_longlong_xor_to_all pshmem_longlong_xor_to_all
#pragma weak shmem_short_xor_to_all = pshmem_short_xor_to_all
#define shmem_short_xor_to_all pshmem_short_xor_to_all
#pragma weak shmem_int_max_to_all = pshmem_int_max_to_all
#define shmem_int_max_to_all pshmem_int_max_to_all
#pragma weak shmem_long_max_to_all = pshmem_long_max_to_all
#define shmem_long_max_to_all pshmem_long_max_to_all
#pragma weak shmem_longlong_max_to_all = pshmem_longlong_max_to_all
#define shmem_longlong_max_to_all pshmem_longlong_max_to_all
#pragma weak shmem_short_max_to_all = pshmem_short_max_to_all
#define shmem_short_max_to_all pshmem_short_max_to_all
#pragma weak shmem_longdouble_max_to_all = pshmem_longdouble_max_to_all
#define shmem_longdouble_max_to_all pshmem_longdouble_max_to_all
#pragma weak shmem_float_max_to_all = pshmem_float_max_to_all
#define shmem_float_max_to_all pshmem_float_max_to_all
#pragma weak shmem_double_max_to_all = pshmem_double_max_to_all
#define shmem_double_max_to_all pshmem_double_max_to_all
#pragma weak shmem_int_min_to_all = pshmem_int_min_to_all
#define shmem_int_min_to_all pshmem_int_min_to_all
#pragma weak shmem_long_min_to_all = pshmem_long_min_to_all
#define shmem_long_min_to_all pshmem_long_min_to_all
#pragma weak shmem_longlong_min_to_all = pshmem_longlong_min_to_all
#define shmem_longlong_min_to_all pshmem_longlong_min_to_all
#pragma weak shmem_short_min_to_all = pshmem_short_min_to_all
#define shmem_short_min_to_all pshmem_short_min_to_all
#pragma weak shmem_longdouble_min_to_all = pshmem_longdouble_min_to_all
#define shmem_longdouble_min_to_all pshmem_longdouble_min_to_all
#pragma weak shmem_float_min_to_all = pshmem_float_min_to_all
#define shmem_float_min_to_all pshmem_float_min_to_all
#pragma weak shmem_double_min_to_all = pshmem_double_min_to_all
#define shmem_double_min_to_all pshmem_double_min_to_all
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to declare reduction operation for active set-based reductions
 *
 * @param _typename Type name suffix
 * @param _type Actual C data type
 * @param _op Reduction operation (sum, prod, and, or, xor, max, min)
 * @param _algo Algorithm implementation suffix
 */
#define SHIM_TO_ALL_DECLARE(_typename, _type, _op, _algo)                      \
  void shmem_##_typename##_##_op##_to_all(                                     \
      _type *dest, const _type *source, int nreduce, int PE_start,             \
      int logPE_stride, int PE_size, _type *pWrk, long *pSync) {               \
    shcoll_##_typename##_##_op##_to_all_##_algo(                               \
        dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);  \
  }

/**
 * @brief Macro to declare bitwise reduction operations for basic integer types
 *
 * @param _op Reduction operation (and, or, xor)
 * @param _algo Algorithm implementation suffix
 */
#define SHIM_TO_ALL_BITWISE_TYPES(_op, _algo)                                  \
  SHIM_TO_ALL_DECLARE(short, short, _op, _algo)                                \
  SHIM_TO_ALL_DECLARE(int, int, _op, _algo)                                    \
  SHIM_TO_ALL_DECLARE(long, long, _op, _algo)                                  \
  SHIM_TO_ALL_DECLARE(longlong, long long, _op, _algo)

/**
 * @brief Macro to declare min/max reduction operations for numeric types
 *
 * @param _op Reduction operation (min, max)
 * @param _algo Algorithm implementation suffix
 */
#define SHIM_TO_ALL_MINMAX_TYPES(_op, _algo)                                   \
  SHIM_TO_ALL_BITWISE_TYPES(_op, _algo)                                        \
  SHIM_TO_ALL_DECLARE(double, double, _op, _algo)                              \
  SHIM_TO_ALL_DECLARE(float, float, _op, _algo)                                \
  SHIM_TO_ALL_DECLARE(longdouble, long double, _op, _algo)

/**
 * @brief Macro to declare arithmetic reduction operations for all numeric types
 *
 * @param _op Reduction operation (sum, prod)
 * @param _algo Algorithm implementation suffix
 */
#define SHIM_TO_ALL_ARITH_TYPES(_op, _algo)                                    \
  SHIM_TO_ALL_MINMAX_TYPES(_op, _algo)                                         \
  SHIM_TO_ALL_DECLARE(complexd, double _Complex, _op, _algo)                   \
  SHIM_TO_ALL_DECLARE(complexf, float _Complex, _op, _algo)

/**
 * @brief Macro to declare all bitwise reduction operations
 *
 * @param _algo Algorithm implementation suffix
 */
#define SHIM_TO_ALL_BITWISE_ALL(_algo)                                         \
  SHIM_TO_ALL_BITWISE_TYPES(or, _algo)                                         \
  SHIM_TO_ALL_BITWISE_TYPES(xor, _algo)                                        \
  SHIM_TO_ALL_BITWISE_TYPES(and, _algo)

/**
 * @brief Macro to declare all min/max reduction operations
 *
 * @param _algo Algorithm implementation suffix
 */
#define SHIM_TO_ALL_MINMAX_ALL(_algo)                                          \
  SHIM_TO_ALL_MINMAX_TYPES(min, _algo)                                         \
  SHIM_TO_ALL_MINMAX_TYPES(max, _algo)

/**
 * @brief Macro to declare all arithmetic reduction operations
 *
 * @param _algo Algorithm implementation suffix
 */
#define SHIM_TO_ALL_ARITH_ALL(_algo)                                           \
  SHIM_TO_ALL_ARITH_TYPES(sum, _algo)                                          \
  SHIM_TO_ALL_ARITH_TYPES(prod, _algo)

/**
 * @brief Macro to declare all reduction operations
 *
 * @param _algo Algorithm implementation suffix
 */
#define SHIM_TO_ALL_ALL(_algo)                                                 \
  SHIM_TO_ALL_BITWISE_ALL(_algo)                                               \
  SHIM_TO_ALL_MINMAX_ALL(_algo)                                                \
  SHIM_TO_ALL_ARITH_ALL(_algo)

/** @} */ /* end active set reductions group */

/**
 * @defgroup team_reductions Team-Based Reductions
 * @brief Reduction operations using teams of processing elements
 * @{
 */

#ifdef ENABLE_PSHMEM
/* and */
#pragma weak shmem_uchar_and_reduce = pshmem_uchar_and_reduce
#define shmem_uchar_and_reduce pshmem_uchar_and_reduce
#pragma weak shmem_ushort_and_reduce = pshmem_ushort_and_reduce
#define shmem_ushort_and_reduce pshmem_ushort_and_reduce
#pragma weak shmem_uint_and_reduce = pshmem_uint_and_reduce
#define shmem_uint_and_reduce pshmem_uint_and_reduce
#pragma weak shmem_ulong_and_reduce = pshmem_ulong_and_reduce
#define shmem_ulong_and_reduce pshmem_ulong_and_reduce
#pragma weak shmem_ulonglong_and_reduce = pshmem_ulonglong_and_reduce
#define shmem_ulonglong_and_reduce pshmem_ulonglong_and_reduce
#pragma weak shmem_int8_and_reduce = pshmem_int8_and_reduce
#define shmem_int8_and_reduce pshmem_int8_and_reduce
#pragma weak shmem_int16_and_reduce = pshmem_int16_and_reduce
#define shmem_int16_and_reduce pshmem_int16_and_reduce
#pragma weak shmem_int32_and_reduce = pshmem_int32_and_reduce
#define shmem_int32_and_reduce pshmem_int32_and_reduce
#pragma weak shmem_int64_and_reduce = pshmem_int64_and_reduce
#define shmem_int64_and_reduce pshmem_int64_and_reduce
#pragma weak shmem_uint8_and_reduce = pshmem_uint8_and_reduce
#define shmem_uint8_and_reduce pshmem_uint8_and_reduce
#pragma weak shmem_uint16_and_reduce = pshmem_uint16_and_reduce
#define shmem_uint16_and_reduce pshmem_uint16_and_reduce
#pragma weak shmem_uint32_and_reduce = pshmem_uint32_and_reduce
#define shmem_uint32_and_reduce pshmem_uint32_and_reduce
#pragma weak shmem_uint64_and_reduce = pshmem_uint64_and_reduce
#define shmem_uint64_and_reduce pshmem_uint64_and_reduce
/* or */
#pragma weak shmem_uchar_or_reduce = pshmem_uchar_or_reduce
#define shmem_uchar_or_reduce pshmem_uchar_or_reduce
#pragma weak shmem_ushort_or_reduce = pshmem_ushort_or_reduce
#define shmem_ushort_or_reduce pshmem_ushort_or_reduce
#pragma weak shmem_uint_or_reduce = pshmem_uint_or_reduce
#define shmem_uint_or_reduce pshmem_uint_or_reduce
#pragma weak shmem_ulong_or_reduce = pshmem_ulong_or_reduce
#define shmem_ulong_or_reduce pshmem_ulong_or_reduce
#pragma weak shmem_ulonglong_or_reduce = pshmem_ulonglong_or_reduce
#define shmem_ulonglong_or_reduce pshmem_ulonglong_or_reduce
#pragma weak shmem_int8_or_reduce = pshmem_int8_or_reduce
#define shmem_int8_or_reduce pshmem_int8_or_reduce
#pragma weak shmem_int16_or_reduce = pshmem_int16_or_reduce
#define shmem_int16_or_reduce pshmem_int16_or_reduce
#pragma weak shmem_int32_or_reduce = pshmem_int32_or_reduce
#define shmem_int32_or_reduce pshmem_int32_or_reduce
#pragma weak shmem_int64_or_reduce = pshmem_int64_or_reduce
#define shmem_int64_or_reduce pshmem_int64_or_reduce
#pragma weak shmem_uint8_or_reduce = pshmem_uint8_or_reduce
#define shmem_uint8_or_reduce pshmem_uint8_or_reduce
#pragma weak shmem_uint16_or_reduce = pshmem_uint16_or_reduce
#define shmem_uint16_or_reduce pshmem_uint16_or_reduce
#pragma weak shmem_uint32_or_reduce = pshmem_uint32_or_reduce
#define shmem_uint32_or_reduce pshmem_uint32_or_reduce
#pragma weak shmem_uint64_or_reduce = pshmem_uint64_or_reduce
#define shmem_uint64_or_reduce pshmem_uint64_or_reduce
#pragma weak shmem_size_or_reduce = pshmem_size_or_reduce
#define shmem_size_or_reduce pshmem_size_or_reduce
/* xor */
#pragma weak shmem_uchar_xor_reduce = pshmem_uchar_xor_reduce
#define shmem_uchar_xor_reduce pshmem_uchar_xor_reduce
#pragma weak shmem_ushort_xor_reduce = pshmem_ushort_xor_reduce
#define shmem_ushort_xor_reduce pshmem_ushort_xor_reduce
#pragma weak shmem_uint_xor_reduce = pshmem_uint_xor_reduce
#define shmem_uint_xor_reduce pshmem_uint_xor_reduce
#pragma weak shmem_ulong_xor_reduce = pshmem_ulong_xor_reduce
#define shmem_ulong_xor_reduce pshmem_ulong_xor_reduce
#pragma weak shmem_ulonglong_xor_reduce = pshmem_ulonglong_xor_reduce
#define shmem_ulonglong_xor_reduce pshmem_ulonglong_xor_reduce
#pragma weak shmem_int8_xor_reduce = pshmem_int8_xor_reduce
#define shmem_int8_xor_reduce pshmem_int8_xor_reduce
#pragma weak shmem_int16_xor_reduce = pshmem_int16_xor_reduce
#define shmem_int16_xor_reduce pshmem_int16_xor_reduce
#pragma weak shmem_int32_xor_reduce = pshmem_int32_xor_reduce
#define shmem_int32_xor_reduce pshmem_int32_xor_reduce
#pragma weak shmem_int64_xor_reduce = pshmem_int64_xor_reduce
#define shmem_int64_xor_reduce pshmem_int64_xor_reduce
#pragma weak shmem_uint8_xor_reduce = pshmem_uint8_xor_reduce
#define shmem_uint8_xor_reduce pshmem_uint8_xor_reduce
#pragma weak shmem_uint16_xor_reduce = pshmem_uint16_xor_reduce
#define shmem_uint16_xor_reduce pshmem_uint16_xor_reduce
#pragma weak shmem_uint32_xor_reduce = pshmem_uint32_xor_reduce
#define shmem_uint32_xor_reduce pshmem_uint32_xor_reduce
#pragma weak shmem_uint64_xor_reduce = pshmem_uint64_xor_reduce
#define shmem_uint64_xor_reduce pshmem_uint64_xor_reduce
#pragma weak shmem_size_xor_reduce = pshmem_size_xor_reduce
#define shmem_size_xor_reduce pshmem_size_xor_reduce
/* max */
#pragma weak shmem_char_max_reduce = pshmem_char_max_reduce
#define shmem_char_max_reduce pshmem_char_max_reduce
#pragma weak shmem_schar_max_reduce = pshmem_schar_max_reduce
#define shmem_schar_max_reduce pshmem_schar_max_reduce
#pragma weak shmem_short_max_reduce = pshmem_short_max_reduce
#define shmem_short_max_reduce pshmem_short_max_reduce
#pragma weak shmem_int_max_reduce = pshmem_int_max_reduce
#define shmem_int_max_reduce pshmem_int_max_reduce
#pragma weak shmem_long_max_reduce = pshmem_long_max_reduce
#define shmem_long_max_reduce pshmem_long_max_reduce
#pragma weak shmem_longlong_max_reduce = pshmem_longlong_max_reduce
#define shmem_longlong_max_reduce pshmem_longlong_max_reduce
#pragma weak shmem_ptrdiff_max_reduce = pshmem_ptrdiff_max_reduce
#define shmem_ptrdiff_max_reduce pshmem_ptrdiff_max_reduce
#pragma weak shmem_uchar_max_reduce = pshmem_uchar_max_reduce
#define shmem_uchar_max_reduce pshmem_uchar_max_reduce
#pragma weak shmem_ushort_max_reduce = pshmem_ushort_max_reduce
#define shmem_ushort_max_reduce pshmem_ushort_max_reduce
#pragma weak shmem_uint_max_reduce = pshmem_uint_max_reduce
#define shmem_uint_max_reduce pshmem_uint_max_reduce
#pragma weak shmem_ulong_max_reduce = pshmem_ulong_max_reduce
#define shmem_ulong_max_reduce pshmem_ulong_max_reduce
#pragma weak shmem_ulonglong_max_reduce = pshmem_ulonglong_max_reduce
#define shmem_ulonglong_max_reduce pshmem_ulonglong_max_reduce
#pragma weak shmem_int8_max_reduce = pshmem_int8_max_reduce
#define shmem_int8_max_reduce pshmem_int8_max_reduce
#pragma weak shmem_int16_max_reduce = pshmem_int16_max_reduce
#define shmem_int16_max_reduce pshmem_int16_max_reduce
#pragma weak shmem_int32_max_reduce = pshmem_int32_max_reduce
#define shmem_int32_max_reduce pshmem_int32_max_reduce
#pragma weak shmem_int64_max_reduce = pshmem_int64_max_reduce
#define shmem_int64_max_reduce pshmem_int64_max_reduce
#pragma weak shmem_uint8_max_reduce = pshmem_uint8_max_reduce
#define shmem_uint8_max_reduce pshmem_uint8_max_reduce
#pragma weak shmem_uint16_max_reduce = pshmem_uint16_max_reduce
#define shmem_uint16_max_reduce pshmem_uint16_max_reduce
#pragma weak shmem_uint32_max_reduce = pshmem_uint32_max_reduce
#define shmem_uint32_max_reduce pshmem_uint32_max_reduce
#pragma weak shmem_uint64_max_reduce = pshmem_uint64_max_reduce
#define shmem_uint64_max_reduce pshmem_uint64_max_reduce
#pragma weak shmem_size_max_reduce = pshmem_size_max_reduce
#define shmem_size_max_reduce pshmem_size_max_reduce
#pragma weak shmem_float_max_reduce = pshmem_float_max_reduce
#define shmem_float_max_reduce pshmem_float_max_reduce
#pragma weak shmem_double_max_reduce = pshmem_double_max_reduce
#define shmem_double_max_reduce pshmem_double_max_reduce
#pragma weak shmem_longdouble_max_reduce = pshmem_longdouble_max_reduce
#define shmem_longdouble_max_reduce pshmem_longdouble_max_reduce
/* min */
#pragma weak shmem_char_min_reduce = pshmem_char_min_reduce
#define shmem_char_min_reduce pshmem_char_min_reduce
#pragma weak shmem_schar_min_reduce = pshmem_schar_min_reduce
#define shmem_schar_min_reduce pshmem_schar_min_reduce
#pragma weak shmem_short_min_reduce = pshmem_short_min_reduce
#define shmem_short_min_reduce pshmem_short_min_reduce
#pragma weak shmem_int_min_reduce = pshmem_int_min_reduce
#define shmem_int_min_reduce pshmem_int_min_reduce
#pragma weak shmem_long_min_reduce = pshmem_long_min_reduce
#define shmem_long_min_reduce pshmem_long_min_reduce
#pragma weak shmem_longlong_min_reduce = pshmem_longlong_min_reduce
#define shmem_longlong_min_reduce pshmem_longlong_min_reduce
#pragma weak shmem_ptrdiff_min_reduce = pshmem_ptrdiff_min_reduce
#define shmem_ptrdiff_min_reduce pshmem_ptrdiff_min_reduce
#pragma weak shmem_uchar_min_reduce = pshmem_uchar_min_reduce
#define shmem_uchar_min_reduce pshmem_uchar_min_reduce
#pragma weak shmem_ushort_min_reduce = pshmem_ushort_min_reduce
#define shmem_ushort_min_reduce pshmem_ushort_min_reduce
#pragma weak shmem_uint_min_reduce = pshmem_uint_min_reduce
#define shmem_uint_min_reduce pshmem_uint_min_reduce
#pragma weak shmem_ulong_min_reduce = pshmem_ulong_min_reduce
#define shmem_ulong_min_reduce pshmem_ulong_min_reduce
#pragma weak shmem_ulonglong_min_reduce = pshmem_ulonglong_min_reduce
#define shmem_ulonglong_min_reduce pshmem_ulonglong_min_reduce
#pragma weak shmem_int8_min_reduce = pshmem_int8_min_reduce
#define shmem_int8_min_reduce pshmem_int8_min_reduce
#pragma weak shmem_int16_min_reduce = pshmem_int16_min_reduce
#define shmem_int16_min_reduce pshmem_int16_min_reduce
#pragma weak shmem_int32_min_reduce = pshmem_int32_min_reduce
#define shmem_int32_min_reduce pshmem_int32_min_reduce
#pragma weak shmem_int64_min_reduce = pshmem_int64_min_reduce
#define shmem_int64_min_reduce pshmem_int64_min_reduce
#pragma weak shmem_uint8_min_reduce = pshmem_uint8_min_reduce
#define shmem_uint8_min_reduce pshmem_uint8_min_reduce
#pragma weak shmem_uint16_min_reduce = pshmem_uint16_min_reduce
#define shmem_uint16_min_reduce pshmem_uint16_min_reduce
#pragma weak shmem_uint32_min_reduce = pshmem_uint32_min_reduce
#define shmem_uint32_min_reduce pshmem_uint32_min_reduce
#pragma weak shmem_uint64_min_reduce = pshmem_uint64_min_reduce
#define shmem_uint64_min_reduce pshmem_uint64_min_reduce
#pragma weak shmem_size_min_reduce = pshmem_size_min_reduce
#define shmem_size_min_reduce pshmem_size_min_reduce
#pragma weak shmem_float_min_reduce = pshmem_float_min_reduce
#define shmem_float_min_reduce pshmem_float_min_reduce
#pragma weak shmem_double_min_reduce = pshmem_double_min_reduce
#define shmem_double_min_reduce pshmem_double_min_reduce
#pragma weak shmem_longdouble_min_reduce = pshmem_longdouble_min_reduce
#define shmem_longdouble_min_reduce pshmem_longdouble_min_reduce
/* sum */
#pragma weak shmem_char_sum_reduce = pshmem_char_sum_reduce
#define shmem_char_sum_reduce pshmem_char_sum_reduce
#pragma weak shmem_schar_sum_reduce = pshmem_schar_sum_reduce
#define shmem_schar_sum_reduce pshmem_schar_sum_reduce
#pragma weak shmem_short_sum_reduce = pshmem_short_sum_reduce
#define shmem_short_sum_reduce pshmem_short_sum_reduce
#pragma weak shmem_int_sum_reduce = pshmem_int_sum_reduce
#define shmem_int_sum_reduce pshmem_int_sum_reduce
#pragma weak shmem_long_sum_reduce = pshmem_long_sum_reduce
#define shmem_long_sum_reduce pshmem_long_sum_reduce
#pragma weak shmem_longlong_sum_reduce = pshmem_longlong_sum_reduce
#define shmem_longlong_sum_reduce pshmem_longlong_sum_reduce
#pragma weak shmem_ptrdiff_sum_reduce = pshmem_ptrdiff_sum_reduce
#define shmem_ptrdiff_sum_reduce pshmem_ptrdiff_sum_reduce
#pragma weak shmem_uchar_sum_reduce = pshmem_uchar_sum_reduce
#define shmem_uchar_sum_reduce pshmem_uchar_sum_reduce
#pragma weak shmem_ushort_sum_reduce = pshmem_ushort_sum_reduce
#define shmem_ushort_sum_reduce pshmem_ushort_sum_reduce
#pragma weak shmem_uint_sum_reduce = pshmem_uint_sum_reduce
#define shmem_uint_sum_reduce pshmem_uint_sum_reduce
#pragma weak shmem_ulong_sum_reduce = pshmem_ulong_sum_reduce
#define shmem_ulong_sum_reduce pshmem_ulong_sum_reduce
#pragma weak shmem_ulonglong_sum_reduce = pshmem_ulonglong_sum_reduce
#define shmem_ulonglong_sum_reduce pshmem_ulonglong_sum_reduce
#pragma weak shmem_int8_sum_reduce = pshmem_int8_sum_reduce
#define shmem_int8_sum_reduce pshmem_int8_sum_reduce
#pragma weak shmem_int16_sum_reduce = pshmem_int16_sum_reduce
#define shmem_int16_sum_reduce pshmem_int16_sum_reduce
#pragma weak shmem_int32_sum_reduce = pshmem_int32_sum_reduce
#define shmem_int32_sum_reduce pshmem_int32_sum_reduce
#pragma weak shmem_int64_sum_reduce = pshmem_int64_sum_reduce
#define shmem_int64_sum_reduce pshmem_int64_sum_reduce
#pragma weak shmem_uint8_sum_reduce = pshmem_uint8_sum_reduce
#define shmem_uint8_sum_reduce pshmem_uint8_sum_reduce
#pragma weak shmem_uint16_sum_reduce = pshmem_uint16_sum_reduce
#define shmem_uint16_sum_reduce pshmem_uint16_sum_reduce
#pragma weak shmem_uint32_sum_reduce = pshmem_uint32_sum_reduce
#define shmem_uint32_sum_reduce pshmem_uint32_sum_reduce
#pragma weak shmem_uint64_sum_reduce = pshmem_uint64_sum_reduce
#define shmem_uint64_sum_reduce pshmem_uint64_sum_reduce
#pragma weak shmem_size_sum_reduce = pshmem_size_sum_reduce
#define shmem_size_sum_reduce pshmem_size_sum_reduce
#pragma weak shmem_float_sum_reduce = pshmem_float_sum_reduce
#define shmem_float_sum_reduce pshmem_float_sum_reduce
#pragma weak shmem_double_sum_reduce = pshmem_double_sum_reduce
#define shmem_double_sum_reduce pshmem_double_sum_reduce
#pragma weak shmem_longdouble_sum_reduce = pshmem_longdouble_sum_reduce
#define shmem_longdouble_sum_reduce pshmem_longdouble_sum_reduce
#pragma weak shmem_complexd_sum_reduce = pshmem_complexd_sum_reduce
#define shmem_complexd_sum_reduce pshmem_complexd_sum_reduce
#pragma weak shmem_complexf_sum_reduce = pshmem_complexf_sum_reduce
#define shmem_complexf_sum_reduce pshmem_complexf_sum_reduce
/* prod */
#pragma weak shmem_char_prod_reduce = pshmem_char_prod_reduce
#define shmem_char_prod_reduce pshmem_char_prod_reduce
#pragma weak shmem_schar_prod_reduce = pshmem_schar_prod_reduce
#define shmem_schar_prod_reduce pshmem_schar_prod_reduce
#pragma weak shmem_short_prod_reduce = pshmem_short_prod_reduce
#define shmem_short_prod_reduce pshmem_short_prod_reduce
#pragma weak shmem_int_prod_reduce = pshmem_int_prod_reduce
#define shmem_int_prod_reduce pshmem_int_prod_reduce
#pragma weak shmem_long_prod_reduce = pshmem_long_prod_reduce
#define shmem_long_prod_reduce pshmem_long_prod_reduce
#pragma weak shmem_longlong_prod_reduce = pshmem_longlong_prod_reduce
#define shmem_longlong_prod_reduce pshmem_longlong_prod_reduce
#pragma weak shmem_ptrdiff_prod_reduce = pshmem_ptrdiff_prod_reduce
#define shmem_ptrdiff_prod_reduce pshmem_ptrdiff_prod_reduce
#pragma weak shmem_uchar_prod_reduce = pshmem_uchar_prod_reduce
#define shmem_uchar_prod_reduce pshmem_uchar_prod_reduce
#pragma weak shmem_ushort_prod_reduce = pshmem_ushort_prod_reduce
#define shmem_ushort_prod_reduce pshmem_ushort_prod_reduce
#pragma weak shmem_uint_prod_reduce = pshmem_uint_prod_reduce
#define shmem_uint_prod_reduce pshmem_uint_prod_reduce
#pragma weak shmem_ulong_prod_reduce = pshmem_ulong_prod_reduce
#define shmem_ulong_prod_reduce pshmem_ulong_prod_reduce
#pragma weak shmem_ulonglong_prod_reduce = pshmem_ulonglong_prod_reduce
#define shmem_ulonglong_prod_reduce pshmem_ulonglong_prod_reduce
#pragma weak shmem_int8_prod_reduce = pshmem_int8_prod_reduce
#define shmem_int8_prod_reduce pshmem_int8_prod_reduce
#pragma weak shmem_int16_prod_reduce = pshmem_int16_prod_reduce
#define shmem_int16_prod_reduce pshmem_int16_prod_reduce
#pragma weak shmem_int32_prod_reduce = pshmem_int32_prod_reduce
#define shmem_int32_prod_reduce pshmem_int32_prod_reduce
#pragma weak shmem_int64_prod_reduce = pshmem_int64_prod_reduce
#define shmem_int64_prod_reduce pshmem_int64_prod_reduce
#pragma weak shmem_uint8_prod_reduce = pshmem_uint8_prod_reduce
#define shmem_uint8_prod_reduce pshmem_uint8_prod_reduce
#pragma weak shmem_uint16_prod_reduce = pshmem_uint16_prod_reduce
#define shmem_uint16_prod_reduce pshmem_uint16_prod_reduce
#pragma weak shmem_uint32_prod_reduce = pshmem_uint32_prod_reduce
#define shmem_uint32_prod_reduce pshmem_uint32_prod_reduce
#pragma weak shmem_uint64_prod_reduce = pshmem_uint64_prod_reduce
#define shmem_uint64_prod_reduce pshmem_uint64_prod_reduce
#pragma weak shmem_size_prod_reduce = pshmem_size_prod_reduce
#define shmem_size_prod_reduce pshmem_size_prod_reduce
#pragma weak shmem_float_prod_reduce = pshmem_float_prod_reduce
#define shmem_float_prod_reduce pshmem_float_prod_reduce
#pragma weak shmem_double_prod_reduce = pshmem_double_prod_reduce
#define shmem_double_prod_reduce pshmem_double_prod_reduce
#pragma weak shmem_longdouble_prod_reduce = pshmem_longdouble_prod_reduce
#define shmem_longdouble_prod_reduce pshmem_longdouble_prod_reduce
#pragma weak shmem_complexd_prod_reduce = pshmem_complexd_prod_reduce
#define shmem_complexd_prod_reduce pshmem_complexd_prod_reduce
#pragma weak shmem_complexf_prod_reduce = pshmem_complexf_prod_reduce
#define shmem_complexf_prod_reduce pshmem_complexf_prod_reduce

#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to declare a reduction operation for a specific type and
 * operation
 *
 * @param _typename The name/prefix for the reduction function
 * @param _type The data type for the reduction
 * @param _op The reduction operation (sum, prod, min, max, etc)
 * @param _algo The algorithm implementation to use
 *
 * This macro declares a reduction function that:
 * - Allocates required synchronization arrays from symmetric heap
 * - Translates team-based parameters to active set parameters
 * - Calls the underlying shcoll reduction implementation
 * - Handles cleanup of allocated resources
 */
#define SHIM_REDUCE_DECLARE(_typename, _type, _op, _algo)                      \
  int shmem_##_typename##_##_op##_reduce(                                      \
      shmem_team_t team, _type *dest, const _type *source, size_t nreduce) {   \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    /* Allocate pSync from symmetric heap */                                   \
    long *pSync = shmem_malloc(SHCOLL_REDUCE_SYNC_SIZE * sizeof(long));        \
    if (!pSync)                                                                \
      return -1;                                                               \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_REDUCE_SYNC_SIZE; i++) {                        \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    /* Allocate pWrk from symmetric heap */                                    \
    _type *pWrk =                                                              \
        shmem_malloc(SHCOLL_REDUCE_MIN_WRKDATA_SIZE * sizeof(_type));          \
    if (!pWrk) {                                                               \
      shmem_free(pSync);                                                       \
      return -1;                                                               \
    }                                                                          \
    /* Ensure all PEs have initialized pSync */                                \
    shmem_team_sync(team);                                                     \
    /* Perform reduction */                                                    \
    shcoll_##_typename##_##_op##_to_all_##_algo(                               \
        dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);  \
    /* Cleanup */                                                              \
    shmem_team_sync(team);                                                     \
    shmem_free(pWrk);                                                          \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

/**
 * @brief Macro to declare bitwise reduction operations for standard integer
 * types
 *
 * @param _op The bitwise operation (and, or, xor)
 * @param _algo The algorithm implementation to use
 *
 * Declares reduction functions for short, int, long, etc using the specified
 * bitwise operation and algorithm.
 */
#define SHIM_REDUCE_BITWISE_TYPES(_op, _algo)                                  \
  SHIM_REDUCE_DECLARE(uchar, unsigned char, _op, _algo)                \
  SHIM_REDUCE_DECLARE(ushort, unsigned short, _op, _algo)              \
  SHIM_REDUCE_DECLARE(uint, unsigned int, _op, _algo)                  \
  SHIM_REDUCE_DECLARE(ulong, unsigned long, _op, _algo)                \
  SHIM_REDUCE_DECLARE(ulonglong, unsigned long long, _op, _algo)      \
  SHIM_REDUCE_DECLARE(int8, int8_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int16, int16_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(int32, int32_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(int64, int64_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint8, uint8_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint16, uint16_t, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(uint32, uint32_t, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(uint64, uint64_t, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(size, size_t, _op, _algo)

/**
 * @brief Macro to declare min/max reduction operations for numeric types
 *
 * @param _op The operation (min or max)
 * @param _algo The algorithm implementation to use
 *
 * Declares reduction functions for integer and floating point types using
 * the specified min/max operation and algorithm.
 */
#define SHIM_REDUCE_MINMAX_TYPES(_op, _algo)                                   \
  SHIM_REDUCE_DECLARE(char, char, _op, _algo)                                  \
  SHIM_REDUCE_DECLARE(schar, signed char, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(short, short, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(int, int, _op, _algo)                                    \
  SHIM_REDUCE_DECLARE(long, long, _op, _algo)                                  \
  SHIM_REDUCE_DECLARE(longlong, long long, _op, _algo)                         \
  SHIM_REDUCE_DECLARE(ptrdiff, ptrdiff_t, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(uchar, unsigned char, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ushort, unsigned short, _op, _algo)                      \
  SHIM_REDUCE_DECLARE(uint, unsigned int, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(ulong, unsigned long, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ulonglong, unsigned long long, _op, _algo)               \
  SHIM_REDUCE_DECLARE(int8, int8_t, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(int16, int16_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int32, int32_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int64, int64_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint8, uint8_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint16, uint16_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint32, uint32_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint64, uint64_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(size, size_t, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(float, float, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(double, double, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(longdouble, long double, _op, _algo)

/**
 * @brief Macro to declare arithmetic reduction operations for all numeric types
 *
 * @param _op The arithmetic operation (sum or prod)
 * @param _algo The algorithm implementation to use
 *
 * Declares reduction functions for all integer, floating point and complex
 * types using the specified arithmetic operation and algorithm.
 */
#define SHIM_REDUCE_ARITH_TYPES(_op, _algo)                                    \
  SHIM_REDUCE_DECLARE(char, char, _op, _algo)                                  \
  SHIM_REDUCE_DECLARE(schar, signed char, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(short, short, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(int, int, _op, _algo)                                    \
  SHIM_REDUCE_DECLARE(long, long, _op, _algo)                                  \
  SHIM_REDUCE_DECLARE(longlong, long long, _op, _algo)                         \
  SHIM_REDUCE_DECLARE(ptrdiff, ptrdiff_t, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(uchar, unsigned char, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ushort, unsigned short, _op, _algo)                      \
  SHIM_REDUCE_DECLARE(uint, unsigned int, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(ulong, unsigned long, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ulonglong, unsigned long long, _op, _algo)               \
  SHIM_REDUCE_DECLARE(int8, int8_t, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(int16, int16_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int32, int32_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int64, int64_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint8, uint8_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint16, uint16_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint32, uint32_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint64, uint64_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(size, size_t, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(float, float, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(double, double, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(longdouble, long double, _op, _algo)                     \
  SHIM_REDUCE_DECLARE(complexd, double _Complex, _op, _algo)                   \
  SHIM_REDUCE_DECLARE(complexf, float _Complex, _op, _algo)

/**
 * @brief Macro to declare all bitwise reduction operations
 *
 * @param _algo The algorithm implementation to use
 *
 * Declares AND, OR and XOR reduction functions for all integer types.
 */
#define SHIM_REDUCE_BITWISE_ALL(_algo)                                         \
  SHIM_REDUCE_BITWISE_TYPES(or, _algo)                                         \
  SHIM_REDUCE_BITWISE_TYPES(xor, _algo)                                        \
  SHIM_REDUCE_BITWISE_TYPES(and, _algo)

/**
 * @brief Macro to declare all min/max reduction operations
 *
 * @param _algo The algorithm implementation to use
 *
 * Declares MIN and MAX reduction functions for all numeric types.
 */
#define SHIM_REDUCE_MINMAX_ALL(_algo)                                          \
  SHIM_REDUCE_MINMAX_TYPES(min, _algo)                                         \
  SHIM_REDUCE_MINMAX_TYPES(max, _algo)

/**
 * @brief Macro to declare all arithmetic reduction operations
 *
 * @param _algo The algorithm implementation to use
 *
 * Declares SUM and PROD reduction functions for all numeric types.
 */
#define SHIM_REDUCE_ARITH_ALL(_algo)                                           \
  SHIM_REDUCE_ARITH_TYPES(sum, _algo)                                          \
  SHIM_REDUCE_ARITH_TYPES(prod, _algo)

/**
 * @brief Macro to declare all reduction operations
 *
 * @param _algo The algorithm implementation to use
 *
 * Declares all reduction operations (bitwise, min/max, arithmetic)
 * for all supported types using the specified algorithm.
 */
#define SHIM_REDUCE_ALL(_algo)                                                 \
  SHIM_REDUCE_BITWISE_ALL(_algo)                                               \
  SHIM_REDUCE_MINMAX_ALL(_algo)                                                \
  SHIM_REDUCE_ARITH_ALL(_algo)

#endif /* ! _REDUCTIONS_H */
