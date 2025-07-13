/**
 * @file fetch-or.c
 * @brief Implementation of SHMEM atomic fetch-and-or operations
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
#pragma weak shmem_ctx_uint_atomic_fetch_or = pshmem_ctx_uint_atomic_fetch_or
#define shmem_ctx_uint_atomic_fetch_or pshmem_ctx_uint_atomic_fetch_or
#pragma weak shmem_ctx_ulong_atomic_fetch_or = pshmem_ctx_ulong_atomic_fetch_or
#define shmem_ctx_ulong_atomic_fetch_or pshmem_ctx_ulong_atomic_fetch_or
#pragma weak shmem_ctx_ulonglong_atomic_fetch_or =                             \
    pshmem_ctx_ulonglong_atomic_fetch_or
#define shmem_ctx_ulonglong_atomic_fetch_or pshmem_ctx_ulonglong_atomic_fetch_or
#pragma weak shmem_ctx_int32_atomic_fetch_or = pshmem_ctx_int32_atomic_fetch_or
#define shmem_ctx_int32_atomic_fetch_or pshmem_ctx_int32_atomic_fetch_or
#pragma weak shmem_ctx_int64_atomic_fetch_or = pshmem_ctx_int64_atomic_fetch_or
#define shmem_ctx_int64_atomic_fetch_or pshmem_ctx_int64_atomic_fetch_or
#pragma weak shmem_ctx_uint32_atomic_fetch_or =                                \
    pshmem_ctx_uint32_atomic_fetch_or
#define shmem_ctx_uint32_atomic_fetch_or pshmem_ctx_uint32_atomic_fetch_or
#pragma weak shmem_ctx_uint64_atomic_fetch_or =                                \
    pshmem_ctx_uint64_atomic_fetch_or
#define shmem_ctx_uint64_atomic_fetch_or pshmem_ctx_uint64_atomic_fetch_or
#endif /* ENABLE_PSHMEM */

/**
 * @brief Implements atomic fetch-or operations for different integer types
 *
 * These macros define atomic fetch-or operations that perform a bitwise OR
 * between a value and a remote variable, returning the original value of the
 * remote variable. The operations are performed without protecting the mutex.
 */

/* Define context-based atomic fetch-or operations using the type table */
#define SHMEM_CTX_TYPE_FETCH_BITWISE_HELPER(_type, _typename)                  \
  SHMEM_CTX_TYPE_FETCH_BITWISE(or, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FETCH_BITWISE_HELPER)
#undef SHMEM_CTX_TYPE_FETCH_BITWISE_HELPER

/**
 * @brief Defines the API for atomic fetch-or operations
 *
 * These macros create the public API functions for atomic fetch-or operations
 * for different integer types. Each function performs a fetch-or operation
 * without a context.
 */
/* Define non-context atomic fetch-or operations using the type table */
#define API_DEF_AMO2_HELPER(_type, _typename)                                  \
  API_DEF_AMO2(fetch_or, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(API_DEF_AMO2_HELPER)
#undef API_DEF_AMO2_HELPER
