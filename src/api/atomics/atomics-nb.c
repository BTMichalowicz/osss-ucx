/**
 * @file atomics-nb.c
 * @brief Implementation of SHMEM non-blocking atomic operations
 *
 * For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmem_mutex.h"
#include "shmemu.h"
#include "shmemc.h"
#include <shmem/api_types.h>

#include <bits/wordsize.h>
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

/*
 * -- context-free
 */

/**
 * @brief Macro to define non-blocking atomic operations with const target
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 */
#define API_DEF_CONST_AMO1_NBI(_op, _name, _type)                              \
  void shmem_##_name##_atomic_##_op##_nbi(_type *fetch, const _type *target,   \
                                          int pe) {                            \
    shmem_ctx_##_name##_atomic_##_op##_nbi(SHMEM_CTX_DEFAULT, fetch, target,   \
                                           pe);                                \
  }

/**
 * @brief Macro to define non-blocking atomic operations
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 */
#define API_DEF_AMO1_NBI(_op, _name, _type)                                    \
  void shmem_##_name##_atomic_##_op##_nbi(_type *fetch, _type *target,         \
                                          int pe) {                            \
    shmem_ctx_##_name##_atomic_##_op##_nbi(SHMEM_CTX_DEFAULT, fetch, target,   \
                                           pe);                                \
  }

/**
 * @brief Macro to define non-blocking atomic operations with value parameter
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 */
#define API_DEF_AMO2_NBI(_op, _name, _type)                                    \
  void shmem_##_name##_atomic_##_op##_nbi(_type *fetch, _type *target,         \
                                          _type value, int pe) {               \
    shmem_ctx_##_name##_atomic_##_op##_nbi(SHMEM_CTX_DEFAULT, fetch, target,   \
                                           value, pe);                         \
  }

#ifdef ENABLE_PSHMEM
#pragma weak shmem_int_atomic_fetch_nbi = pshmem_int_atomic_fetch_nbi
#define shmem_int_atomic_fetch_nbi pshmem_int_atomic_fetch_nbi
#pragma weak shmem_long_atomic_fetch_nbi = pshmem_long_atomic_fetch_nbi
#define shmem_long_atomic_fetch_nbi pshmem_long_atomic_fetch_nbi
#pragma weak shmem_longlong_atomic_fetch_nbi = pshmem_longlong_atomic_fetch_nbi
#define shmem_longlong_atomic_fetch_nbi pshmem_longlong_atomic_fetch_nbi
#pragma weak shmem_float_atomic_fetch_nbi = pshmem_float_atomic_fetch_nbi
#define shmem_float_atomic_fetch_nbi pshmem_float_atomic_fetch_nbi
#pragma weak shmem_double_atomic_fetch_nbi = pshmem_double_atomic_fetch_nbi
#define shmem_double_atomic_fetch_nbi pshmem_double_atomic_fetch_nbi
#pragma weak shmem_uint_atomic_fetch_nbi = pshmem_uint_atomic_fetch_nbi
#define shmem_uint_atomic_fetch_nbi pshmem_uint_atomic_fetch_nbi
#pragma weak shmem_ulong_atomic_fetch_nbi = pshmem_ulong_atomic_fetch_nbi
#define shmem_ulong_atomic_fetch_nbi pshmem_ulong_atomic_fetch_nbi
#pragma weak shmem_ulonglong_atomic_fetch_nbi =                                \
    pshmem_ulonglong_atomic_fetch_nbi
