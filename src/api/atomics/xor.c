/**
 * @file xor.c
 * @brief Implementation of SHMEM atomic XOR operations
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
#pragma weak shmem_ctx_uint_atomic_xor = pshmem_ctx_uint_atomic_xor
#define shmem_ctx_uint_atomic_xor pshmem_ctx_uint_atomic_xor
#pragma weak shmem_ctx_ulong_atomic_xor = pshmem_ctx_ulong_atomic_xor
#define shmem_ctx_ulong_atomic_xor pshmem_ctx_ulong_atomic_xor
#pragma weak shmem_ctx_ulonglong_atomic_xor = pshmem_ctx_ulonglong_atomic_xor
#define shmem_ctx_ulonglong_atomic_xor pshmem_ctx_ulonglong_atomic_xor
#pragma weak shmem_ctx_int32_atomic_xor = pshmem_ctx_int32_atomic_xor
#define shmem_ctx_int32_atomic_xor pshmem_ctx_int32_atomic_xor
#pragma weak shmem_ctx_int64_atomic_xor = pshmem_ctx_int64_atomic_xor
#define shmem_ctx_int64_atomic_xor pshmem_ctx_int64_atomic_xor
#pragma weak shmem_ctx_uint32_atomic_xor = pshmem_ctx_uint32_atomic_xor
#define shmem_ctx_uint32_atomic_xor pshmem_ctx_uint32_atomic_xor
#pragma weak shmem_ctx_uint64_atomic_xor = pshmem_ctx_uint64_atomic_xor
#define shmem_ctx_uint64_atomic_xor pshmem_ctx_uint64_atomic_xor
#endif /* ENABLE_PSHMEM */

/**
 * @brief Atomic XOR operations for different integer types
 *
 * These routines perform an atomic bitwise XOR operation. An atomic XOR
 * performs a bitwise XOR between the target and value without the possibility
 * of another process updating the target during the operation.
 */

/* Define context-based atomic XOR operations using the type table */
#define SHMEM_CTX_TYPE_BITWISE_HELPER(_type, _typename)                        \
  SHMEM_CTX_TYPE_BITWISE(xor, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_BITWISE_HELPER)
#undef SHMEM_CTX_TYPE_BITWISE_HELPER

/**
 * @brief Defines the API for atomic XOR operations
 *
 * These macros create the public API functions for atomic XOR operations
 * for different integer types. Each function performs a XOR operation
 * without a context.
 */
/* Define non-context atomic XOR operations using the type table */
#define API_DEF_VOID_AMO2_HELPER(_type, _typename)                             \
  API_DEF_VOID_AMO2(xor, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(API_DEF_VOID_AMO2_HELPER)
#undef API_DEF_VOID_AMO2_HELPER
