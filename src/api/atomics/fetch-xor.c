/**
 * @file fetch-xor.c
 * @brief Implementation of SHMEM atomic fetch-and-xor operations
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
#pragma weak shmem_ctx_uint_atomic_fetch_xor = pshmem_ctx_uint_atomic_fetch_xor
#define shmem_ctx_uint_atomic_fetch_xor pshmem_ctx_uint_atomic_fetch_xor
#pragma weak shmem_ctx_ulong_atomic_fetch_xor =                                \
    pshmem_ctx_ulong_atomic_fetch_xor
#define shmem_ctx_ulong_atomic_fetch_xor pshmem_ctx_ulong_atomic_fetch_xor
#pragma weak shmem_ctx_ulonglong_atomic_fetch_xor =                            \
    pshmem_ctx_ulonglong_atomic_fetch_xor
#define shmem_ctx_ulonglong_atomic_fetch_xor                                   \
  pshmem_ctx_ulonglong_atomic_fetch_xor
#pragma weak shmem_ctx_int32_atomic_fetch_xor =                                \
    pshmem_ctx_int32_atomic_fetch_xor
#define shmem_ctx_int32_atomic_fetch_xor pshmem_ctx_int32_atomic_fetch_xor
#pragma weak shmem_ctx_int64_atomic_fetch_xor =                                \
    pshmem_ctx_int64_atomic_fetch_xor
#define shmem_ctx_int64_atomic_fetch_xor pshmem_ctx_int64_atomic_fetch_xor
#pragma weak shmem_ctx_uint32_atomic_fetch_xor =                               \
    pshmem_ctx_uint32_atomic_fetch_xor
#define shmem_ctx_uint32_atomic_fetch_xor pshmem_ctx_uint32_atomic_fetch_xor
#pragma weak shmem_ctx_uint64_atomic_fetch_xor =                               \
    pshmem_ctx_uint64_atomic_fetch_xor
#define shmem_ctx_uint64_atomic_fetch_xor pshmem_ctx_uint64_atomic_fetch_xor
#endif /* ENABLE_PSHMEM */

/**
 * @brief Atomic fetch-and-xor operations for different integer types
 *
 * These routines perform an atomic fetch-and-xor operation. An atomic
 * fetch-and-xor performs an XOR operation between the target and value
 * and returns the value that had previously been at the target address.
 */

/* Define context-based atomic fetch-xor operations using the type table */
#define SHMEM_CTX_TYPE_FETCH_BITWISE_HELPER(_type, _typename)                  \
  SHMEM_CTX_TYPE_FETCH_BITWISE(xor, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FETCH_BITWISE_HELPER)
#undef SHMEM_CTX_TYPE_FETCH_BITWISE_HELPER

/* Define non-context atomic fetch-xor operations using the type table */
#define API_DEF_AMO2_HELPER(_type, _typename)                                  \
  API_DEF_AMO2(fetch_xor, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(API_DEF_AMO2_HELPER)
#undef API_DEF_AMO2_HELPER
