/**
 * @file fetch-nbi.c
 * @brief Implementation of SHMEM non-blocking atomic fetch operations
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
#pragma weak shmem_ctx_int_atomic_fetch_nbi = pshmem_ctx_int_atomic_fetch_nbi
#define shmem_ctx_int_atomic_fetch_nbi pshmem_ctx_int_atomic_fetch_nbi
#pragma weak shmem_ctx_long_atomic_fetch_nbi = pshmem_ctx_long_atomic_fetch_nbi
#define shmem_ctx_long_atomic_fetch_nbi pshmem_ctx_long_atomic_fetch_nbi
#pragma weak shmem_ctx_longlong_atomic_fetch_nbi =                             \
    pshmem_ctx_longlong_atomic_fetch_nbi
#define shmem_ctx_longlong_atomic_fetch_nbi pshmem_ctx_longlong_atomic_fetch_nbi
#pragma weak shmem_ctx_float_atomic_fetch_nbi =                                \
    pshmem_ctx_float_atomic_fetch_nbi
#define shmem_ctx_float_atomic_fetch_nbi pshmem_ctx_float_atomic_fetch_nbi
#pragma weak shmem_ctx_double_atomic_fetch_nbi =                               \
    pshmem_ctx_double_atomic_fetch_nbi
#define shmem_ctx_double_atomic_fetch_nbi pshmem_ctx_double_atomic_fetch_nbi
#pragma weak shmem_ctx_uint_atomic_fetch_nbi = pshmem_ctx_uint_atomic_fetch_nbi
#define shmem_ctx_uint_atomic_fetch_nbi pshmem_ctx_uint_atomic_fetch_nbi
#pragma weak shmem_ctx_ulong_atomic_fetch_nbi =                                \
    pshmem_ctx_ulong_atomic_fetch_nbi
#define shmem_ctx_ulong_atomic_fetch_nbi pshmem_ctx_ulong_atomic_fetch_nbi
#pragma weak shmem_ctx_ulonglong_atomic_fetch_nbi =                            \
    pshmem_ctx_ulonglong_atomic_fetch_nbi
#define shmem_ctx_ulonglong_atomic_fetch_nbi                                   \
  pshmem_ctx_ulonglong_atomic_fetch_nbi
#pragma weak shmem_ctx_int32_atomic_fetch_nbi =                                \
    pshmem_ctx_int32_atomic_fetch_nbi
#define shmem_ctx_int32_atomic_fetch_nbi pshmem_ctx_int32_atomic_fetch_nbi
#pragma weak shmem_ctx_int64_atomic_fetch_nbi =                                \
    pshmem_ctx_int64_atomic_fetch_nbi
#define shmem_ctx_int64_atomic_fetch_nbi pshmem_ctx_int64_atomic_fetch_nbi
#pragma weak shmem_ctx_uint32_atomic_fetch_nbi =                               \
    pshmem_ctx_uint32_atomic_fetch_nbi
#define shmem_ctx_uint32_atomic_fetch_nbi pshmem_ctx_uint32_atomic_fetch_nbi
#pragma weak shmem_ctx_uint64_atomic_fetch_nbi =                               \
    pshmem_ctx_uint64_atomic_fetch_nbi
#define shmem_ctx_uint64_atomic_fetch_nbi pshmem_ctx_uint64_atomic_fetch_nbi
#pragma weak shmem_ctx_size_atomic_fetch_nbi = pshmem_ctx_size_atomic_fetch_nbi
#define shmem_ctx_size_atomic_fetch_nbi pshmem_ctx_size_atomic_fetch_nbi
#pragma weak shmem_ctx_ptrdiff_atomic_fetch_nbi =                              \
    pshmem_ctx_ptrdiff_atomic_fetch_nbi
#define shmem_ctx_ptrdiff_atomic_fetch_nbi pshmem_ctx_ptrdiff_atomic_fetch_nbi
#endif /* ENABLE_PSHMEM */

/*
 * NB currently using blocking as first hack
 */

/**
 * @brief Macro to define non-blocking atomic fetch operations
 *
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that performs a non-blocking atomic fetch operation.
 * The operation retrieves the value of a remote variable without blocking.
 * The operation is performed without protecting the mutex.
 *
 * Note: Currently implemented using blocking operations as a first hack.
 */
#define SHMEM_CTX_TYPE_FETCH_NBI(_name, _type)                                 \
  void shmem_ctx_##_name##_atomic_fetch_nbi(shmem_ctx_t ctx, _type *fetch,     \
                                            const _type *target, int pe) {     \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_fetch(ctx, (_type *)target, sizeof(*target), pe, fetch));   \
  }

/* Define context-based non-blocking atomic fetch operations using the type
 * table */
#define SHMEM_CTX_TYPE_FETCH_NBI_HELPER(_type, _typename)                      \
  SHMEM_CTX_TYPE_FETCH_NBI(_typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FETCH_NBI_HELPER)
#undef SHMEM_CTX_TYPE_FETCH_NBI_HELPER

/**
 * @brief Default context non-blocking atomic fetch operations
 *
 * These operations perform the same functionality as the context-based
 * operations above but use the default SHMEM context.
 */
/* Define non-context non-blocking atomic fetch operations using the type table
 */
#define API_DEF_CONST_AMO1_NBI_HELPER(_type, _typename)                        \
  API_DEF_CONST_AMO1_NBI(fetch, _typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(API_DEF_CONST_AMO1_NBI_HELPER)
#undef API_DEF_CONST_AMO1_NBI_HELPER
