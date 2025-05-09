/**
 * @file reduction.h
 * @brief Header file containing declarations for collective reduction
 * operations
 *
 * This file provides declarations for various reduction operations (AND, OR,
 * XOR, MIN, MAX, SUM, PROD) across different data types. Multiple algorithm
 * implementations are supported including linear, binomial, recursive doubling,
 * and Rabenseifner's algorithm.
 */

#ifndef _SHCOLL_REDUCTION_H
#define _SHCOLL_REDUCTION_H 1

#include <shmem/teams.h>
#include "shmemu.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Macro to declare a single reduction operation
 *
 * @param _typename_op Name of the reduction operation (e.g. int_sum)
 * @param _type Data type to operate on
 * @param _algo Algorithm implementation to use
 */
#define SHCOLL_TO_ALL_DECLARE(_typename_op, _type, _algo)                      \
  void shcoll_##_typename_op##_to_all_##_algo(                                 \
      _type *dest, const _type *source, int nreduce, int PE_start,             \
      int logPE_stride, int PE_size, _type *pWrk, long *pSync)

/**
 * @brief Macro to declare all reduction operations for a given algorithm
 *
 * Declares reduction operations for:
 * - Bitwise operations (AND, OR, XOR)
 * - Comparison operations (MIN, MAX)
 * - Arithmetic operations (SUM, PROD)
 * Across multiple data types including integers, floating point and complex
 * numbers
 *
 * @param _algo Algorithm implementation to use
 */