#define shmem_ulonglong_atomic_fetch_nbi pshmem_ulonglong_atomic_fetch_nbi
#pragma weak shmem_int32_atomic_fetch_nbi = pshmem_int32_atomic_fetch_nbi
#define shmem_int32_atomic_fetch_nbi pshmem_int32_atomic_fetch_nbi
#pragma weak shmem_int64_atomic_fetch_nbi = pshmem_int64_atomic_fetch_nbi
#define shmem_int64_atomic_fetch_nbi pshmem_int64_atomic_fetch_nbi
#pragma weak shmem_uint32_atomic_fetch_nbi = pshmem_uint32_atomic_fetch_nbi
#define shmem_uint32_atomic_fetch_nbi pshmem_uint32_atomic_fetch_nbi
#pragma weak shmem_uint64_atomic_fetch_nbi = pshmem_uint64_atomic_fetch_nbi
#define shmem_uint64_atomic_fetch_nbi pshmem_uint64_atomic_fetch_nbi
#pragma weak shmem_size_atomic_fetch_nbi = pshmem_size_atomic_fetch_nbi
#define shmem_size_atomic_fetch_nbi pshmem_size_atomic_fetch_nbi
#pragma weak shmem_ptrdiff_atomic_fetch_nbi = pshmem_ptrdiff_atomic_fetch_nbi
#define shmem_ptrdiff_atomic_fetch_nbi pshmem_ptrdiff_atomic_fetch_nbi
#endif /* ENABLE_PSHMEM */

/* Define non-blocking atomic fetch operations using the type table */
#define API_DEF_CONST_AMO1_NBI_HELPER(_type, _typename)                        \
  API_DEF_CONST_AMO1_NBI(fetch, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_CONST_AMO1_NBI_HELPER)
#undef API_DEF_CONST_AMO1_NBI_HELPER

#ifdef ENABLE_PSHMEM
#pragma weak shmem_int_atomic_fetch_inc_nbi = pshmem_int_atomic_fetch_inc_nbi
#define shmem_int_atomic_fetch_inc_nbi pshmem_int_atomic_fetch_inc_nbi
#pragma weak shmem_long_atomic_fetch_inc_nbi = pshmem_long_atomic_fetch_inc_nbi
#define shmem_long_atomic_fetch_inc_nbi pshmem_long_atomic_fetch_inc_nbi
#pragma weak shmem_longlong_atomic_fetch_inc_nbi =                             \
    pshmem_longlong_atomic_fetch_inc_nbi
#define shmem_longlong_atomic_fetch_inc_nbi pshmem_longlong_atomic_fetch_inc_nbi
#pragma weak shmem_uint_atomic_fetch_inc_nbi = pshmem_uint_atomic_fetch_inc_nbi
#define shmem_uint_atomic_fetch_inc_nbi pshmem_uint_atomic_fetch_inc_nbi
#pragma weak shmem_ulong_atomic_fetch_inc_nbi =                                \
    pshmem_ulong_atomic_fetch_inc_nbi
#define shmem_ulong_atomic_fetch_inc_nbi pshmem_ulong_atomic_fetch_inc_nbi
#pragma weak shmem_ulonglong_atomic_fetch_inc_nbi =                            \
    pshmem_ulonglong_atomic_fetch_inc_nbi
#define shmem_ulonglong_atomic_fetch_inc_nbi                                   \
  pshmem_ulonglong_atomic_fetch_inc_nbi
#pragma weak shmem_int32_atomic_fetch_inc_nbi =                                \
    pshmem_int32_atomic_fetch_inc_nbi
#define shmem_int32_atomic_fetch_inc_nbi pshmem_int32_atomic_fetch_inc_nbi
#pragma weak shmem_int64_atomic_fetch_inc_nbi =                                \
    pshmem_int64_atomic_fetch_inc_nbi
#define shmem_int64_atomic_fetch_inc_nbi pshmem_int64_atomic_fetch_inc_nbi
#pragma weak shmem_uint32_atomic_fetch_inc_nbi =                               \
    pshmem_uint32_atomic_fetch_inc_nbi
#define shmem_uint32_atomic_fetch_inc_nbi pshmem_uint32_atomic_fetch_inc_nbi
#pragma weak shmem_uint64_atomic_fetch_inc_nbi =                               \
    pshmem_uint64_atomic_fetch_inc_nbi
#define shmem_uint64_atomic_fetch_inc_nbi pshmem_uint64_atomic_fetch_inc_nbi
#pragma weak shmem_size_atomic_fetch_inc_nbi = pshmem_size_atomic_fetch_inc_nbi
#define shmem_size_atomic_fetch_inc_nbi pshmem_size_atomic_fetch_inc_nbi
#pragma weak shmem_ptrdiff_atomic_fetch_inc_nbi =                              \
    pshmem_ptrdiff_atomic_fetch_inc_nbi
