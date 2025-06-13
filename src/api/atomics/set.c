/**
 * @file set.c
 * @brief Implementation of SHMEM atomic set operations
 *
 * For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmem_mutex.h"
#include "shmemu.h"
#include "shmemc.h"
#include "common.h"
#include <shmem/api_types.h>

#ifdef ENABLE_PSHMEM
#pragma weak shmem_ctx_int_atomic_set = pshmem_ctx_int_atomic_set
#define shmem_ctx_int_atomic_set pshmem_ctx_int_atomic_set
#pragma weak shmem_ctx_long_atomic_set = pshmem_ctx_long_atomic_set
#define shmem_ctx_long_atomic_set pshmem_ctx_long_atomic_set
#pragma weak shmem_ctx_longlong_atomic_set = pshmem_ctx_longlong_atomic_set
#define shmem_ctx_longlong_atomic_set pshmem_ctx_longlong_atomic_set
#pragma weak shmem_ctx_float_atomic_set = pshmem_ctx_float_atomic_set
#define shmem_ctx_float_atomic_set pshmem_ctx_float_atomic_set
#pragma weak shmem_ctx_double_atomic_set = pshmem_ctx_double_atomic_set
#define shmem_ctx_double_atomic_set pshmem_ctx_double_atomic_set
#pragma weak shmem_ctx_uint_atomic_set = pshmem_ctx_uint_atomic_set
#define shmem_ctx_uint_atomic_set pshmem_ctx_uint_atomic_set
#pragma weak shmem_ctx_ulong_atomic_set = pshmem_ctx_ulong_atomic_set
#define shmem_ctx_ulong_atomic_set pshmem_ctx_ulong_atomic_set
#pragma weak shmem_ctx_ulonglong_atomic_set = pshmem_ctx_ulonglong_atomic_set
#define shmem_ctx_ulonglong_atomic_set pshmem_ctx_ulonglong_atomic_set
#pragma weak shmem_ctx_int32_atomic_set = pshmem_ctx_int32_atomic_set
#define shmem_ctx_int32_atomic_set pshmem_ctx_int32_atomic_set
#pragma weak shmem_ctx_int64_atomic_set = pshmem_ctx_int64_atomic_set
#define shmem_ctx_int64_atomic_set pshmem_ctx_int64_atomic_set
#pragma weak shmem_ctx_uint32_atomic_set = pshmem_ctx_uint32_atomic_set
#define shmem_ctx_uint32_atomic_set pshmem_ctx_uint32_atomic_set
#pragma weak shmem_ctx_uint64_atomic_set = pshmem_ctx_uint64_atomic_set
#define shmem_ctx_uint64_atomic_set pshmem_ctx_uint64_atomic_set
#pragma weak shmem_ctx_size_atomic_set = pshmem_ctx_size_atomic_set
#define shmem_ctx_size_atomic_set pshmem_ctx_size_atomic_set
#pragma weak shmem_ctx_ptrdiff_atomic_set = pshmem_ctx_ptrdiff_atomic_set
#define shmem_ctx_ptrdiff_atomic_set pshmem_ctx_ptrdiff_atomic_set
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to define atomic set operations for different types
 *
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that performs an atomic set operation.
 * The operation atomically sets the value of a remote variable.
 * The operation is performed without protecting the mutex.
 */
#define SHMEM_CTX_TYPE_SET(_name, _type)                                       \
  void shmem_ctx_##_name##_atomic_set(shmem_ctx_t ctx, _type *target,          \
                                      _type value, int pe) {                   \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_set(ctx, target, sizeof(*target),        \
                                          &value, sizeof(value), pe));         \
  }

/* Define context-based atomic set operations using the type table */
#define SHMEM_CTX_TYPE_SET_HELPER(_type, _typename)                            \
  SHMEM_CTX_TYPE_SET(_typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_SET_HELPER)
#undef SHMEM_CTX_TYPE_SET_HELPER

/**
 * @brief Defines the API for atomic set operations
 *
 * These macros create the public API functions for atomic set operations
 * for different types. Each function performs a set operation without a
 * context.
 */
/* Define non-context atomic set operations using the type table */
#define API_DEF_VOID_AMO2_HELPER(_type, _typename)                             \
  API_DEF_VOID_AMO2(set, _typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(API_DEF_VOID_AMO2_HELPER)
#undef API_DEF_VOID_AMO2_HELPER
