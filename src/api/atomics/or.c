/**
 * @file or.c
 * @brief Implementation of SHMEM atomic OR operations
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
#pragma weak shmem_ctx_uint_atomic_or = pshmem_ctx_uint_atomic_or
#define shmem_ctx_uint_atomic_or pshmem_ctx_uint_atomic_or
#pragma weak shmem_ctx_ulong_atomic_or = pshmem_ctx_ulong_atomic_or
#define shmem_ctx_ulong_atomic_or pshmem_ctx_ulong_atomic_or
#pragma weak shmem_ctx_ulonglong_atomic_or = pshmem_ctx_ulonglong_atomic_or
#define shmem_ctx_ulonglong_atomic_or pshmem_ctx_ulonglong_atomic_or
#pragma weak shmem_ctx_int32_atomic_or = pshmem_ctx_int32_atomic_or
#define shmem_ctx_int32_atomic_or pshmem_ctx_int32_atomic_or
#pragma weak shmem_ctx_int64_atomic_or = pshmem_ctx_int64_atomic_or
#define shmem_ctx_int64_atomic_or pshmem_ctx_int64_atomic_or
#pragma weak shmem_ctx_uint32_atomic_or = pshmem_ctx_uint32_atomic_or
#define shmem_ctx_uint32_atomic_or pshmem_ctx_uint32_atomic_or
#pragma weak shmem_ctx_uint64_atomic_or = pshmem_ctx_uint64_atomic_or
#define shmem_ctx_uint64_atomic_or pshmem_ctx_uint64_atomic_or
#endif /* ENABLE_PSHMEM */

/**
 * @brief Atomic OR operations for different integer types
 *
 * These routines perform an atomic bitwise OR operation. An atomic OR
 * performs a bitwise OR between the target and value without the possibility
 * of another process updating the target during the operation.
 */
SHMEM_CTX_TYPE_BITWISE(or, uint, unsigned int)
SHMEM_CTX_TYPE_BITWISE(or, ulong, unsigned long)
SHMEM_CTX_TYPE_BITWISE(or, ulonglong, unsigned long long)
SHMEM_CTX_TYPE_BITWISE(or, int32, int32_t)
SHMEM_CTX_TYPE_BITWISE(or, int64, int64_t)
SHMEM_CTX_TYPE_BITWISE(or, uint32, uint32_t)
SHMEM_CTX_TYPE_BITWISE(or, uint64, uint64_t)

/**
 * @brief Defines the API for atomic OR operations
 *
 * These macros create the public API functions for atomic OR operations
 * for different integer types. Each function performs an OR operation
 * without a context.
 */
API_DEF_VOID_AMO2(or, uint, unsigned int)
API_DEF_VOID_AMO2(or, ulong, unsigned long)
API_DEF_VOID_AMO2(or, ulonglong, unsigned long long)
API_DEF_VOID_AMO2(or, int32, int32_t)
API_DEF_VOID_AMO2(or, int64, int64_t)
API_DEF_VOID_AMO2(or, uint32, uint32_t)
API_DEF_VOID_AMO2(or, uint64, uint64_t)