#define shmem_ptrdiff_atomic_fetch_inc_nbi pshmem_ptrdiff_atomic_fetch_inc_nbi
#endif /* ENABLE_PSHMEM */

/* Define non-blocking atomic fetch_inc operations using the type table */
#define API_DEF_AMO1_NBI_HELPER(_type, _typename)                              \
  API_DEF_AMO1_NBI(fetch_inc, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO1_NBI_HELPER)
#undef API_DEF_AMO1_NBI_HELPER

#ifdef ENABLE_PSHMEM
#pragma weak shmem_int_atomic_fetch_add_nbi = pshmem_int_atomic_fetch_add_nbi
#define shmem_int_atomic_fetch_add_nbi pshmem_int_atomic_fetch_add_nbi
#pragma weak shmem_long_atomic_fetch_add_nbi = pshmem_long_atomic_fetch_add_nbi
#define shmem_long_atomic_fetch_add_nbi pshmem_long_atomic_fetch_add_nbi
#pragma weak shmem_longlong_atomic_fetch_add_nbi =                             \
    pshmem_longlong_atomic_fetch_add_nbi
#define shmem_longlong_atomic_fetch_add_nbi pshmem_longlong_atomic_fetch_add_nbi
#pragma weak shmem_uint_atomic_fetch_add_nbi = pshmem_uint_atomic_fetch_add_nbi
#define shmem_uint_atomic_fetch_add_nbi pshmem_uint_atomic_fetch_add_nbi
#pragma weak shmem_ulong_atomic_fetch_add_nbi =                                \
    pshmem_ulong_atomic_fetch_add_nbi
#define shmem_ulong_atomic_fetch_add_nbi pshmem_ulong_atomic_fetch_add_nbi
#pragma weak shmem_ulonglong_atomic_fetch_add_nbi =                            \
    pshmem_ulonglong_atomic_fetch_add_nbi
#define shmem_ulonglong_atomic_fetch_add_nbi                                   \
  pshmem_ulonglong_atomic_fetch_add_nbi
#pragma weak shmem_int32_atomic_fetch_add_nbi =                                \
    pshmem_int32_atomic_fetch_add_nbi
#define shmem_int32_atomic_fetch_add_nbi pshmem_int32_atomic_fetch_add_nbi
#pragma weak shmem_int64_atomic_fetch_add_nbi =                                \
    pshmem_int64_atomic_fetch_add_nbi
#define shmem_int64_atomic_fetch_add_nbi pshmem_int64_atomic_fetch_add_nbi
#pragma weak shmem_uint32_atomic_fetch_add_nbi =                               \
    pshmem_uint32_atomic_fetch_add_nbi
#define shmem_uint32_atomic_fetch_add_nbi pshmem_uint32_atomic_fetch_add_nbi
#pragma weak shmem_uint64_atomic_fetch_add_nbi =                               \
    pshmem_uint64_atomic_fetch_add_nbi
#define shmem_uint64_atomic_fetch_add_nbi pshmem_uint64_atomic_fetch_add_nbi
#pragma weak shmem_size_atomic_fetch_add_nbi = pshmem_size_atomic_fetch_add_nbi
#define shmem_size_atomic_fetch_add_nbi pshmem_size_atomic_fetch_add_nbi
#pragma weak shmem_ptrdiff_atomic_fetch_add_nbi =                              \
    pshmem_ptrdiff_atomic_fetch_add_nbi
#define shmem_ptrdiff_atomic_fetch_add_nbi pshmem_ptrdiff_atomic_fetch_add_nbi
#endif /* ENABLE_PSHMEM */

/* Define non-blocking atomic fetch_add operations using the type table */
#define API_DEF_AMO2_NBI_HELPER(_type, _typename)                              \
  API_DEF_AMO2_NBI(fetch_add, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO2_NBI_HELPER)
#undef API_DEF_AMO2_NBI_HELPER

#ifdef ENABLE_PSHMEM
#pragma weak shmem_uint_atomic_fetch_xor = pshmem_uint_atomic_fetch_xor
#define shmem_uint_atomic_fetch_xor pshmem_uint_atomic_fetch_xor
#pragma weak shmem_ulong_atomic_fetch_xor = pshmem_ulong_atomic_fetch_xor
#define shmem_ulong_atomic_fetch_xor pshmem_ulong_atomic_fetch_xor
#pragma weak shmem_ulonglong_atomic_fetch_xor =                                \
    pshmem_ulonglong_atomic_fetch_xor
