/**
 * @file fetch-and-nbi.c
 * @brief Implementation of SHMEM non-blocking atomic fetch-and-and operations
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
#pragma weak shmem_uint_atomic_fetch_and_nbi = pshmem_uint_atomic_fetch_and_nbi
#define shmem_uint_atomic_fetch_and_nbi pshmem_uint_atomic_fetch_and_nbi
#pragma weak shmem_ulong_atomic_fetch_and_nbi =                                \
    pshmem_ulong_atomic_fetch_and_nbi
#define shmem_ulong_atomic_fetch_and_nbi pshmem_ulong_atomic_fetch_and_nbi
#pragma weak shmem_ulonglong_atomic_fetch_and_nbi =                            \
    pshmem_ulonglong_atomic_fetch_and_nbi
#define shmem_ulonglong_atomic_fetch_and_nbi                                   \
  pshmem_ulonglong_atomic_fetch_and_nbi
#pragma weak shmem_int32_atomic_fetch_and_nbi =                                \
    pshmem_int32_atomic_fetch_and_nbi
#define shmem_int32_atomic_fetch_and_nbi pshmem_int32_atomic_fetch_and_nbi
#pragma weak shmem_int64_atomic_fetch_and_nbi =                                \
    pshmem_int64_atomic_fetch_and_nbi
#define shmem_int64_atomic_fetch_and_nbi pshmem_int64_atomic_fetch_and_nbi
#pragma weak shmem_uint32_atomic_fetch_and_nbi =                               \
    pshmem_uint32_atomic_fetch_and_nbi
#define shmem_uint32_atomic_fetch_and_nbi pshmem_uint32_atomic_fetch_and_nbi
#pragma weak shmem_uint64_atomic_fetch_and_nbi =                               \
    pshmem_uint64_atomic_fetch_and_nbi
#define shmem_uint64_atomic_fetch_and_nbi pshmem_uint64_atomic_fetch_and_nbi
#endif /* ENABLE_PSHMEM */

/**
 * @brief Non-blocking atomic fetch-and-and operations for different integer
 * types
 *
 * These operations perform a non-blocking atomic fetch-and-and on a remote PE.
 * The operations fetch the old value of the target and perform a bitwise AND
 * with the provided value, storing the result back to the target location.
 * The old value is returned asynchronously.
 */

/* Define context-based non-blocking atomic fetch-and operations using the type
 * table */
#define SHMEM_CTX_TYPE_FETCH_BITWISE_NBI_HELPER(_type, _typename)              \
  SHMEM_CTX_TYPE_FETCH_BITWISE_NBI(and, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FETCH_BITWISE_NBI_HELPER)
#undef SHMEM_CTX_TYPE_FETCH_BITWISE_NBI_HELPER

/**
 * @brief Default context non-blocking atomic fetch-and-and operations
 *
 * These operations perform the same functionality as the context-based
 * operations above but use the default SHMEM context.
 */

/* Define non-context non-blocking atomic fetch-and operations using the type
 * table */
#define API_DEF_AMO2_NBI_HELPER(_type, _typename)                              \
  API_DEF_AMO2_NBI(fetch_and, _typename, _type)
SHMEM_BITWISE_AMO_TYPE_TABLE(API_DEF_AMO2_NBI_HELPER)
#undef API_DEF_AMO2_NBI_HELPER
