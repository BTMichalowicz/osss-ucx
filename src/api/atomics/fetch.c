/**
 * @file fetch.c
 * @brief Implementation of SHMEM atomic fetch operations
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
#pragma weak shmem_ctx_int_atomic_fetch = pshmem_ctx_int_atomic_fetch
#define shmem_ctx_int_atomic_fetch pshmem_ctx_int_atomic_fetch
#pragma weak shmem_ctx_long_atomic_fetch = pshmem_ctx_long_atomic_fetch
#define shmem_ctx_long_atomic_fetch pshmem_ctx_long_atomic_fetch
#pragma weak shmem_ctx_longlong_atomic_fetch = pshmem_ctx_longlong_atomic_fetch
#define shmem_ctx_longlong_atomic_fetch pshmem_ctx_longlong_atomic_fetch
#pragma weak shmem_ctx_float_atomic_fetch = pshmem_ctx_float_atomic_fetch
#define shmem_ctx_float_atomic_fetch pshmem_ctx_float_atomic_fetch
#pragma weak shmem_ctx_double_atomic_fetch = pshmem_ctx_double_atomic_fetch
#define shmem_ctx_double_atomic_fetch pshmem_ctx_double_atomic_fetch
#pragma weak shmem_ctx_uint_atomic_fetch = pshmem_ctx_uint_atomic_fetch
#define shmem_ctx_uint_atomic_fetch pshmem_ctx_uint_atomic_fetch
#pragma weak shmem_ctx_ulong_atomic_fetch = pshmem_ctx_ulong_atomic_fetch
#define shmem_ctx_ulong_atomic_fetch pshmem_ctx_ulong_atomic_fetch
#pragma weak shmem_ctx_ulonglong_atomic_fetch =                                \
    pshmem_ctx_ulonglong_atomic_fetch
#define shmem_ctx_ulonglong_atomic_fetch pshmem_ctx_ulonglong_atomic_fetch
#pragma weak shmem_ctx_int32_atomic_fetch = pshmem_ctx_int32_atomic_fetch
#define shmem_ctx_int32_atomic_fetch pshmem_ctx_int32_atomic_fetch
#pragma weak shmem_ctx_int64_atomic_fetch = pshmem_ctx_int64_atomic_fetch
#define shmem_ctx_int64_atomic_fetch pshmem_ctx_int64_atomic_fetch
#pragma weak shmem_ctx_uint32_atomic_fetch = pshmem_ctx_uint32_atomic_fetch
#define shmem_ctx_uint32_atomic_fetch pshmem_ctx_uint32_atomic_fetch
#pragma weak shmem_ctx_uint64_atomic_fetch = pshmem_ctx_uint64_atomic_fetch
#define shmem_ctx_uint64_atomic_fetch pshmem_ctx_uint64_atomic_fetch
#pragma weak shmem_ctx_size_atomic_fetch = pshmem_ctx_size_atomic_fetch
#define shmem_ctx_size_atomic_fetch pshmem_ctx_size_atomic_fetch
#pragma weak shmem_ctx_ptrdiff_atomic_fetch = pshmem_ctx_ptrdiff_atomic_fetch
#define shmem_ctx_ptrdiff_atomic_fetch pshmem_ctx_ptrdiff_atomic_fetch
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to define atomic fetch operations for different types
 *
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that performs an atomic fetch operation.
 * The operation retrieves the value of a remote variable atomically.
 * The operation is performed without protecting the mutex.
 */
#define SHMEM_CTX_TYPE_FETCH(_name, _type)                                     \
  _type shmem_ctx_##_name##_atomic_fetch(shmem_ctx_t ctx, const _type *target, \
                                         int pe) {                             \
    _type v;                                                                   \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_fetch(ctx, (_type *)target, sizeof(*target), pe, &v));      \
    return v;                                                                  \
  }

/* Define context-based atomic fetch operations using the type table */
#define SHMEM_CTX_TYPE_FETCH_HELPER(_type, _typename)                          \
  SHMEM_CTX_TYPE_FETCH(_typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FETCH_HELPER)
#undef SHMEM_CTX_TYPE_FETCH_HELPER

/* Define non-context atomic fetch operations using the type table */
#define API_DEF_CONST_AMO1_HELPER(_type, _typename)                            \
  API_DEF_CONST_AMO1(fetch, _typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(API_DEF_CONST_AMO1_HELPER)
#undef API_DEF_CONST_AMO1_HELPER
