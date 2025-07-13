/**
 * @file swap.c
 * @brief Implementation of SHMEM atomic swap operations
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
#pragma weak shmem_ctx_float_atomic_swap = pshmem_ctx_float_atomic_swap
#define shmem_ctx_float_atomic_swap pshmem_ctx_float_atomic_swap
#pragma weak shmem_ctx_double_atomic_swap = pshmem_ctx_double_atomic_swap
#define shmem_ctx_double_atomic_swap pshmem_ctx_double_atomic_swap
#pragma weak shmem_ctx_int_atomic_swap = pshmem_ctx_int_atomic_swap
#define shmem_ctx_int_atomic_swap pshmem_ctx_int_atomic_swap
#pragma weak shmem_ctx_long_atomic_swap = pshmem_ctx_long_atomic_swap
#define shmem_ctx_long_atomic_swap pshmem_ctx_long_atomic_swap
#pragma weak shmem_ctx_longlong_atomic_swap = pshmem_ctx_longlong_atomic_swap
#define shmem_ctx_longlong_atomic_swap pshmem_ctx_longlong_atomic_swap
#pragma weak shmem_ctx_uint_atomic_swap = pshmem_ctx_uint_atomic_swap
#define shmem_ctx_uint_atomic_swap pshmem_ctx_uint_atomic_swap
#pragma weak shmem_ctx_ulong_atomic_swap = pshmem_ctx_ulong_atomic_swap
#define shmem_ctx_ulong_atomic_swap pshmem_ctx_ulong_atomic_swap
#pragma weak shmem_ctx_ulonglong_atomic_swap = pshmem_ctx_ulonglong_atomic_swap
#define shmem_ctx_ulonglong_atomic_swap pshmem_ctx_ulonglong_atomic_swap
#pragma weak shmem_ctx_int32_atomic_swap = pshmem_ctx_int32_atomic_swap
#define shmem_ctx_int32_atomic_swap pshmem_ctx_int32_atomic_swap
#pragma weak shmem_ctx_int64_atomic_swap = pshmem_ctx_int64_atomic_swap
#define shmem_ctx_int64_atomic_swap pshmem_ctx_int64_atomic_swap
#pragma weak shmem_ctx_uint32_atomic_swap = pshmem_ctx_uint32_atomic_swap
#define shmem_ctx_uint32_atomic_swap pshmem_ctx_uint32_atomic_swap
#pragma weak shmem_ctx_uint64_atomic_swap = pshmem_ctx_uint64_atomic_swap
#define shmem_ctx_uint64_atomic_swap pshmem_ctx_uint64_atomic_swap
#pragma weak shmem_ctx_size_atomic_swap = pshmem_ctx_size_atomic_swap
#define shmem_ctx_size_atomic_swap pshmem_ctx_size_atomic_swap
#pragma weak shmem_ctx_ptrdiff_atomic_swap = pshmem_ctx_ptrdiff_atomic_swap
#define shmem_ctx_ptrdiff_atomic_swap pshmem_ctx_ptrdiff_atomic_swap
#endif /* ENABLE_PSHMEM */

/**
 * @brief Atomic swap operations for different types
 *
 * These routines perform an atomic swap operation. An atomic swap writes
 * the value to the target address on the specified PE and returns the
 * previous contents of the target as an atomic operation.
 *
 * @param _name The type name suffix for the function
 * @param _type The actual C type for the operation
 */
#define SHMEM_CTX_TYPE_SWAP(_name, _type)                                      \
  _type shmem_ctx_##_name##_atomic_swap(shmem_ctx_t ctx, _type *target,        \
                                        _type value, int pe) {                 \
    _type v;                                                                   \
                                                                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_SYMMETRIC(target, 2);                                         \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_swap(ctx, target, &value, sizeof(value), pe, &v));          \
    return v;                                                                  \
  }

/* Define context-based atomic swap operations using the type table */
#define SHMEM_CTX_TYPE_SWAP_HELPER(_type, _typename)                           \
  SHMEM_CTX_TYPE_SWAP(_typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_SWAP_HELPER)
#undef SHMEM_CTX_TYPE_SWAP_HELPER

/**
 * @brief Defines the API for atomic swap operations
 *
 * These macros create the public API functions for atomic swap operations
 * for different types. Each function performs a swap operation without a
 * context.
 */
/* Define non-context atomic swap operations using the type table */
#define API_DEF_AMO2_HELPER(_type, _typename)                                  \
  API_DEF_AMO2(swap, _typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(API_DEF_AMO2_HELPER)
#undef API_DEF_AMO2_HELPER