#define shmem_ulonglong_atomic_fetch_xor pshmem_ulonglong_atomic_fetch_xor
#pragma weak shmem_int32_atomic_fetch_xor = pshmem_int32_atomic_fetch_xor
#define shmem_int32_atomic_fetch_xor pshmem_int32_atomic_fetch_xor
#pragma weak shmem_int64_atomic_fetch_xor = pshmem_int64_atomic_fetch_xor
#define shmem_int64_atomic_fetch_xor pshmem_int64_atomic_fetch_xor
#pragma weak shmem_uint32_atomic_fetch_xor = pshmem_uint32_atomic_fetch_xor
#define shmem_uint32_atomic_fetch_xor pshmem_uint32_atomic_fetch_xor
#pragma weak shmem_uint64_atomic_fetch_xor = pshmem_uint64_atomic_fetch_xor
#define shmem_uint64_atomic_fetch_xor pshmem_uint64_atomic_fetch_xor

#pragma weak shmem_uint_atomic_fetch_or = pshmem_uint_atomic_fetch_or
#define shmem_uint_atomic_fetch_or pshmem_uint_atomic_fetch_or
#pragma weak shmem_ulong_atomic_fetch_or = pshmem_ulong_atomic_fetch_or
#define shmem_ulong_atomic_fetch_or pshmem_ulong_atomic_fetch_or
#pragma weak shmem_ulonglong_atomic_fetch_or = pshmem_ulonglong_atomic_fetch_or
#define shmem_ulonglong_atomic_fetch_or pshmem_ulonglong_atomic_fetch_or
#pragma weak shmem_int32_atomic_fetch_or = pshmem_int32_atomic_fetch_or
#define shmem_int32_atomic_fetch_or pshmem_int32_atomic_fetch_or
#pragma weak shmem_int64_atomic_fetch_or = pshmem_int64_atomic_fetch_or
#define shmem_int64_atomic_fetch_or pshmem_int64_atomic_fetch_or
#pragma weak shmem_uint32_atomic_fetch_or = pshmem_uint32_atomic_fetch_or
#define shmem_uint32_atomic_fetch_or pshmem_uint32_atomic_fetch_or
#pragma weak shmem_uint64_atomic_fetch_or = pshmem_uint64_atomic_fetch_or
#define shmem_uint64_atomic_fetch_or pshmem_uint64_atomic_fetch_or

#pragma weak shmem_uint_atomic_fetch_and = pshmem_uint_atomic_fetch_and
#define shmem_uint_atomic_fetch_and pshmem_uint_atomic_fetch_and
#pragma weak shmem_ulong_atomic_fetch_and = pshmem_ulong_atomic_fetch_and
#define shmem_ulong_atomic_fetch_and pshmem_ulong_atomic_fetch_and
#pragma weak shmem_ulonglong_atomic_fetch_and =                                \
    pshmem_ulonglong_atomic_fetch_and
#define shmem_ulonglong_atomic_fetch_and pshmem_ulonglong_atomic_fetch_and
#pragma weak shmem_int32_atomic_fetch_and = pshmem_int32_atomic_fetch_and
#define shmem_int32_atomic_fetch_and pshmem_int32_atomic_fetch_and
#pragma weak shmem_int64_atomic_fetch_and = pshmem_int64_atomic_fetch_and
#define shmem_int64_atomic_fetch_and pshmem_int64_atomic_fetch_and
#pragma weak shmem_uint32_atomic_fetch_and = pshmem_uint32_atomic_fetch_and
#define shmem_uint32_atomic_fetch_and pshmem_uint32_atomic_fetch_and
#pragma weak shmem_uint64_atomic_fetch_and = pshmem_uint64_atomic_fetch_and
#define shmem_uint64_atomic_fetch_and pshmem_uint64_atomic_fetch_and
#endif /* ENABLE_PSHMEM */

