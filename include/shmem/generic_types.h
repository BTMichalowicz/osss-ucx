/**
 * @file generic_types.h
 * @brief OpenSHMEM generic type macros and definitions
 *
 * This file contains C11 generic selection macros that map generic OpenSHMEM
 * operations to their type-specific implementations.
 */

#ifndef _SHMEM_GENERIC_TYPES_H
#define _SHMEM_GENERIC_TYPES_H 1

#include <shmem/defs.h>
#include <sys/types.h>
#include <stdint.h>
#include <complex.h>

#if SHMEM_HAS_C11

/*
 * Standard RMA types (Table 5)
 *
 * Used by:
 * - shmem_put()
 * - shmem_p()
 * - shmem_iput()
 * - shmem_get()
 * - shmem_g()
 * - shmem_iget()
 * - shmem_put_nbi()
 * - shmem_get_nbi()
 * - shmem_put_signal()
 * - shmem_put_signal_nbi()
 *
 * - shmem_alltoall()
 * - shmem_alltoalls()
 * - shmem_broadcast()
 * - shmem_collect()
 * - shmem_fcollect()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors. For 64-bit: use long, unsigned long, long
 * long, unsigned long long. For size_t: use unsigned long (LP64). For
 * ptrdiff_t: use long (LP64).
 */
#define C11_SHMEM_STANDARD_RMA_TYPE_TABLE(X)                                   \
  X(float, float)                                                              \
  X(double, double)                                                            \
  X(long double, longdouble)                                                   \
  X(char, char)                                                                \
  X(signed char, schar)                                                        \
  X(short, short)                                                              \
  X(int, int)                                                                  \
  X(long, long)                                                                \
  X(long long, longlong)                                                       \
  X(unsigned char, uchar)                                                      \
  X(unsigned short, ushort)                                                    \
  X(unsigned int, uint)                                                        \
  X(unsigned long, ulong)                                                      \
  X(unsigned long long, ulonglong)

/*
 * Standard AMO types (Table 6)
 *
 * Used by:
 * - shmem_atomic_compare_swap()
 * - shmem_atomic_fetch_inc()
 * - shmem_atomic_inc()
 * - shmem_atomic_fetch_add()
 * - shmem_atomic_add()
 * - shmem_atomic_compare_swap_nbi()
 * - shmem_atomic_fetch_inc_nbi()
 * - shmem_atomic_fetch_add_nbi()
 *
 * - shmem_wait_until() (FIXME: also need `short` and `ushort`, which are
 * deprecated)
 * - shmem_wait_until_all()
 * - shmem_wait_until_any()
 * - shmem_wait_until_some()
 * - shmem_wait_until_all_vector()
 * - shmem_wait_until_any_vector()
 * - shmem_wait_until_some_vector()
 * - shmem_test() (FIXME: also need `short` and `ushort`, which are deprecated)
 * - shmmem_test_all()
 * - shmem_test_any()
 * - shmem_test_some()
 * - shmem_test_all_vector()
 * - shmem_test_any_vector()
 * - shmem_test_some_vector()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors.
 */
#define C11_SHMEM_STANDARD_AMO_TYPE_TABLE(X)                                   \
  X(int, int)                                                                  \
  X(long, long)                                                                \
  X(long long, longlong)                                                       \
  X(unsigned int, uint)                                                        \
  X(unsigned long, ulong)                                                      \
  X(unsigned long long, ulonglong)

/*
 * Extended AMO types (Table 7)
 *
 * Used by:
 * - shmem_atomic_fetch()
 * - shmem_atomic_set()
 * - shmem_atomic_swap()
 * - shmem_atomic_fetch_nbi()
 * - shmem_atomic_swap_nbi()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors.
 */
#define C11_SHMEM_EXTENDED_AMO_TYPE_TABLE(X)                                   \
  X(float, float)                                                              \
  X(double, double)                                                            \
  X(int, int)                                                                  \
  X(long, long)                                                                \
  X(long long, longlong)                                                       \
  X(unsigned int, uint)                                                        \
  X(unsigned long, ulong)                                                      \
  X(unsigned long long, ulonglong)

/*
 * Bitwise AMO types (Table 8)
 *
 * Used by:
 * - shmem_atomic_fetch_and()
 * - shmem_atomic_and()
 * - shmem_atomic_fetch_or()
 * - shmem_atomic_or()
 * - shmem_atomic_fetch_xor()
 * - shmem_atomic_xor()
 * - shmem_atomic_fetch_and_nbi()
 * - shmem_atomic_fetch_or_nbi()
 * - shmem_atomic_fetch_xor_nbi()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors.
 */
#define C11_SHMEM_BITWISE_AMO_TYPE_TABLE(X)                                    \
  X(unsigned int, uint)                                                        \
  X(unsigned long, ulong)                                                      \
  X(unsigned long long, ulonglong)                                             \
  X(int32_t, int32)                                                            \
  X(int64_t, int64)

