/**
 * @file and.c
 * @brief Implementation of SHMEM atomic AND operations
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
#pragma weak shmem_ctx_uint_atomic_and = pshmem_ctx_uint_atomic_and
#define shmem_ctx_uint_atomic_and pshmem_ctx_uint_atomic_and
#pragma weak shmem_ctx_ulong_atomic_and = pshmem_ctx_ulong_atomic_and
#define shmem_ctx_ulong_atomic_and pshmem_ctx_ulong_atomic_and
#pragma weak shmem_ctx_ulonglong_atomic_and = pshmem_ctx_ulonglong_atomic_and
#define shmem_ctx_ulonglong_atomic_and pshmem_ctx_ulonglong_atomic_and
#pragma weak shmem_ctx_int32_atomic_and = pshmem_ctx_int32_atomic_and
#define shmem_ctx_int32_atomic_and pshmem_ctx_int32_atomic_and
#pragma weak shmem_ctx_int64_atomic_and = pshmem_ctx_int64_atomic_and
#define shmem_ctx_int64_atomic_and pshmem_ctx_int64_atomic_and
#pragma weak shmem_ctx_uint32_atomic_and = pshmem_ctx_uint32_atomic_and
#define shmem_ctx_uint32_atomic_and pshmem_ctx_uint32_atomic_and
#pragma weak shmem_ctx_uint64_atomic_and = pshmem_ctx_uint64_atomic_and
#define shmem_ctx_uint64_atomic_and pshmem_ctx_uint64_atomic_and
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to define atomic AND operations for different types
 *
 * @param _typename Type name string
 * @param _type Data type
 *
 * Defines a function that atomically performs AND operation on a remote
 * variable
 */
#define SHMEM_CTX_TYPE_AND(_typename, _type)                                   \
  void shmem_ctx_##_typename##_atomic_and(shmem_ctx_t ctx, _type *target,      \
                                          _type value, int pe) {               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_and(ctx, target, &value, sizeof(value), pe));               \
  }

/* Define atomic AND operations for different types using the type table */
#define SHMEM_CTX_TYPE_AND_HELPER(_type, _typename)                            \
  SHMEM_CTX_TYPE_AND(_typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_AND_HELPER)
#undef SHMEM_CTX_TYPE_AND_HELPER

/* Define non-context atomic AND operations */
#define API_DEF_VOID_AMO2_HELPER(_type, _typename)                             \
  API_DEF_VOID_AMO2(and, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_VOID_AMO2_HELPER)
#undef API_DEF_VOID_AMO2_HELPER
