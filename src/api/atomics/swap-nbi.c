/**
 * @file swap-nbi.c
 * @brief Implementation of SHMEM non-blocking atomic swap operations
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
#pragma weak shmem_ctx_float_atomic_swap_nbi = pshmem_ctx_float_atomic_swap_nbi
#define shmem_ctx_float_atomic_swap_nbi pshmem_ctx_float_atomic_swap_nbi
#pragma weak shmem_ctx_double_atomic_swap_nbi =                                \
    pshmem_ctx_double_atomic_swap_nbi
#define shmem_ctx_double_atomic_swap_nbi pshmem_ctx_double_atomic_swap_nbi
#pragma weak shmem_ctx_int_atomic_swap_nbi = pshmem_ctx_int_atomic_swap_nbi
#define shmem_ctx_int_atomic_swap_nbi pshmem_ctx_int_atomic_swap_nbi
#pragma weak shmem_ctx_long_atomic_swap_nbi = pshmem_ctx_long_atomic_swap_nbi
#define shmem_ctx_long_atomic_swap_nbi pshmem_ctx_long_atomic_swap_nbi
#pragma weak shmem_ctx_longlong_atomic_swap_nbi =                              \
    pshmem_ctx_longlong_atomic_swap_nbi
#define shmem_ctx_longlong_atomic_swap_nbi pshmem_ctx_longlong_atomic_swap_nbi
#pragma weak shmem_ctx_uint_atomic_swap_nbi = pshmem_ctx_uint_atomic_swap_nbi
#define shmem_ctx_uint_atomic_swap_nbi pshmem_ctx_uint_atomic_swap_nbi
#pragma weak shmem_ctx_ulong_atomic_swap_nbi = pshmem_ctx_ulong_atomic_swap_nbi
#define shmem_ctx_ulong_atomic_swap_nbi pshmem_ctx_ulong_atomic_swap_nbi
#pragma weak shmem_ctx_ulonglong_atomic_swap_nbi =                             \
    pshmem_ctx_ulonglong_atomic_swap_nbi
#define shmem_ctx_ulonglong_atomic_swap_nbi pshmem_ctx_ulonglong_atomic_swap_nbi
#pragma weak shmem_ctx_int32_atomic_swap_nbi = pshmem_ctx_int32_atomic_swap_nbi
#define shmem_ctx_int32_atomic_swap_nbi pshmem_ctx_int32_atomic_swap_nbi
#pragma weak shmem_ctx_int64_atomic_swap_nbi = pshmem_ctx_int64_atomic_swap_nbi
#define shmem_ctx_int64_atomic_swap_nbi pshmem_ctx_int64_atomic_swap_nbi
#pragma weak shmem_ctx_uint32_atomic_swap_nbi =                                \
    pshmem_ctx_uint32_atomic_swap_nbi
#define shmem_ctx_uint32_atomic_swap_nbi pshmem_ctx_uint32_atomic_swap_nbi
#pragma weak shmem_ctx_uint64_atomic_swap_nbi =                                \
    pshmem_ctx_uint64_atomic_swap_nbi
#define shmem_ctx_uint64_atomic_swap_nbi pshmem_ctx_uint64_atomic_swap_nbi
#pragma weak shmem_ctx_size_atomic_swap_nbi = pshmem_ctx_size_atomic_swap_nbi
#define shmem_ctx_size_atomic_swap_nbi pshmem_ctx_size_atomic_swap_nbi
#pragma weak shmem_ctx_ptrdiff_atomic_swap_nbi =                               \
    pshmem_ctx_ptrdiff_atomic_swap_nbi
#define shmem_ctx_ptrdiff_atomic_swap_nbi pshmem_ctx_ptrdiff_atomic_swap_nbi
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to define non-blocking atomic swap operations for different
 * types
 *
 * These routines perform a non-blocking atomic swap operation. The operation
 * atomically sets the value of a remote variable and returns its previous
 * value. The operation is performed without protecting the mutex.
 *
 * @param _name The type name suffix for the function
 * @param _type The actual C type for the operation
 */
#define SHMEM_CTX_TYPE_SWAP_NBI(_name, _type)                                  \
  void shmem_ctx_##_name##_atomic_swap_nbi(                                    \
      shmem_ctx_t ctx, _type *fetch, _type *target, _type value, int pe) {     \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_SYMMETRIC(target, 3);                                         \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_swap(ctx, target, &value, sizeof(value), pe, fetch));       \
  }

/* Define context-based non-blocking atomic swap operations using the type table
 */
#define SHMEM_CTX_TYPE_SWAP_NBI_HELPER(_type, _typename)                       \
  SHMEM_CTX_TYPE_SWAP_NBI(_typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_SWAP_NBI_HELPER)
#undef SHMEM_CTX_TYPE_SWAP_NBI_HELPER

/**
 * @brief Defines the API for non-blocking atomic swap operations
 *
 * These macros create the public API functions for non-blocking atomic swap
 * operations for different types. Each function performs a swap operation
 * without a context.
 */
/* Define non-context non-blocking atomic swap operations using the type table
 */
#define API_DEF_AMO2_NBI_HELPER(_type, _typename)                              \
  API_DEF_AMO2_NBI(swap, _typename, _type)
SHMEM_EXTENDED_AMO_TYPE_TABLE(API_DEF_AMO2_NBI_HELPER)
#undef API_DEF_AMO2_NBI_HELPER
