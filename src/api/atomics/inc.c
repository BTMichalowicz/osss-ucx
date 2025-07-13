/**
 * @file inc.c
 * @brief Implementation of SHMEM atomic increment operations
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
#pragma weak shmem_ctx_int_atomic_inc = pshmem_ctx_int_atomic_inc
#define shmem_ctx_int_atomic_inc pshmem_ctx_int_atomic_inc
#pragma weak shmem_ctx_long_atomic_inc = pshmem_ctx_long_atomic_inc
#define shmem_ctx_long_atomic_inc pshmem_ctx_long_atomic_inc
#pragma weak shmem_ctx_longlong_atomic_inc = pshmem_ctx_longlong_atomic_inc
#define shmem_ctx_longlong_atomic_inc pshmem_ctx_longlong_atomic_inc
#pragma weak shmem_ctx_uint_atomic_inc = pshmem_ctx_uint_atomic_inc
#define shmem_ctx_uint_atomic_inc pshmem_ctx_uint_atomic_inc
#pragma weak shmem_ctx_ulong_atomic_inc = pshmem_ctx_ulong_atomic_inc
#define shmem_ctx_ulong_atomic_inc pshmem_ctx_ulong_atomic_inc
#pragma weak shmem_ctx_ulonglong_atomic_inc = pshmem_ctx_ulonglong_atomic_inc
#define shmem_ctx_ulonglong_atomic_inc pshmem_ctx_ulonglong_atomic_inc
#pragma weak shmem_ctx_int32_atomic_inc = pshmem_ctx_int32_atomic_inc
#define shmem_ctx_int32_atomic_inc pshmem_ctx_int32_atomic_inc
#pragma weak shmem_ctx_int64_atomic_inc = pshmem_ctx_int64_atomic_inc
#define shmem_ctx_int64_atomic_inc pshmem_ctx_int64_atomic_inc
#pragma weak shmem_ctx_uint32_atomic_inc = pshmem_ctx_uint32_atomic_inc
#define shmem_ctx_uint32_atomic_inc pshmem_ctx_uint32_atomic_inc
#pragma weak shmem_ctx_uint64_atomic_inc = pshmem_ctx_uint64_atomic_inc
#define shmem_ctx_uint64_atomic_inc pshmem_ctx_uint64_atomic_inc
#pragma weak shmem_ctx_size_atomic_inc = pshmem_ctx_size_atomic_inc
#define shmem_ctx_size_atomic_inc pshmem_ctx_size_atomic_inc
#pragma weak shmem_ctx_ptrdiff_atomic_inc = pshmem_ctx_ptrdiff_atomic_inc
#define shmem_ctx_ptrdiff_atomic_inc pshmem_ctx_ptrdiff_atomic_inc
#endif /* ENABLE_PSHMEM */

/**
 * @brief Implements atomic increment operations for different integer types
 *
 * These macros define atomic increment operations that atomically increment
 * a remote variable by 1. The operations are performed without protecting
 * the mutex.
 *
 * @param _name The type name suffix for the function
 * @param _type The actual C type for the operation
 */
#define SHMEM_CTX_TYPE_INC(_name, _type)                                       \
  void shmem_ctx_##_name##_atomic_inc(shmem_ctx_t ctx, _type *target,          \
                                      int pe) {                                \
    _type one = 1;                                                             \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_add(ctx, target, &one, sizeof(one), pe));                   \
  }

/* Define context-based atomic increment operations using the type table */
#define SHMEM_CTX_TYPE_INC_HELPER(_type, _typename)                            \
  SHMEM_CTX_TYPE_INC(_typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_INC_HELPER)
#undef SHMEM_CTX_TYPE_INC_HELPER

/**
 * @brief Defines the API for atomic increment operations
 *
 * These macros create the public API functions for atomic increment operations
 * for different integer types. Each function performs an increment operation
 * without a context.
 */
/* Define non-context atomic increment operations using the type table */
#define API_DEF_VOID_AMO1_HELPER(_type, _typename)                             \
  API_DEF_VOID_AMO1(inc, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_VOID_AMO1_HELPER)
#undef API_DEF_VOID_AMO1_HELPER