/* Define non-blocking atomic fetch_xor operations using the type table */
#define API_DEF_AMO2_NBI_XOR_HELPER(_type, _typename)                          \
  API_DEF_AMO2_NBI(fetch_xor, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO2_NBI_XOR_HELPER)
#undef API_DEF_AMO2_NBI_XOR_HELPER

/* Define non-blocking atomic fetch_or operations using the type table */
#define API_DEF_AMO2_NBI_OR_HELPER(_type, _typename)                           \
  API_DEF_AMO2_NBI(fetch_or, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO2_NBI_OR_HELPER)
#undef API_DEF_AMO2_NBI_OR_HELPER

/* Define non-blocking atomic fetch_and operations using the type table */
#define API_DEF_AMO2_NBI_AND_HELPER(_type, _typename)                          \
  API_DEF_AMO2_NBI(fetch_and, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO2_NBI_AND_HELPER)
#undef API_DEF_AMO2_NBI_AND_HELPER

#ifdef ENABLE_PSHMEM
#pragma weak shmem_ctx_int_atomic_fetch_add_nbi =                              \
    pshmem_ctx_int_atomic_fetch_add_nbi
#define shmem_ctx_int_atomic_fetch_add_nbi pshmem_ctx_int_atomic_fetch_add_nbi
#pragma weak shmem_ctx_long_atomic_fetch_add_nbi =                             \
    pshmem_ctx_long_atomic_fetch_add_nbi
#define shmem_ctx_long_atomic_fetch_add_nbi pshmem_ctx_long_atomic_fetch_add_nbi
#pragma weak shmem_ctx_longlong_atomic_fetch_add_nbi =                         \
    pshmem_ctx_longlong_atomic_fetch_add_nbi
#define shmem_ctx_longlong_atomic_fetch_add_nbi                                \
  pshmem_ctx_longlong_atomic_fetch_add_nbi
#pragma weak shmem_ctx_uint_atomic_fetch_add_nbi =                             \
    pshmem_ctx_uint_atomic_fetch_add_nbi
#define shmem_ctx_uint_atomic_fetch_add_nbi pshmem_ctx_uint_atomic_fetch_add_nbi
#pragma weak shmem_ctx_ulong_atomic_fetch_add_nbi =                            \
    pshmem_ctx_ulong_atomic_fetch_add_nbi
#define shmem_ctx_ulong_atomic_fetch_add_nbi                                   \
  pshmem_ctx_ulong_atomic_fetch_add_nbi
#pragma weak shmem_ctx_ulonglong_atomic_fetch_add_nbi =                        \
    pshmem_ctx_ulonglong_atomic_fetch_add_nbi
#define shmem_ctx_ulonglong_atomic_fetch_add_nbi                               \
  pshmem_ctx_ulonglong_atomic_fetch_add_nbi
#pragma weak shmem_ctx_int32_atomic_fetch_add_nbi =                            \
    pshmem_ctx_int32_atomic_fetch_add_nbi
#define shmem_ctx_int32_atomic_fetch_add_nbi                                   \
  pshmem_ctx_int32_atomic_fetch_add_nbi
#pragma weak shmem_ctx_int64_atomic_fetch_add_nbi =                            \
    pshmem_ctx_int64_atomic_fetch_add_nbi
#define shmem_ctx_int64_atomic_fetch_add_nbi                                   \
  pshmem_ctx_int64_atomic_fetch_add_nbi
#pragma weak shmem_ctx_uint32_atomic_fetch_add_nbi =                           \
    pshmem_ctx_uint32_atomic_fetch_add_nbi
#define shmem_ctx_uint32_atomic_fetch_add_nbi                                  \
  pshmem_ctx_uint32_atomic_fetch_add_nbi
#pragma weak shmem_ctx_uint64_atomic_fetch_add_nbi =                           \
    pshmem_ctx_uint64_atomic_fetch_add_nbi
#define shmem_ctx_uint64_atomic_fetch_add_nbi                                  \
  pshmem_ctx_uint64_atomic_fetch_add_nbi
#pragma weak shmem_ctx_size_atomic_fetch_add_nbi =                             \
    pshmem_ctx_size_atomic_fetch_add_nbi
