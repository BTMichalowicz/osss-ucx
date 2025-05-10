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

SHMEM_CTX_TYPE_FETCH_BITWISE(or, uint, unsigned int)
SHMEM_CTX_TYPE_FETCH_BITWISE(or, ulong, unsigned long)
SHMEM_CTX_TYPE_FETCH_BITWISE(or, ulonglong, unsigned long long)
SHMEM_CTX_TYPE_FETCH_BITWISE(or, int32, int32_t)
SHMEM_CTX_TYPE_FETCH_BITWISE(or, int64, int64_t)
SHMEM_CTX_TYPE_FETCH_BITWISE(or, uint32, uint32_t)
SHMEM_CTX_TYPE_FETCH_BITWISE(or, uint64, uint64_t)

/**
 * @brief Defines the API for atomic fetch-or operations
 *
 * These macros create the public API functions for atomic fetch-or operations
 * for different integer types. Each function performs a fetch-or operation
 * without a context.
 */

API_DEF_AMO2(fetch_or, uint, unsigned int)
API_DEF_AMO2(fetch_or, ulong, unsigned long)
API_DEF_AMO2(fetch_or, ulonglong, unsigned long long)
API_DEF_AMO2(fetch_or, int32, int32_t)
API_DEF_AMO2(fetch_or, int64, int64_t)
API_DEF_AMO2(fetch_or, uint32, uint32_t)
API_DEF_AMO2(fetch_or, uint64, uint64_t)
