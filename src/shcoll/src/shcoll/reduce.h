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

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Macro to declare a single reduction operation
 *
 * @param _name Name of the reduction operation (e.g. int_sum)
 * @param _type Data type to operate on
 * @param _algorithm Algorithm implementation to use
 */
#define SHCOLL_REDUCE_DECLARE(_name, _type, _algorithm)                        \
  void shcoll_##_name##_to_all_##_algorithm(                                   \
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
 * @param _algorithm Algorithm implementation to use
 */
#define SHCOLL_REDUCE_DECLARE_ALL(_algorithm)                                  \
  /* AND operation */                                                          \
  SHCOLL_REDUCE_DECLARE(char_and, char, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(schar_and, signed char, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(short_and, short, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int_and, int, _algorithm);                             \
  SHCOLL_REDUCE_DECLARE(long_and, long, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(longlong_and, long long, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(ptrdiff_and, ptrdiff_t, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(uchar_and, unsigned char, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ushort_and, unsigned short, _algorithm);               \
  SHCOLL_REDUCE_DECLARE(uint_and, unsigned int, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(ulong_and, unsigned long, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ulonglong_and, unsigned long long, _algorithm);        \
  SHCOLL_REDUCE_DECLARE(int8_and, int8_t, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int16_and, int16_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int32_and, int32_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int64_and, int64_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint8_and, uint8_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint16_and, uint16_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint32_and, uint32_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint64_and, uint64_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(size_and, size_t, _algorithm);                         \
                                                                               \
  /* OR operation */                                                           \
  SHCOLL_REDUCE_DECLARE(char_or, char, _algorithm);                            \
  SHCOLL_REDUCE_DECLARE(schar_or, signed char, _algorithm);                    \
  SHCOLL_REDUCE_DECLARE(short_or, short, _algorithm);                          \
  SHCOLL_REDUCE_DECLARE(int_or, int, _algorithm);                              \
  SHCOLL_REDUCE_DECLARE(long_or, long, _algorithm);                            \
  SHCOLL_REDUCE_DECLARE(longlong_or, long long, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(ptrdiff_or, ptrdiff_t, _algorithm);                    \
  SHCOLL_REDUCE_DECLARE(uchar_or, unsigned char, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(ushort_or, unsigned short, _algorithm);                \
  SHCOLL_REDUCE_DECLARE(uint_or, unsigned int, _algorithm);                    \
  SHCOLL_REDUCE_DECLARE(ulong_or, unsigned long, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(ulonglong_or, unsigned long long, _algorithm);         \
  SHCOLL_REDUCE_DECLARE(int8_or, int8_t, _algorithm);                          \
  SHCOLL_REDUCE_DECLARE(int16_or, int16_t, _algorithm);                        \
  SHCOLL_REDUCE_DECLARE(int32_or, int32_t, _algorithm);                        \
  SHCOLL_REDUCE_DECLARE(int64_or, int64_t, _algorithm);                        \
  SHCOLL_REDUCE_DECLARE(uint8_or, uint8_t, _algorithm);                        \
  SHCOLL_REDUCE_DECLARE(uint16_or, uint16_t, _algorithm);                      \
  SHCOLL_REDUCE_DECLARE(uint32_or, uint32_t, _algorithm);                      \
  SHCOLL_REDUCE_DECLARE(uint64_or, uint64_t, _algorithm);                      \
  SHCOLL_REDUCE_DECLARE(size_or, size_t, _algorithm);                          \
                                                                               \
  /* XOR operation */                                                          \
  SHCOLL_REDUCE_DECLARE(char_xor, char, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(schar_xor, signed char, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(short_xor, short, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int_xor, int, _algorithm);                             \
  SHCOLL_REDUCE_DECLARE(long_xor, long, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(longlong_xor, long long, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(ptrdiff_xor, ptrdiff_t, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(uchar_xor, unsigned char, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ushort_xor, unsigned short, _algorithm);               \
  SHCOLL_REDUCE_DECLARE(uint_xor, unsigned int, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(ulong_xor, unsigned long, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ulonglong_xor, unsigned long long, _algorithm);        \
  SHCOLL_REDUCE_DECLARE(int8_xor, int8_t, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int16_xor, int16_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int32_xor, int32_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int64_xor, int64_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint8_xor, uint8_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint16_xor, uint16_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint32_xor, uint32_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint64_xor, uint64_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(size_xor, size_t, _algorithm);                         \
                                                                               \
  /* MAX operation */                                                          \
  SHCOLL_REDUCE_DECLARE(char_max, char, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(schar_max, signed char, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(short_max, short, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int_max, int, _algorithm);                             \
  SHCOLL_REDUCE_DECLARE(long_max, long, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(longlong_max, long long, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(ptrdiff_max, ptrdiff_t, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(uchar_max, unsigned char, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ushort_max, unsigned short, _algorithm);               \
  SHCOLL_REDUCE_DECLARE(uint_max, unsigned int, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(ulong_max, unsigned long, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ulonglong_max, unsigned long long, _algorithm);        \
  SHCOLL_REDUCE_DECLARE(int8_max, int8_t, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int16_max, int16_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int32_max, int32_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int64_max, int64_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint8_max, uint8_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint16_max, uint16_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint32_max, uint32_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint64_max, uint64_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(size_max, size_t, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(float_max, float, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(double_max, double, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(longdouble_max, long double, _algorithm);              \
                                                                               \
  /* MIN operation */                                                          \
  SHCOLL_REDUCE_DECLARE(char_min, char, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(schar_min, signed char, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(short_min, short, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int_min, int, _algorithm);                             \
  SHCOLL_REDUCE_DECLARE(long_min, long, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(longlong_min, long long, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(ptrdiff_min, ptrdiff_t, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(uchar_min, unsigned char, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ushort_min, unsigned short, _algorithm);               \
  SHCOLL_REDUCE_DECLARE(uint_min, unsigned int, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(ulong_min, unsigned long, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ulonglong_min, unsigned long long, _algorithm);        \
  SHCOLL_REDUCE_DECLARE(int8_min, int8_t, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int16_min, int16_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int32_min, int32_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int64_min, int64_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint8_min, uint8_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint16_min, uint16_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint32_min, uint32_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint64_min, uint64_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(size_min, size_t, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(float_min, float, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(double_min, double, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(longdouble_min, long double, _algorithm);              \
                                                                               \
  /* SUM operation */                                                          \
  SHCOLL_REDUCE_DECLARE(char_sum, char, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(schar_sum, signed char, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(short_sum, short, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int_sum, int, _algorithm);                             \
  SHCOLL_REDUCE_DECLARE(long_sum, long, _algorithm);                           \
  SHCOLL_REDUCE_DECLARE(longlong_sum, long long, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(ptrdiff_sum, ptrdiff_t, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(uchar_sum, unsigned char, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ushort_sum, unsigned short, _algorithm);               \
  SHCOLL_REDUCE_DECLARE(uint_sum, unsigned int, _algorithm);                   \
  SHCOLL_REDUCE_DECLARE(ulong_sum, unsigned long, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ulonglong_sum, unsigned long long, _algorithm);        \
  SHCOLL_REDUCE_DECLARE(int8_sum, int8_t, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(int16_sum, int16_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int32_sum, int32_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(int64_sum, int64_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint8_sum, uint8_t, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(uint16_sum, uint16_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint32_sum, uint32_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(uint64_sum, uint64_t, _algorithm);                     \
  SHCOLL_REDUCE_DECLARE(size_sum, size_t, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(float_sum, float, _algorithm);                         \
  SHCOLL_REDUCE_DECLARE(double_sum, double, _algorithm);                       \
  SHCOLL_REDUCE_DECLARE(longdouble_sum, long double, _algorithm);              \
  SHCOLL_REDUCE_DECLARE(complexf_sum, float _Complex, _algorithm);             \
  SHCOLL_REDUCE_DECLARE(complexd_sum, double _Complex, _algorithm);            \
                                                                               \
  /* PROD operation */                                                         \
  SHCOLL_REDUCE_DECLARE(char_prod, char, _algorithm);                          \
  SHCOLL_REDUCE_DECLARE(schar_prod, signed char, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(short_prod, short, _algorithm);                        \
  SHCOLL_REDUCE_DECLARE(int_prod, int, _algorithm);                            \
  SHCOLL_REDUCE_DECLARE(long_prod, long, _algorithm);                          \
  SHCOLL_REDUCE_DECLARE(longlong_prod, long long, _algorithm);                 \
  SHCOLL_REDUCE_DECLARE(ptrdiff_prod, ptrdiff_t, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(uchar_prod, unsigned char, _algorithm);                \
  SHCOLL_REDUCE_DECLARE(ushort_prod, unsigned short, _algorithm);              \
  SHCOLL_REDUCE_DECLARE(uint_prod, unsigned int, _algorithm);                  \
  SHCOLL_REDUCE_DECLARE(ulong_prod, unsigned long, _algorithm);                \
  SHCOLL_REDUCE_DECLARE(ulonglong_prod, unsigned long long, _algorithm);       \
  SHCOLL_REDUCE_DECLARE(int8_prod, int8_t, _algorithm);                        \
  SHCOLL_REDUCE_DECLARE(int16_prod, int16_t, _algorithm);                      \
  SHCOLL_REDUCE_DECLARE(int32_prod, int32_t, _algorithm);                      \
  SHCOLL_REDUCE_DECLARE(int64_prod, int64_t, _algorithm);                      \
  SHCOLL_REDUCE_DECLARE(uint8_prod, uint8_t, _algorithm);                      \
  SHCOLL_REDUCE_DECLARE(uint16_prod, uint16_t, _algorithm);                    \
  SHCOLL_REDUCE_DECLARE(uint32_prod, uint32_t, _algorithm);                    \
  SHCOLL_REDUCE_DECLARE(uint64_prod, uint64_t, _algorithm);                    \
  SHCOLL_REDUCE_DECLARE(size_prod, size_t, _algorithm);                        \
  SHCOLL_REDUCE_DECLARE(float_prod, float, _algorithm);                        \
  SHCOLL_REDUCE_DECLARE(double_prod, double, _algorithm);                      \
  SHCOLL_REDUCE_DECLARE(longdouble_prod, long double, _algorithm);             \
  SHCOLL_REDUCE_DECLARE(complexf_prod, float _Complex, _algorithm);            \
  SHCOLL_REDUCE_DECLARE(complexd_prod, double _Complex, _algorithm);

// clang-format off
/* Declare all reduction operations for each supported algorithm */
SHCOLL_REDUCE_DECLARE_ALL(linear)        /**< Linear reduction algorithm */
SHCOLL_REDUCE_DECLARE_ALL(binomial)      /**< Binomial tree reduction */
SHCOLL_REDUCE_DECLARE_ALL(rec_dbl)       /**< Recursive doubling reduction */
SHCOLL_REDUCE_DECLARE_ALL(rabenseifner)  /**< Rabenseifner's reduction algorithm */
SHCOLL_REDUCE_DECLARE_ALL(rabenseifner2) /**< Modified Rabenseifner's algorithm */
// clang-format on

#endif /* ! _SHCOLL_REDUCTION_H */