#define shmem_ctx_size_atomic_fetch_add_nbi pshmem_ctx_size_atomic_fetch_add_nbi
#pragma weak shmem_ctx_ptrdiff_atomic_fetch_add_nbi =                          \
    pshmem_ctx_ptrdiff_atomic_fetch_add_nbi
#define shmem_ctx_ptrdiff_atomic_fetch_add_nbi                                 \
  pshmem_ctx_ptrdiff_atomic_fetch_add_nbi
#endif /* ENABLE_PSHMEM */

/*
 * fetch-and-add
 */

#undef SHMEM_CTX_TYPE_FADD_NBI

#ifdef ENABLE_PSHMEM
#pragma weak shmem_ctx_int_atomic_fetch_inc_nbi =                              \
    pshmem_ctx_int_atomic_fetch_inc_nbi
#define shmem_ctx_int_atomic_fetch_inc_nbi pshmem_ctx_int_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_long_atomic_fetch_inc_nbi =                             \
    pshmem_ctx_long_atomic_fetch_inc_nbi
#define shmem_ctx_long_atomic_fetch_inc_nbi pshmem_ctx_long_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_longlong_atomic_fetch_inc_nbi =                         \
    pshmem_ctx_longlong_atomic_fetch_inc_nbi
#define shmem_ctx_longlong_atomic_fetch_inc_nbi                                \
  pshmem_ctx_longlong_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_uint_atomic_fetch_inc_nbi =                             \
    pshmem_ctx_uint_atomic_fetch_inc_nbi
#define shmem_ctx_uint_atomic_fetch_inc_nbi pshmem_ctx_uint_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_ulong_atomic_fetch_inc_nbi =                            \
    pshmem_ctx_ulong_atomic_fetch_inc_nbi
#define shmem_ctx_ulong_atomic_fetch_inc_nbi                                   \
  pshmem_ctx_ulong_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_ulonglong_atomic_fetch_inc_nbi =                        \
    pshmem_ctx_ulonglong_atomic_fetch_inc_nbi
#define shmem_ctx_ulonglong_atomic_fetch_inc_nbi                               \
  pshmem_ctx_ulonglong_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_int32_atomic_fetch_inc_nbi =                            \
    pshmem_ctx_int32_atomic_fetch_inc_nbi
#define shmem_ctx_int32_atomic_fetch_inc_nbi                                   \
  pshmem_ctx_int32_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_int64_atomic_fetch_inc_nbi =                            \
    pshmem_ctx_int64_atomic_fetch_inc_nbi
#define shmem_ctx_int64_atomic_fetch_inc_nbi                                   \
  pshmem_ctx_int64_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_uint32_atomic_fetch_inc_nbi =                           \
    pshmem_ctx_uint32_atomic_fetch_inc_nbi
#define shmem_ctx_uint32_atomic_fetch_inc_nbi                                  \
  pshmem_ctx_uint32_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_uint64_atomic_fetch_inc_nbi =                           \
    pshmem_ctx_uint64_atomic_fetch_inc_nbi
#define shmem_ctx_uint64_atomic_fetch_inc_nbi                                  \
  pshmem_ctx_uint64_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_size_atomic_fetch_inc_nbi =                             \
    pshmem_ctx_size_atomic_fetch_inc_nbi
#define shmem_ctx_size_atomic_fetch_inc_nbi pshmem_ctx_size_atomic_fetch_inc_nbi
#pragma weak shmem_ctx_ptrdiff_atomic_fetch_inc_nbi =                          \
    pshmem_ctx_ptrdiff_atomic_fetch_inc_nbi
#define shmem_ctx_ptrdiff_atomic_fetch_inc_nbi                                 \
  pshmem_ctx_ptrdiff_atomic_fetch_inc_nbi
#endif /* ENABLE_PSHMEM */

/*
 * fetch-and-increment
 */

#undef SHMEM_CTX_TYPE_FINC_NBI

#undef SHMEM_CTX_TYPE_FETCH_NBI

#ifdef ENABLE_PSHMEM
#pragma weak shmem_ctx_uint_atomic_fetch_xor_nbi =                             \
    pshmem_ctx_uint_atomic_fetch_xor_nbi
