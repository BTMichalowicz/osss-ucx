/**
 * @file fetch-add.c
 * @brief Implementation of SHMEM atomic fetch-and-add operations
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
#pragma weak shmem_ctx_int_atomic_fetch_add = pshmem_ctx_int_atomic_fetch_add
#define shmem_ctx_int_atomic_fetch_add pshmem_ctx_int_atomic_fetch_add
#pragma weak shmem_ctx_long_atomic_fetch_add = pshmem_ctx_long_atomic_fetch_add
#define shmem_ctx_long_atomic_fetch_add pshmem_ctx_long_atomic_fetch_add
#pragma weak shmem_ctx_longlong_atomic_fetch_add =                             \
    pshmem_ctx_longlong_atomic_fetch_add
#define shmem_ctx_longlong_atomic_fetch_add pshmem_ctx_longlong_atomic_fetch_add
#pragma weak shmem_ctx_uint_atomic_fetch_add = pshmem_ctx_uint_atomic_fetch_add
#define shmem_ctx_uint_atomic_fetch_add pshmem_ctx_uint_atomic_fetch_add
#pragma weak shmem_ctx_ulong_atomic_fetch_add =                                \
    pshmem_ctx_ulong_atomic_fetch_add
#define shmem_ctx_ulong_atomic_fetch_add pshmem_ctx_ulong_atomic_fetch_add
#pragma weak shmem_ctx_ulonglong_atomic_fetch_add =                            \
    pshmem_ctx_ulonglong_atomic_fetch_add
#define shmem_ctx_ulonglong_atomic_fetch_add                                   \
  pshmem_ctx_ulonglong_atomic_fetch_add
#pragma weak shmem_ctx_int32_atomic_fetch_add =                                \
    pshmem_ctx_int32_atomic_fetch_add
#define shmem_ctx_int32_atomic_fetch_add pshmem_ctx_int32_atomic_fetch_add
#pragma weak shmem_ctx_int64_atomic_fetch_add =                                \
    pshmem_ctx_int64_atomic_fetch_add
#define shmem_ctx_int64_atomic_fetch_add pshmem_ctx_int64_atomic_fetch_add
#pragma weak shmem_ctx_uint32_atomic_fetch_add =                               \
    pshmem_ctx_uint32_atomic_fetch_add
#define shmem_ctx_uint32_atomic_fetch_add pshmem_ctx_uint32_atomic_fetch_add
#pragma weak shmem_ctx_uint64_atomic_fetch_add =                               \
    pshmem_ctx_uint64_atomic_fetch_add
#define shmem_ctx_uint64_atomic_fetch_add pshmem_ctx_uint64_atomic_fetch_add
#pragma weak shmem_ctx_size_atomic_fetch_add = pshmem_ctx_size_atomic_fetch_add
#define shmem_ctx_size_atomic_fetch_add pshmem_ctx_size_atomic_fetch_add
#pragma weak shmem_ctx_ptrdiff_atomic_fetch_add =                              \
    pshmem_ctx_ptrdiff_atomic_fetch_add
#define shmem_ctx_ptrdiff_atomic_fetch_add pshmem_ctx_ptrdiff_atomic_fetch_add
#endif /* ENABLE_PSHMEM */

/*
 * fetch-and-add
 */

/**
 * @brief Macro to define atomic fetch-and-add operations with contexts
 *
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that atomically adds a value to a remote variable and
 * returns the previous value. The operation is performed using the specified
 * context.
 */
#define SHMEM_CTX_TYPE_FADD(_name, _type)                                      \
  _type shmem_ctx_##_name##_atomic_fetch_add(shmem_ctx_t ctx, _type *target,   \
                                             _type value, int pe) {            \
    _type v;                                                                   \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_fadd(ctx, target, &value, sizeof(value), pe, &v));          \
    return v;                                                                  \
  }

/* Define context-based atomic fetch-and-add operations using the type table */
#define SHMEM_CTX_TYPE_FADD_HELPER(_type, _typename)                           \
  SHMEM_CTX_TYPE_FADD(_typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_FADD_HELPER)
#undef SHMEM_CTX_TYPE_FADD_HELPER

/* Define non-context atomic fetch-and-add operations using the type table */
#define API_DEF_AMO2_HELPER(_type, _typename)                                  \
  API_DEF_AMO2(fetch_add, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO2_HELPER)
#undef API_DEF_AMO2_HELPER
