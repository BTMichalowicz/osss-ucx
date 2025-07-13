/**
 * @file add.c
 * @brief Implementation of SHMEM atomic add operations
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
#pragma weak shmem_ctx_int_atomic_add = pshmem_ctx_int_atomic_add
#define shmem_ctx_int_atomic_add pshmem_ctx_int_atomic_add
#pragma weak shmem_ctx_long_atomic_add = pshmem_ctx_long_atomic_add
#define shmem_ctx_long_atomic_add pshmem_ctx_long_atomic_add
#pragma weak shmem_ctx_longlong_atomic_add = pshmem_ctx_longlong_atomic_add
#define shmem_ctx_longlong_atomic_add pshmem_ctx_longlong_atomic_add
#pragma weak shmem_ctx_uint_atomic_add = pshmem_ctx_uint_atomic_add
#define shmem_ctx_uint_atomic_add pshmem_ctx_uint_atomic_add
#pragma weak shmem_ctx_ulong_atomic_add = pshmem_ctx_ulong_atomic_add
#define shmem_ctx_ulong_atomic_add pshmem_ctx_ulong_atomic_add
#pragma weak shmem_ctx_ulonglong_atomic_add = pshmem_ctx_ulonglong_atomic_add
#define shmem_ctx_ulonglong_atomic_add pshmem_ctx_ulonglong_atomic_add
#pragma weak shmem_ctx_int32_atomic_add = pshmem_ctx_int32_atomic_add
#define shmem_ctx_int32_atomic_add pshmem_ctx_int32_atomic_add
#pragma weak shmem_ctx_int64_atomic_add = pshmem_ctx_int64_atomic_add
#define shmem_ctx_int64_atomic_add pshmem_ctx_int64_atomic_add
#pragma weak shmem_ctx_uint32_atomic_add = pshmem_ctx_uint32_atomic_add
#define shmem_ctx_uint32_atomic_add pshmem_ctx_uint32_atomic_add
#pragma weak shmem_ctx_uint64_atomic_add = pshmem_ctx_uint64_atomic_add
#define shmem_ctx_uint64_atomic_add pshmem_ctx_uint64_atomic_add
#pragma weak shmem_ctx_size_atomic_add = pshmem_ctx_size_atomic_add
#define shmem_ctx_size_atomic_add pshmem_ctx_size_atomic_add
#pragma weak shmem_ctx_ptrdiff_atomic_add = pshmem_ctx_ptrdiff_atomic_add
#define shmem_ctx_ptrdiff_atomic_add pshmem_ctx_ptrdiff_atomic_add
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to define atomic add operations for different types
 *
 * @param _typename Type name string
 * @param _type Data type
 *
 * Defines a function that atomically adds a value to a remote variable
 */
#define SHMEM_CTX_TYPE_ADD(_typename, _type)                                   \
  void shmem_ctx_##_typename##_atomic_add(shmem_ctx_t ctx, _type *target,      \
                                          _type value, int pe) {               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_add(ctx, target, &value, sizeof(value), pe));               \
  }

/* Define atomic add operations for different types */
#define SHMEM_CTX_TYPE_ADD_HELPER(_type, _typename)                            \
  SHMEM_CTX_TYPE_ADD(_typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_ADD_HELPER)
#undef SHMEM_CTX_TYPE_ADD_HELPER

/* Define non-context atomic add operations */
#define API_DEF_VOID_AMO2_HELPER(_type, _typename)                             \
  API_DEF_VOID_AMO2(add, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_VOID_AMO2_HELPER)
#undef API_DEF_VOID_AMO2_HELPER