#define shmem_ctx_uint_atomic_fetch_xor_nbi pshmem_ctx_uint_atomic_fetch_xor_nbi
#pragma weak shmem_ctx_ulong_atomic_fetch_xor_nbi =                            \
    pshmem_ctx_ulong_atomic_fetch_xor_nbi
#define shmem_ctx_ulong_atomic_fetch_xor_nbi                                   \
  pshmem_ctx_ulong_atomic_fetch_xor_nbi
#pragma weak shmem_ctx_ulonglong_atomic_fetch_xor_nbi =                        \
    pshmem_ctx_ulonglong_atomic_fetch_xor_nbi
#define shmem_ctx_ulonglong_atomic_fetch_xor_nbi                               \
  pshmem_ctx_ulonglong_atomic_fetch_xor_nbi
#pragma weak shmem_ctx_int32_atomic_fetch_xor_nbi =                            \
    pshmem_ctx_int32_atomic_fetch_xor_nbi
#define shmem_ctx_int32_atomic_fetch_xor_nbi                                   \
  pshmem_ctx_int32_atomic_fetch_xor_nbi
#pragma weak shmem_ctx_int64_atomic_fetch_xor_nbi =                            \
    pshmem_ctx_int64_atomic_fetch_xor_nbi
#define shmem_ctx_int64_atomic_fetch_xor_nbi                                   \
  pshmem_ctx_int64_atomic_fetch_xor_nbi
#pragma weak shmem_ctx_uint32_atomic_fetch_xor_nbi =                           \
    pshmem_ctx_uint32_atomic_fetch_xor_nbi
#define shmem_ctx_uint32_atomic_fetch_xor_nbi                                  \
  pshmem_ctx_uint32_atomic_fetch_xor_nbi
#pragma weak shmem_ctx_uint64_atomic_fetch_xor_nbi =                           \
    pshmem_ctx_uint64_atomic_fetch_xor_nbi
#define shmem_ctx_uint64_atomic_fetch_xor_nbi                                  \
  pshmem_ctx_uint64_atomic_fetch_xor_nbi
#pragma weak shmem_ctx_uint_atomic_fetch_or_nbi =                              \
    pshmem_ctx_uint_atomic_fetch_or_nbi
#define shmem_ctx_uint_atomic_fetch_or_nbi pshmem_ctx_uint_atomic_fetch_or_nbi
#pragma weak shmem_ctx_ulong_atomic_fetch_or_nbi =                             \
    pshmem_ctx_ulong_atomic_fetch_or_nbi
#define shmem_ctx_ulong_atomic_fetch_or_nbi pshmem_ctx_ulong_atomic_fetch_or_nbi
#pragma weak shmem_ctx_ulonglong_atomic_fetch_or_nbi =                         \
    pshmem_ctx_ulonglong_atomic_fetch_or_nbi
#define shmem_ctx_ulonglong_atomic_fetch_or_nbi                                \
  pshmem_ctx_ulonglong_atomic_fetch_or_nbi
#pragma weak shmem_ctx_int32_atomic_fetch_or_nbi =                             \
    pshmem_ctx_int32_atomic_fetch_or_nbi
#define shmem_ctx_int32_atomic_fetch_or_nbi pshmem_ctx_int32_atomic_fetch_or_nbi
#pragma weak shmem_ctx_int64_atomic_fetch_or_nbi =                             \
    pshmem_ctx_int64_atomic_fetch_or_nbi
#define shmem_ctx_int64_atomic_fetch_or_nbi pshmem_ctx_int64_atomic_fetch_or_nbi
#pragma weak shmem_ctx_uint32_atomic_fetch_or_nbi =                            \
    pshmem_ctx_uint32_atomic_fetch_or_nbi
#define shmem_ctx_uint32_atomic_fetch_or_nbi                                   \
  pshmem_ctx_uint32_atomic_fetch_or_nbi
#pragma weak shmem_ctx_uint64_atomic_fetch_or_nbi =                            \
    pshmem_ctx_uint64_atomic_fetch_or_nbi
#define shmem_ctx_uint64_atomic_fetch_or_nbi                                   \
  pshmem_ctx_uint64_atomic_fetch_or_nbi
#pragma weak shmem_ctx_uint_atomic_fetch_and_nbi =                             \
    pshmem_ctx_uint_atomic_fetch_and_nbi
