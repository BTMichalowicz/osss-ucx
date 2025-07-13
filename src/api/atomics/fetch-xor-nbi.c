/**
 * @file fetch-xor-nbi.c
 * @brief Implementation of SHMEM non-blocking atomic fetch-and-xor operations
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
#pragma weak shmem_uint_atomic_fetch_xor_nbi = pshmem_uint_atomic_fetch_xor_nbi
#define shmem_uint_atomic_fetch_xor_nbi pshmem_uint_atomic_fetch_xor_nbi
#pragma weak shmem_ulong_atomic_fetch_xor_nbi =                                \
    pshmem_ulong_atomic_fetch_xor_nbi
#define shmem_ulong_atomic_fetch_xor_nbi pshmem_ulong_atomic_fetch_xor_nbi
#pragma weak shmem_ulonglong_atomic_fetch_xor_nbi =                            \
    pshmem_ulonglong_atomic_fetch_xor_nbi
#define shmem_ulonglong_atomic_fetch_xor_nbi                                   \
  pshmem_ulonglong_atomic_fetch_xor_nbi
#pragma weak shmem_int32_atomic_fetch_xor_nbi =                                \
    pshmem_int32_atomic_fetch_xor_nbi
#define shmem_int32_atomic_fetch_xor_nbi pshmem_int32_atomic_fetch_xor_nbi
#pragma weak shmem_int64_atomic_fetch_xor_nbi =                                \
    pshmem_int64_atomic_fetch_xor_nbi
#define shmem_int64_atomic_fetch_xor_nbi pshmem_int64_atomic_fetch_xor_nbi
#pragma weak shmem_uint32_atomic_fetch_xor_nbi =                               \
    pshmem_uint32_atomic_fetch_xor_nbi
#define shmem_uint32_atomic_fetch_xor_nbi pshmem_uint32_atomic_fetch_xor_nbi
#pragma weak shmem_uint64_atomic_fetch_xor_nbi =                               \
    pshmem_uint64_atomic_fetch_xor_nbi
#define shmem_uint64_atomic_fetch_xor_nbi pshmem_uint64_atomic_fetch_xor_nbi
#endif /* ENABLE_PSHMEM */

/**
 * @brief Non-blocking atomic fetch-and-xor operations with contexts
 *
 * The fetch-and-xor routines perform an atomic bitwise XOR operation
 * on a remote data object and return the previous value in a non-blocking
 * manner.
 */

/* Define context-based non-blocking atomic fetch-xor operations using the type
 * table */
#define SHMEM_CTX_TYPE_FETCH_BITWISE_NBI_HELPER(_type, _typename)              \
  SHMEM_CTX_TYPE_FETCH_BITWISE_NBI(xor, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FETCH_BITWISE_NBI_HELPER)
#undef SHMEM_CTX_TYPE_FETCH_BITWISE_NBI_HELPER

/**
 * @brief Non-blocking atomic fetch-and-xor operations without contexts
 *
 * These routines perform an atomic bitwise XOR operation on a remote data
 * object and return the previous value in a non-blocking manner, using the
 * default context.
 */
/* Define non-context non-blocking atomic fetch-xor operations using the type
 * table */
#define API_DEF_AMO2_NBI_HELPER(_type, _typename)                              \
  API_DEF_AMO2_NBI(fetch_xor, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(API_DEF_AMO2_NBI_HELPER)
#undef API_DEF_AMO2_NBI_HELPER
