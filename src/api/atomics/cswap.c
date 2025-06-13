/**
 * @file cswap.c
 * @brief Implementation of SHMEM atomic compare-and-swap operations
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
#pragma weak shmem_int_atomic_compare_swap = pshmem_int_atomic_compare_swap
#define shmem_int_atomic_compare_swap pshmem_int_atomic_compare_swap
#pragma weak shmem_long_atomic_compare_swap = pshmem_long_atomic_compare_swap
#define shmem_long_atomic_compare_swap pshmem_long_atomic_compare_swap
#pragma weak shmem_longlong_atomic_compare_swap =                              \
    pshmem_longlong_atomic_compare_swap
#define shmem_longlong_atomic_compare_swap pshmem_longlong_atomic_compare_swap
#pragma weak shmem_uint_atomic_compare_swap = pshmem_uint_atomic_compare_swap
#define shmem_uint_atomic_compare_swap pshmem_uint_atomic_compare_swap
#pragma weak shmem_ulong_atomic_compare_swap = pshmem_ulong_atomic_compare_swap
#define shmem_ulong_atomic_compare_swap pshmem_ulong_atomic_compare_swap
#pragma weak shmem_ulonglong_atomic_compare_swap =                             \
    pshmem_ulonglong_atomic_compare_swap
#define shmem_ulonglong_atomic_compare_swap pshmem_ulonglong_atomic_compare_swap
#pragma weak shmem_int32_atomic_compare_swap = pshmem_int32_atomic_compare_swap
#define shmem_int32_atomic_compare_swap pshmem_int32_atomic_compare_swap
#pragma weak shmem_int64_atomic_compare_swap = pshmem_int64_atomic_compare_swap
#define shmem_int64_atomic_compare_swap pshmem_int64_atomic_compare_swap
#pragma weak shmem_uint32_atomic_compare_swap =                                \
    pshmem_uint32_atomic_compare_swap
#define shmem_uint32_atomic_compare_swap pshmem_uint32_atomic_compare_swap
#pragma weak shmem_uint64_atomic_compare_swap =                                \
    pshmem_uint64_atomic_compare_swap
#define shmem_uint64_atomic_compare_swap pshmem_uint64_atomic_compare_swap
#pragma weak shmem_size_atomic_compare_swap = pshmem_size_atomic_compare_swap
#define shmem_size_atomic_compare_swap pshmem_size_atomic_compare_swap
#pragma weak shmem_ptrdiff_atomic_compare_swap =                               \
    pshmem_ptrdiff_atomic_compare_swap
#define shmem_ptrdiff_atomic_compare_swap pshmem_ptrdiff_atomic_compare_swap
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to define atomic compare-and-swap operations
 *
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that atomically compares a value with a condition and
 * swaps it with a new value if they match. The operation is performed without
 * protecting the mutex.
 */
#define SHMEM_CTX_TYPE_CSWAP(_name, _type)                                     \
  _type shmem_ctx_##_name##_atomic_compare_swap(                               \
      shmem_ctx_t ctx, _type *target, _type cond, _type value, int pe) {       \
    _type v;                                                                   \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_cswap(ctx, target, &cond, &value, sizeof(value), pe, &v));  \
    return v;                                                                  \
  }

/* Define context-based atomic compare-and-swap operations using the type table
 */
#define SHMEM_CTX_TYPE_CSWAP_HELPER(_type, _typename)                          \
  SHMEM_CTX_TYPE_CSWAP(_typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_CSWAP_HELPER)
#undef SHMEM_CTX_TYPE_CSWAP_HELPER

/* Define non-context atomic compare-and-swap operations using the type table */
#define API_DEF_AMO3_HELPER(_type, _typename)                                  \
  API_DEF_AMO3(compare_swap, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO3_HELPER)
#undef API_DEF_AMO3_HELPER