#define shmem_ctx_uint_atomic_fetch_and_nbi pshmem_ctx_uint_atomic_fetch_and_nbi
#pragma weak shmem_ctx_ulong_atomic_fetch_and_nbi =                            \
    pshmem_ctx_ulong_atomic_fetch_and_nbi
#define shmem_ctx_ulong_atomic_fetch_and_nbi                                   \
  pshmem_ctx_ulong_atomic_fetch_and_nbi
#pragma weak shmem_ctx_ulonglong_atomic_fetch_and_nbi =                        \
    pshmem_ctx_ulonglong_atomic_fetch_and_nbi
#define shmem_ctx_ulonglong_atomic_fetch_and_nbi                               \
  pshmem_ctx_ulonglong_atomic_fetch_and_nbi
#pragma weak shmem_ctx_int32_atomic_fetch_and_nbi =                            \
    pshmem_ctx_int32_atomic_fetch_and_nbi
#define shmem_ctx_int32_atomic_fetch_and_nbi                                   \
  pshmem_ctx_int32_atomic_fetch_and_nbi
#pragma weak shmem_ctx_int64_atomic_fetch_and_nbi =                            \
    pshmem_ctx_int64_atomic_fetch_and_nbi
#define shmem_ctx_int64_atomic_fetch_and_nbi                                   \
  pshmem_ctx_int64_atomic_fetch_and_nbi
#pragma weak shmem_ctx_uint32_atomic_fetch_and_nbi =                           \
    pshmem_ctx_uint32_atomic_fetch_and_nbi
#define shmem_ctx_uint32_atomic_fetch_and_nbi                                  \
  pshmem_ctx_uint32_atomic_fetch_and_nbi
#pragma weak shmem_ctx_uint64_atomic_fetch_and_nbi =                           \
    pshmem_ctx_uint64_atomic_fetch_and_nbi
#define shmem_ctx_uint64_atomic_fetch_and_nbi                                  \
  pshmem_ctx_uint64_atomic_fetch_and_nbi
#endif /* ENABLE_PSHMEM */

/*
 * fetch-bitwise NBI
 */
/**
 * @brief Macro to define non-blocking atomic fetch-and-bitwise operations
 *
 * @param _opname The bitwise operation name (xor, or, and)
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that atomically performs a bitwise operation on a remote
 * variable and returns the old value in a non-blocking manner. The operation is
 * performed without protecting the mutex.
 */
#define SHMEM_CTX_TYPE_FETCH_BITWISE_NBI(_opname, _name, _type)                \
  void shmem_ctx_##_name##_atomic_fetch_##_opname##_nbi(                       \
      shmem_ctx_t ctx, _type *fetch, _type *target, _type value, int pe) {     \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_fetch_##_opname(                         \
        ctx, target, &value, sizeof(value), pe, fetch));                       \
  }

/* Define context-based non-blocking atomic fetch_xor operations using the type
 * table */
#define SHMEM_CTX_TYPE_FETCH_XOR_NBI_HELPER(_type, _typename)                  \
  SHMEM_CTX_TYPE_FETCH_BITWISE_NBI(xor, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FETCH_XOR_NBI_HELPER)
#undef SHMEM_CTX_TYPE_FETCH_XOR_NBI_HELPER

/* Define context-based non-blocking atomic fetch_or operations using the type
 * table */
#define SHMEM_CTX_TYPE_FETCH_OR_NBI_HELPER(_type, _typename)                   \
  SHMEM_CTX_TYPE_FETCH_BITWISE_NBI(or, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FETCH_OR_NBI_HELPER)
#undef SHMEM_CTX_TYPE_FETCH_OR_NBI_HELPER

/* Define context-based non-blocking atomic fetch_and operations using the type
 * table */
#define SHMEM_CTX_TYPE_FETCH_AND_NBI_HELPER(_type, _typename)                  \
  SHMEM_CTX_TYPE_FETCH_BITWISE_NBI(and, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FETCH_AND_NBI_HELPER)
#undef SHMEM_CTX_TYPE_FETCH_AND_NBI_HELPER

#undef SHMEM_CTX_TYPE_FETCH_BITWISE_NBI
