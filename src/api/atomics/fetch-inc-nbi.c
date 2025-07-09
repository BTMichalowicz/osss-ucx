/**
 * @file fetch-inc-nbi.c
 * @brief Implementation of SHMEM non-blocking atomic fetch-and-increment
 * operations
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
#pragma weak shmem_int_atomic_fetch_inc_nbi = pshmem_int_atomic_fetch_inc_nbi
#define shmem_int_atomic_fetch_inc_nbi pshmem_int_atomic_fetch_inc_nbi
#pragma weak shmem_long_atomic_fetch_inc_nbi = pshmem_long_atomic_fetch_inc_nbi
#define shmem_long_atomic_fetch_inc_nbi pshmem_long_atomic_fetch_inc_nbi
#pragma weak shmem_longlong_atomic_fetch_inc_nbi =                             \
    pshmem_longlong_atomic_fetch_inc_nbi
#define shmem_longlong_atomic_fetch_inc_nbi pshmem_longlong_atomic_fetch_inc_nbi
#pragma weak shmem_uint_atomic_fetch_inc_nbi = pshmem_uint_atomic_fetch_inc_nbi
#define shmem_uint_atomic_fetch_inc_nbi pshmem_uint_atomic_fetch_inc_nbi
#pragma weak shmem_ulong_atomic_fetch_inc_nbi =                                \
    pshmem_ulong_atomic_fetch_inc_nbi
#define shmem_ulong_atomic_fetch_inc_nbi pshmem_ulong_atomic_fetch_inc_nbi
#pragma weak shmem_ulonglong_atomic_fetch_inc_nbi =                            \
    pshmem_ulonglong_atomic_fetch_inc_nbi
#define shmem_ulonglong_atomic_fetch_inc_nbi                                   \
  pshmem_ulonglong_atomic_fetch_inc_nbi
#pragma weak shmem_int32_atomic_fetch_inc_nbi =                                \
    pshmem_int32_atomic_fetch_inc_nbi
#define shmem_int32_atomic_fetch_inc_nbi pshmem_int32_atomic_fetch_inc_nbi
#pragma weak shmem_int64_atomic_fetch_inc_nbi =                                \
    pshmem_int64_atomic_fetch_inc_nbi
#define shmem_int64_atomic_fetch_inc_nbi pshmem_int64_atomic_fetch_inc_nbi
#pragma weak shmem_uint32_atomic_fetch_inc_nbi =                               \
    pshmem_uint32_atomic_fetch_inc_nbi
#define shmem_uint32_atomic_fetch_inc_nbi pshmem_uint32_atomic_fetch_inc_nbi
#pragma weak shmem_uint64_atomic_fetch_inc_nbi =                               \
    pshmem_uint64_atomic_fetch_inc_nbi
#define shmem_uint64_atomic_fetch_inc_nbi pshmem_uint64_atomic_fetch_inc_nbi
#pragma weak shmem_size_atomic_fetch_inc_nbi = pshmem_size_atomic_fetch_inc_nbi
#define shmem_size_atomic_fetch_inc_nbi pshmem_size_atomic_fetch_inc_nbi
#pragma weak shmem_ptrdiff_atomic_fetch_inc_nbi =                              \
    pshmem_ptrdiff_atomic_fetch_inc_nbi
#define shmem_ptrdiff_atomic_fetch_inc_nbi pshmem_ptrdiff_atomic_fetch_inc_nbi
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to define non-blocking atomic fetch-and-increment operations
 *
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that atomically increments a remote variable by 1 and
 * returns the old value in a non-blocking manner. The operation is performed
 * without protecting the mutex.
 */
#define SHMEM_CTX_TYPE_FINC_NBI(_name, _type)                                  \
  void shmem_ctx_##_name##_atomic_fetch_inc_nbi(shmem_ctx_t ctx, _type *fetch, \
                                                _type *target, int pe) {       \
    _type one = 1;                                                             \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_fadd(ctx, target, &one, sizeof(one), pe, fetch));           \
  }

/* Define context-based non-blocking atomic fetch-and-increment operations using
 * the type table */
#define SHMEM_CTX_TYPE_FINC_NBI_HELPER(_type, _typename)                       \
  SHMEM_CTX_TYPE_FINC_NBI(_typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FINC_NBI_HELPER)
#undef SHMEM_CTX_TYPE_FINC_NBI_HELPER

/* Define non-context non-blocking atomic fetch-and-increment operations using
 * the type table */
#define API_DEF_AMO1_NBI_HELPER(_type, _typename)                              \
  API_DEF_AMO1_NBI(fetch_inc, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO1_NBI_HELPER)
#undef API_DEF_AMO1_NBI_HELPER
