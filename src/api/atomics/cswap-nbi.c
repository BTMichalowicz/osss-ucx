/**
 * @file cswap-nbi.c
 * @brief Implementation of SHMEM non-blocking atomic compare-and-swap
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
#pragma weak shmem_int_atomic_compare_swap_nbi =                               \
    pshmem_int_atomic_compare_swap_nbi
#define shmem_int_atomic_compare_swap_nbi pshmem_int_atomic_compare_swap_nbi
#pragma weak shmem_long_atomic_compare_swap_nbi =                              \
    pshmem_long_atomic_compare_swap_nbi
#define shmem_long_atomic_compare_swap_nbi pshmem_long_atomic_compare_swap_nbi
#pragma weak shmem_longlong_atomic_compare_swap_nbi =                          \
    pshmem_longlong_atomic_compare_swap_nbi
#define shmem_longlong_atomic_compare_swap_nbi                                 \
  pshmem_longlong_atomic_compare_swap_nbi
#pragma weak shmem_uint_atomic_compare_swap_nbi =                              \
    pshmem_uint_atomic_compare_swap_nbi
#define shmem_uint_atomic_compare_swap_nbi pshmem_uint_atomic_compare_swap_nbi
#pragma weak shmem_ulong_atomic_compare_swap_nbi =                             \
    pshmem_ulong_atomic_compare_swap_nbi
#define shmem_ulong_atomic_compare_swap_nbi pshmem_ulong_atomic_compare_swap_nbi
#pragma weak shmem_ulonglong_atomic_compare_swap_nbi =                         \
    pshmem_ulonglong_atomic_compare_swap_nbi
#define shmem_ulonglong_atomic_compare_swap_nbi                                \
  pshmem_ulonglong_atomic_compare_swap_nbi
#pragma weak shmem_int32_atomic_compare_swap_nbi =                             \
    pshmem_int32_atomic_compare_swap_nbi
#define shmem_int32_atomic_compare_swap_nbi pshmem_int32_atomic_compare_swap_nbi
#pragma weak shmem_int64_atomic_compare_swap_nbi =                             \
    pshmem_int64_atomic_compare_swap_nbi
#define shmem_int64_atomic_compare_swap_nbi pshmem_int64_atomic_compare_swap_nbi
#pragma weak shmem_uint32_atomic_compare_swap_nbi =                            \
    pshmem_uint32_atomic_compare_swap_nbi
#define shmem_uint32_atomic_compare_swap_nbi                                   \
  pshmem_uint32_atomic_compare_swap_nbi
#pragma weak shmem_uint64_atomic_compare_swap_nbi =                            \
    pshmem_uint64_atomic_compare_swap_nbi
#define shmem_uint64_atomic_compare_swap_nbi                                   \
  pshmem_uint64_atomic_compare_swap_nbi
#pragma weak shmem_size_atomic_compare_swap_nbi =                              \
    pshmem_size_atomic_compare_swap_nbi
#define shmem_size_atomic_compare_swap_nbi pshmem_size_atomic_compare_swap_nbi
#pragma weak shmem_ptrdiff_atomic_compare_swap_nbi =                           \
    pshmem_ptrdiff_atomic_compare_swap_nbi
#define shmem_ptrdiff_atomic_compare_swap_nbi                                  \
  pshmem_ptrdiff_atomic_compare_swap_nbi
#endif /* ENABLE_PSHMEM */

/**
 * @brief Macro to define non-blocking atomic compare-and-swap operations with
 * contexts
 *
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that atomically compares a value with a remote variable
 * and swaps it with a new value if they match. The operation is performed in a
 * non-blocking manner using the specified context.
 */
#define SHMEM_CTX_TYPE_CSWAP_NBI(_name, _type)                                 \
  void shmem_ctx_##_name##_atomic_compare_swap_nbi(                            \
      shmem_ctx_t ctx, _type *fetch, _type *target, _type cond, _type value,   \
      int pe) {                                                                \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_SYMMETRIC(target, 3);                                         \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_cswap(ctx, target, &cond, &value,        \
                                            sizeof(value), pe, fetch));        \
  }

/* Define context-based non-blocking atomic compare-and-swap operations using
 * the type table */
#define SHMEM_CTX_TYPE_CSWAP_NBI_HELPER(_type, _typename)                      \
  SHMEM_CTX_TYPE_CSWAP_NBI(_typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_CTX_TYPE_CSWAP_NBI_HELPER)
#undef SHMEM_CTX_TYPE_CSWAP_NBI_HELPER

/**
 * @brief Define non-blocking atomic compare-and-swap operations without
 * contexts
 *
 * Implements non-blocking atomic compare-and-swap operations for various
 * integer types using the default context. These operations atomically compare
 * a value with a remote variable and swap it with a new value if they match.
 */
#define API_DEF_AMO3_NBI_HELPER(_type, _typename)                              \
  API_DEF_AMO3_NBI(compare_swap, _typename, _type)
SHMEM_STANDARD_AMO_TYPE_TABLE(API_DEF_AMO3_NBI_HELPER)
#undef API_DEF_AMO3_NBI_HELPER
