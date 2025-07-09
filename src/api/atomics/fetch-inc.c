/**
 * @file fetch-inc.c
 * @brief Implementation of SHMEM atomic fetch-and-increment operations
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
#pragma weak shmem_ctx_int_atomic_fetch_inc = pshmem_ctx_int_atomic_fetch_inc
#define shmem_ctx_int_atomic_fetch_inc pshmem_ctx_int_atomic_fetch_inc
#pragma weak shmem_ctx_long_atomic_fetch_inc = pshmem_ctx_long_atomic_fetch_inc
#define shmem_ctx_long_atomic_fetch_inc pshmem_ctx_long_atomic_fetch_inc
#pragma weak shmem_ctx_longlong_atomic_fetch_inc =                             \
    pshmem_ctx_longlong_atomic_fetch_inc
#define shmem_ctx_longlong_atomic_fetch_inc pshmem_ctx_longlong_atomic_fetch_inc
#pragma weak shmem_ctx_uint_atomic_fetch_inc = pshmem_ctx_uint_atomic_fetch_inc
#define shmem_ctx_uint_atomic_fetch_inc pshmem_ctx_uint_atomic_fetch_inc
#pragma weak shmem_ctx_ulong_atomic_fetch_inc =                                \
    pshmem_ctx_ulong_atomic_fetch_inc
#define shmem_ctx_ulong_atomic_fetch_inc pshmem_ctx_ulong_atomic_fetch_inc
#pragma weak shmem_ctx_ulonglong_atomic_fetch_inc =                            \
    pshmem_ctx_ulonglong_atomic_fetch_inc
#define shmem_ctx_ulonglong_atomic_fetch_inc                                   \
  pshmem_ctx_ulonglong_atomic_fetch_inc
#pragma weak shmem_ctx_int32_atomic_fetch_inc =                                \
    pshmem_ctx_int32_atomic_fetch_inc
#define shmem_ctx_int32_atomic_fetch_inc pshmem_ctx_int32_atomic_fetch_inc
#pragma weak shmem_ctx_int64_atomic_fetch_inc =                                \
    pshmem_ctx_int64_atomic_fetch_inc
#define shmem_ctx_int64_atomic_fetch_inc pshmem_ctx_int64_atomic_fetch_inc
#pragma weak shmem_ctx_uint32_atomic_fetch_inc =                               \
    pshmem_ctx_uint32_atomic_fetch_inc
#define shmem_ctx_uint32_atomic_fetch_inc pshmem_ctx_uint32_atomic_fetch_inc
#pragma weak shmem_ctx_uint64_atomic_fetch_inc =                               \
    pshmem_ctx_uint64_atomic_fetch_inc
#define shmem_ctx_uint64_atomic_fetch_inc pshmem_ctx_uint64_atomic_fetch_inc
#pragma weak shmem_ctx_size_atomic_fetch_inc = pshmem_ctx_size_atomic_fetch_inc
#define shmem_ctx_size_atomic_fetch_inc pshmem_ctx_size_atomic_fetch_inc
#pragma weak shmem_ctx_ptrdiff_atomic_fetch_inc =                              \
    pshmem_ctx_ptrdiff_atomic_fetch_inc
#define shmem_ctx_ptrdiff_atomic_fetch_inc pshmem_ctx_ptrdiff_atomic_fetch_inc
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to define atomic fetch-and-increment operations with contexts
 *
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that atomically increments a remote variable by 1 and
 * returns the previous value. The operation is performed using the specified
 * context without protecting the mutex.
 */
#define SHMEM_CTX_TYPE_FINC(_name, _type)                                      \
  _type shmem_ctx_##_name##_atomic_fetch_inc(shmem_ctx_t ctx, _type *target,   \
                                             int pe) {                         \
    _type one = 1;                                                             \
    _type v;                                                                   \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_fadd(ctx, target, &one, sizeof(one), pe, &v));              \
    return v;                                                                  \
  }

/* Define context-based atomic fetch-and-increment operations using the type
 * table */
#define SHMEM_CTX_TYPE_FINC_HELPER(_type, _typename)                           \
  SHMEM_CTX_TYPE_FINC(_typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FINC_HELPER)
#undef SHMEM_CTX_TYPE_FINC_HELPER

/**
 * @brief Default context atomic fetch-and-increment operations
 *
 * These operations perform the same functionality as the context-based
 * operations above but use the default SHMEM context.
 */
/* Define non-context atomic fetch-and-increment operations using the type table
 */
#define API_DEF_AMO1_HELPER(_type, _typename)                                  \
  API_DEF_AMO1(fetch_inc, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO1_HELPER)
#undef API_DEF_AMO1_HELPER