/*
 * Bitwise to_all types (Table 11)
 *
 * Used by:
 * - shmem_and_to_all()
 * - shmem_or_to_all()
 * - shmem_xor_to_all()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors.
 */
#define C11_SHMEM_TOALL_BITWISE_TYPE_TABLE(X)                                  \
  X(short, short)                                                              \
  X(int, int)                                                                  \
  X(long, long)                                                                \
  X(long long, longlong)

/*
 * Minmax to_all types (Table 11)
 *
 * Used by:
 * - shmem_min_to_all()
 * - shmem_max_to_all()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors.
 */
#define C11_SHMEM_TOALL_MINMAX_TYPE_TABLE(X)                                   \
  X(short, short)                                                              \
  X(int, int)                                                                  \
  X(long, long)                                                                \
  X(long long, longlong)                                                       \
  X(float, float)                                                              \
  X(double, double)                                                            \
  X(long double, longdouble)

/*
 * Arithmetic to_all types (Table 11)
 *
 * Used by:
 * - shmem_sum_to_all()
 * - shmem_prod_to_all()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors.
 */
#define C11_SHMEM_TOALL_ARITH_TYPE_TABLE(X)                                    \
  X(short, short)                                                              \
  X(int, int)                                                                  \
  X(long, long)                                                                \
  X(long long, longlong)                                                       \
  X(float, float)                                                              \
  X(double, double)                                                            \
  X(long double, longdouble)                                                   \
  X(double _Complex, complexd)                                                 \
  X(float _Complex, complexf)

/*
 * Team-based reduce bitwise operations (Table 10)
 *
 * Used by:
 * - shmem_and_reduce()
 * - shmem_or_reduce()
 * - shmem_xor_reduce()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors.
 */
#define C11_SHMEM_REDUCE_BITWISE_TYPE_TABLE(X)                                 \
  X(unsigned char, uchar)                                                      \
  X(unsigned short, ushort)                                                    \
  X(unsigned int, uint)                                                        \
  X(unsigned long, ulong)                                                      \
  X(unsigned long long, ulonglong)                                             \
  X(int, int)                                                                  \
  X(long, long)                                                                \
  X(long long, longlong)

/*
 * Team-based reduce minmax operations (Table 10)
 *
 * Used by:
 * - shmem_min_reduce()
 * - shmem_max_reduce()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors.
 */
#define C11_SHMEM_REDUCE_MINMAX_TYPE_TABLE(X)                                  \
  X(char, char)                                                                \
  X(signed char, schar)                                                        \
  X(short, short)                                                              \
  X(int, int)                                                                  \
  X(long, long)                                                                \
  X(long long, longlong)                                                       \
  X(unsigned char, uchar)                                                      \
  X(unsigned short, ushort)                                                    \
  X(unsigned int, uint)                                                        \
  X(unsigned long, ulong)                                                      \
  X(unsigned long long, ulonglong)                                             \
  X(float, float)                                                              \
  X(double, double)                                                            \
  X(long double, longdouble)

/*
 * Team-based reduce arithmetic operations (Table 10)
 *
 * Used by:
 * - shmem_sum_reduce()
 * - shmem_prod_reduce()
 *
 * NOTE: Only canonical C types are included below to avoid _Generic
 * duplicate/compatible type errors.
 */
#define C11_SHMEM_REDUCE_ARITH_TYPE_TABLE(X)                                   \
  X(char, char)                                                                \
  X(signed char, schar)                                                        \
  X(short, short)                                                              \
  X(int, int)                                                                  \
  X(long, long)                                                                \
  X(long long, longlong)                                                       \
  X(unsigned char, uchar)                                                      \
  X(unsigned short, ushort)                                                    \
  X(unsigned int, uint)                                                        \
  X(unsigned long, ulong)                                                      \
  X(unsigned long long, ulonglong)                                             \
  X(float, float)                                                              \
  X(double, double)                                                            \
  X(long double, longdouble)                                                   \
  X(double _Complex, complexd)                                                 \
  X(float _Complex, complexf)

/* Preprocesser hack used for implementation of typed routines
 * that need the operand size.
 */
#define SHMEM_TYPE_BITSOF_short 16
#define SHMEM_TYPE_BITSOF_ushort 16
#define SHMEM_TYPE_BITSOF_int 32
#define SHMEM_TYPE_BITSOF_long 64
#define SHMEM_TYPE_BITSOF_longlong 64
#define SHMEM_TYPE_BITSOF_uint 32
#define SHMEM_TYPE_BITSOF_ulong 64
#define SHMEM_TYPE_BITSOF_ulonglong 64
#define SHMEM_TYPE_BITSOF_int32 32
#define SHMEM_TYPE_BITSOF_int64 64
#define SHMEM_TYPE_BITSOF_uint32 32
#define SHMEM_TYPE_BITSOF_uint64 64
#define SHMEM_TYPE_BITSOF_size 64
#define SHMEM_TYPE_BITSOF_ptrdiff 64

#endif /* SHMEM_HAS_C11 */

#endif /* _SHMEM_GENERIC_TYPES_H */
