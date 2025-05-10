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
SHMEM_CTX_TYPE_FETCH_BITWISE(xor, uint, unsigned int)
SHMEM_CTX_TYPE_FETCH_BITWISE(xor, ulong, unsigned long)
SHMEM_CTX_TYPE_FETCH_BITWISE(xor, ulonglong, unsigned long long)
SHMEM_CTX_TYPE_FETCH_BITWISE(xor, int32, int32_t)
SHMEM_CTX_TYPE_FETCH_BITWISE(xor, int64, int64_t)
SHMEM_CTX_TYPE_FETCH_BITWISE(xor, uint32, uint32_t)
SHMEM_CTX_TYPE_FETCH_BITWISE(xor, uint64, uint64_t)

API_DEF_AMO2(fetch_xor, uint, unsigned int)
API_DEF_AMO2(fetch_xor, ulong, unsigned long)
API_DEF_AMO2(fetch_xor, ulonglong, unsigned long long)
API_DEF_AMO2(fetch_xor, int32, int32_t)
API_DEF_AMO2(fetch_xor, int64, int64_t)
API_DEF_AMO2(fetch_xor, uint32, uint32_t)
API_DEF_AMO2(fetch_xor, uint64, uint64_t)