#define SHCOLL_TO_ALL_DECLARE_ALL(_algo)                                       \
  /* AND operation */                                                          \
  SHCOLL_TO_ALL_DECLARE(char_and, char, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(schar_and, signed char, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(short_and, short, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int_and, int, _algo);                                  \
  SHCOLL_TO_ALL_DECLARE(long_and, long, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(longlong_and, long long, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(ptrdiff_and, ptrdiff_t, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(uchar_and, unsigned char, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ushort_and, unsigned short, _algo);                    \
  SHCOLL_TO_ALL_DECLARE(uint_and, unsigned int, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(ulong_and, unsigned long, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ulonglong_and, unsigned long long, _algo);             \
  SHCOLL_TO_ALL_DECLARE(int8_and, int8_t, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int16_and, int16_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int32_and, int32_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int64_and, int64_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint8_and, uint8_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint16_and, uint16_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint32_and, uint32_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint64_and, uint64_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(size_and, size_t, _algo);                              \
                                                                               \
  /* OR operation */                                                           \
  SHCOLL_TO_ALL_DECLARE(char_or, char, _algo);                                 \
  SHCOLL_TO_ALL_DECLARE(schar_or, signed char, _algo);                         \
  SHCOLL_TO_ALL_DECLARE(short_or, short, _algo);                               \
  SHCOLL_TO_ALL_DECLARE(int_or, int, _algo);                                   \
  SHCOLL_TO_ALL_DECLARE(long_or, long, _algo);                                 \
  SHCOLL_TO_ALL_DECLARE(longlong_or, long long, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(ptrdiff_or, ptrdiff_t, _algo);                         \
  SHCOLL_TO_ALL_DECLARE(uchar_or, unsigned char, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(ushort_or, unsigned short, _algo);                     \
  SHCOLL_TO_ALL_DECLARE(uint_or, unsigned int, _algo);                         \
  SHCOLL_TO_ALL_DECLARE(ulong_or, unsigned long, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(ulonglong_or, unsigned long long, _algo);              \
  SHCOLL_TO_ALL_DECLARE(int8_or, int8_t, _algo);                               \
  SHCOLL_TO_ALL_DECLARE(int16_or, int16_t, _algo);                             \
  SHCOLL_TO_ALL_DECLARE(int32_or, int32_t, _algo);                             \
  SHCOLL_TO_ALL_DECLARE(int64_or, int64_t, _algo);                             \
  SHCOLL_TO_ALL_DECLARE(uint8_or, uint8_t, _algo);                             \
  SHCOLL_TO_ALL_DECLARE(uint16_or, uint16_t, _algo);                           \
  SHCOLL_TO_ALL_DECLARE(uint32_or, uint32_t, _algo);                           \
  SHCOLL_TO_ALL_DECLARE(uint64_or, uint64_t, _algo);                           \
  SHCOLL_TO_ALL_DECLARE(size_or, size_t, _algo);                               \
                                                                               \
  /* XOR operation */                                                          \
  SHCOLL_TO_ALL_DECLARE(char_xor, char, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(schar_xor, signed char, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(short_xor, short, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int_xor, int, _algo);                                  \
  SHCOLL_TO_ALL_DECLARE(long_xor, long, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(longlong_xor, long long, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(ptrdiff_xor, ptrdiff_t, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(uchar_xor, unsigned char, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ushort_xor, unsigned short, _algo);                    \
  SHCOLL_TO_ALL_DECLARE(uint_xor, unsigned int, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(ulong_xor, unsigned long, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ulonglong_xor, unsigned long long, _algo);             \
  SHCOLL_TO_ALL_DECLARE(int8_xor, int8_t, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int16_xor, int16_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int32_xor, int32_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int64_xor, int64_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint8_xor, uint8_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint16_xor, uint16_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint32_xor, uint32_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint64_xor, uint64_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(size_xor, size_t, _algo);                              \
                                                                               \
  /* MAX operation */                                                          \
  SHCOLL_TO_ALL_DECLARE(char_max, char, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(schar_max, signed char, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(short_max, short, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int_max, int, _algo);                                  \
  SHCOLL_TO_ALL_DECLARE(long_max, long, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(longlong_max, long long, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(ptrdiff_max, ptrdiff_t, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(uchar_max, unsigned char, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ushort_max, unsigned short, _algo);                    \
  SHCOLL_TO_ALL_DECLARE(uint_max, unsigned int, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(ulong_max, unsigned long, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ulonglong_max, unsigned long long, _algo);             \
  SHCOLL_TO_ALL_DECLARE(int8_max, int8_t, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int16_max, int16_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int32_max, int32_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int64_max, int64_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint8_max, uint8_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint16_max, uint16_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint32_max, uint32_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint64_max, uint64_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(size_max, size_t, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(float_max, float, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(double_max, double, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(longdouble_max, long double, _algo);                   \
                                                                               \
  /* MIN operation */                                                          \
  SHCOLL_TO_ALL_DECLARE(char_min, char, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(schar_min, signed char, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(short_min, short, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int_min, int, _algo);                                  \
  SHCOLL_TO_ALL_DECLARE(long_min, long, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(longlong_min, long long, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(ptrdiff_min, ptrdiff_t, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(uchar_min, unsigned char, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ushort_min, unsigned short, _algo);                    \
  SHCOLL_TO_ALL_DECLARE(uint_min, unsigned int, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(ulong_min, unsigned long, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ulonglong_min, unsigned long long, _algo);             \
  SHCOLL_TO_ALL_DECLARE(int8_min, int8_t, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int16_min, int16_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int32_min, int32_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int64_min, int64_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint8_min, uint8_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint16_min, uint16_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint32_min, uint32_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint64_min, uint64_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(size_min, size_t, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(float_min, float, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(double_min, double, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(longdouble_min, long double, _algo);                   \
                                                                               \
  /* SUM operation */                                                          \
  SHCOLL_TO_ALL_DECLARE(char_sum, char, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(schar_sum, signed char, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(short_sum, short, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int_sum, int, _algo);                                  \
  SHCOLL_TO_ALL_DECLARE(long_sum, long, _algo);                                \
  SHCOLL_TO_ALL_DECLARE(longlong_sum, long long, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(ptrdiff_sum, ptrdiff_t, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(uchar_sum, unsigned char, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ushort_sum, unsigned short, _algo);                    \
  SHCOLL_TO_ALL_DECLARE(uint_sum, unsigned int, _algo);                        \
  SHCOLL_TO_ALL_DECLARE(ulong_sum, unsigned long, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ulonglong_sum, unsigned long long, _algo);             \
  SHCOLL_TO_ALL_DECLARE(int8_sum, int8_t, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(int16_sum, int16_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int32_sum, int32_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(int64_sum, int64_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint8_sum, uint8_t, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(uint16_sum, uint16_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint32_sum, uint32_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(uint64_sum, uint64_t, _algo);                          \
  SHCOLL_TO_ALL_DECLARE(size_sum, size_t, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(float_sum, float, _algo);                              \
  SHCOLL_TO_ALL_DECLARE(double_sum, double, _algo);                            \
  SHCOLL_TO_ALL_DECLARE(longdouble_sum, long double, _algo);                   \
  SHCOLL_TO_ALL_DECLARE(complexf_sum, float _Complex, _algo);                  \
  SHCOLL_TO_ALL_DECLARE(complexd_sum, double _Complex, _algo);                 \
                                                                               \
  /* PROD operation */                                                         \
  SHCOLL_TO_ALL_DECLARE(char_prod, char, _algo);                               \
  SHCOLL_TO_ALL_DECLARE(schar_prod, signed char, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(short_prod, short, _algo);                             \
  SHCOLL_TO_ALL_DECLARE(int_prod, int, _algo);                                 \
  SHCOLL_TO_ALL_DECLARE(long_prod, long, _algo);                               \
  SHCOLL_TO_ALL_DECLARE(longlong_prod, long long, _algo);                      \
  SHCOLL_TO_ALL_DECLARE(ptrdiff_prod, ptrdiff_t, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(uchar_prod, unsigned char, _algo);                     \
  SHCOLL_TO_ALL_DECLARE(ushort_prod, unsigned short, _algo);                   \
  SHCOLL_TO_ALL_DECLARE(uint_prod, unsigned int, _algo);                       \
  SHCOLL_TO_ALL_DECLARE(ulong_prod, unsigned long, _algo);                     \
  SHCOLL_TO_ALL_DECLARE(ulonglong_prod, unsigned long long, _algo);            \
  SHCOLL_TO_ALL_DECLARE(int8_prod, int8_t, _algo);                             \
  SHCOLL_TO_ALL_DECLARE(int16_prod, int16_t, _algo);                           \
  SHCOLL_TO_ALL_DECLARE(int32_prod, int32_t, _algo);                           \
  SHCOLL_TO_ALL_DECLARE(int64_prod, int64_t, _algo);                           \
  SHCOLL_TO_ALL_DECLARE(uint8_prod, uint8_t, _algo);                           \
  SHCOLL_TO_ALL_DECLARE(uint16_prod, uint16_t, _algo);                         \
  SHCOLL_TO_ALL_DECLARE(uint32_prod, uint32_t, _algo);                         \
  SHCOLL_TO_ALL_DECLARE(uint64_prod, uint64_t, _algo);                         \
  SHCOLL_TO_ALL_DECLARE(size_prod, size_t, _algo);                             \
  SHCOLL_TO_ALL_DECLARE(float_prod, float, _algo);                             \
  SHCOLL_TO_ALL_DECLARE(double_prod, double, _algo);                           \
  SHCOLL_TO_ALL_DECLARE(longdouble_prod, long double, _algo);                  \
  SHCOLL_TO_ALL_DECLARE(complexf_prod, float _Complex, _algo);                 \
  SHCOLL_TO_ALL_DECLARE(complexd_prod, double _Complex, _algo);

// clang-format off
/* Declare all "to all" operations for each supported algorithm */
SHCOLL_TO_ALL_DECLARE_ALL(linear)        /**< Linear "to all" algorithm */
SHCOLL_TO_ALL_DECLARE_ALL(binomial)      /**< Binomial tree "to all" */
SHCOLL_TO_ALL_DECLARE_ALL(rec_dbl)       /**< Recursive doubling "to all" */
SHCOLL_TO_ALL_DECLARE_ALL(rabenseifner)  /**< Rabenseifner's "to all" algorithm */
SHCOLL_TO_ALL_DECLARE_ALL(rabenseifner2) /**< Modified Rabenseifner's algorithm */
// clang-format on

/********************************************************************************/

/**
 * @brief Macro to declare a single reduction operation for a specific type
 *
 * @param _typename Type name used in function name (e.g. int, float)
 * @param _type Actual C type (e.g. int, float)
 * @param _op Operation name (e.g. sum, prod, min, max)
 * @param _algo Algorithm implementation to use
 */
#define SHCOLL_REDUCE_DECLARE(_typename, _type, _op, _algo)                    \
  int shcoll_##_typename##_##_op##_reduce_##_algo(                             \
      shmem_team_t team, _type *dest, const _type *source, size_t nreduce);

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
#define SHCOLL_REDUCE_BITWISE_TYPES(_op, _algo)                                \
  SHCOLL_REDUCE_DECLARE(uchar, unsigned char, _op, _algo)                      \
  SHCOLL_REDUCE_DECLARE(ushort, unsigned short, _op, _algo)                    \
  SHCOLL_REDUCE_DECLARE(uint, unsigned int, _op, _algo)                        \
  SHCOLL_REDUCE_DECLARE(ulong, unsigned long, _op, _algo)                      \
  SHCOLL_REDUCE_DECLARE(ulonglong, unsigned long long, _op, _algo)             \
  SHCOLL_REDUCE_DECLARE(int8, int8_t, _op, _algo)                              \
  SHCOLL_REDUCE_DECLARE(int16, int16_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(int32, int32_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(int64, int64_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(uint8, uint8_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(uint16, uint16_t, _op, _algo)                          \
  SHCOLL_REDUCE_DECLARE(uint32, uint32_t, _op, _algo)                          \
  SHCOLL_REDUCE_DECLARE(uint64, uint64_t, _op, _algo)                          \
  SHCOLL_REDUCE_DECLARE(size, size_t, _op, _algo)

/**
 * @brief Macro to declare min/max reduction operations for numeric types
 *
 * @param _op The operation (min or max)
 * @param _algo The algorithm implementation to use
 *
 * Declares reduction functions for integer and floating point types using
 * the specified min/max operation and algorithm.
 */
#define SHCOLL_REDUCE_MINMAX_TYPES(_op, _algo)                                 \
  SHCOLL_REDUCE_DECLARE(char, char, _op, _algo)                                \
  SHCOLL_REDUCE_DECLARE(schar, signed char, _op, _algo)                        \
  SHCOLL_REDUCE_DECLARE(short, short, _op, _algo)                              \
  SHCOLL_REDUCE_DECLARE(int, int, _op, _algo)                                  \
  SHCOLL_REDUCE_DECLARE(long, long, _op, _algo)                                \
  SHCOLL_REDUCE_DECLARE(longlong, long long, _op, _algo)                       \
  SHCOLL_REDUCE_DECLARE(ptrdiff, ptrdiff_t, _op, _algo)                        \
  SHCOLL_REDUCE_DECLARE(uchar, unsigned char, _op, _algo)                      \
  SHCOLL_REDUCE_DECLARE(ushort, unsigned short, _op, _algo)                    \
  SHCOLL_REDUCE_DECLARE(uint, unsigned int, _op, _algo)                        \
  SHCOLL_REDUCE_DECLARE(ulong, unsigned long, _op, _algo)                      \
  SHCOLL_REDUCE_DECLARE(ulonglong, unsigned long long, _op, _algo)             \
  SHCOLL_REDUCE_DECLARE(int8, int8_t, _op, _algo)                              \
  SHCOLL_REDUCE_DECLARE(int16, int16_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(int32, int32_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(int64, int64_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(uint8, uint8_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(uint16, uint16_t, _op, _algo)                          \
  SHCOLL_REDUCE_DECLARE(uint32, uint32_t, _op, _algo)                          \
  SHCOLL_REDUCE_DECLARE(uint64, uint64_t, _op, _algo)                          \
  SHCOLL_REDUCE_DECLARE(size, size_t, _op, _algo)                              \
  SHCOLL_REDUCE_DECLARE(float, float, _op, _algo)                              \
  SHCOLL_REDUCE_DECLARE(double, double, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(longdouble, long double, _op, _algo)

/**
 * @brief Macro to declare arithmetic reduction operations for all numeric types
 *
 * @param _op The arithmetic operation (sum or prod)
 * @param _algo The algorithm implementation to use
 *
 * Declares reduction functions for all integer, floating point and complex
 * types using the specified arithmetic operation and algorithm.
 */
#define SHCOLL_REDUCE_ARITH_TYPES(_op, _algo)                                  \
  SHCOLL_REDUCE_DECLARE(char, char, _op, _algo)                                \
  SHCOLL_REDUCE_DECLARE(schar, signed char, _op, _algo)                        \
  SHCOLL_REDUCE_DECLARE(short, short, _op, _algo)                              \
  SHCOLL_REDUCE_DECLARE(int, int, _op, _algo)                                  \
  SHCOLL_REDUCE_DECLARE(long, long, _op, _algo)                                \
  SHCOLL_REDUCE_DECLARE(longlong, long long, _op, _algo)                       \
  SHCOLL_REDUCE_DECLARE(ptrdiff, ptrdiff_t, _op, _algo)                        \
  SHCOLL_REDUCE_DECLARE(uchar, unsigned char, _op, _algo)                      \
  SHCOLL_REDUCE_DECLARE(ushort, unsigned short, _op, _algo)                    \
  SHCOLL_REDUCE_DECLARE(uint, unsigned int, _op, _algo)                        \
  SHCOLL_REDUCE_DECLARE(ulong, unsigned long, _op, _algo)                      \
  SHCOLL_REDUCE_DECLARE(ulonglong, unsigned long long, _op, _algo)             \
  SHCOLL_REDUCE_DECLARE(int8, int8_t, _op, _algo)                              \
  SHCOLL_REDUCE_DECLARE(int16, int16_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(int32, int32_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(int64, int64_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(uint8, uint8_t, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(uint16, uint16_t, _op, _algo)                          \
  SHCOLL_REDUCE_DECLARE(uint32, uint32_t, _op, _algo)                          \
  SHCOLL_REDUCE_DECLARE(uint64, uint64_t, _op, _algo)                          \
  SHCOLL_REDUCE_DECLARE(size, size_t, _op, _algo)                              \
  SHCOLL_REDUCE_DECLARE(float, float, _op, _algo)                              \
  SHCOLL_REDUCE_DECLARE(double, double, _op, _algo)                            \
  SHCOLL_REDUCE_DECLARE(longdouble, long double, _op, _algo)                   \
  SHCOLL_REDUCE_DECLARE(complexd, double _Complex, _op, _algo)                 \
  SHCOLL_REDUCE_DECLARE(complexf, float _Complex, _op, _algo)

/**
 * @brief Macro to declare all bitwise reduction operations
 *
 * @param _algo The algorithm implementation to use
 *
 * Declares AND, OR and XOR reduction functions for all integer types.
 */
#define SHCOLL_REDUCE_BITWISE_ALL(_algo)                                       \
  SHCOLL_REDUCE_BITWISE_TYPES(or, _algo)                                       \
  SHCOLL_REDUCE_BITWISE_TYPES(xor, _algo)                                      \
  SHCOLL_REDUCE_BITWISE_TYPES(and, _algo)

/**
 * @brief Macro to declare all min/max reduction operations
 *
 * @param _algo The algorithm implementation to use
 *
 * Declares MIN and MAX reduction functions for all numeric types.
 */
#define SHCOLL_REDUCE_MINMAX_ALL(_algo)                                        \
  SHCOLL_REDUCE_MINMAX_TYPES(min, _algo)                                       \
  SHCOLL_REDUCE_MINMAX_TYPES(max, _algo)

/**
 * @brief Macro to declare all arithmetic reduction operations
 *
 * @param _algo The algorithm implementation to use
 *
 * Declares SUM and PROD reduction functions for all numeric types.
 */
#define SHCOLL_REDUCE_ARITH_ALL(_algo)                                         \
  SHCOLL_REDUCE_ARITH_TYPES(sum, _algo)                                        \
  SHCOLL_REDUCE_ARITH_TYPES(prod, _algo)

/**
 * @brief Macro to declare all reduction operations
 *
 * @param _algo The algorithm implementation to use
 *
 * Declares all reduction operations (bitwise, min/max, arithmetic)
 * for all supported types using the specified algorithm.
 */
#define SHCOLL_REDUCE_ALL(_algo)                                               \
  SHCOLL_REDUCE_BITWISE_ALL(_algo)                                             \
  SHCOLL_REDUCE_MINMAX_ALL(_algo)                                              \
  SHCOLL_REDUCE_ARITH_ALL(_algo)

/**
 * @brief Macro to declare all reduction operations for all supported types
 *
 * @param _algo The algorithm implementation to use
 *
 * Declares all reduction operations (bitwise, min/max, arithmetic)
 * for all supported types using the specified algorithm.
 */
#define SHCOLL_REDUCE_DECLARE_ALL(_algo)                                       \
  SHCOLL_REDUCE_BITWISE_ALL(_algo)                                             \
  SHCOLL_REDUCE_MINMAX_ALL(_algo)                                              \
  SHCOLL_REDUCE_ARITH_ALL(_algo)

SHCOLL_REDUCE_DECLARE_ALL(linear)
SHCOLL_REDUCE_DECLARE_ALL(binomial)
SHCOLL_REDUCE_DECLARE_ALL(rec_dbl)
SHCOLL_REDUCE_DECLARE_ALL(rabenseifner)
SHCOLL_REDUCE_DECLARE_ALL(rabenseifner2)

#endif /* ! _SHCOLL_REDUCTION_H */
