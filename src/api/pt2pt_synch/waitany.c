/**
 * @file waitany.c
 * @brief Implementation of OpenSHMEM wait operations
 *
 * This file provides wait operations that block until any element in an array
 * meets specified comparison criteria.
 *
 * For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <shmem/generics.h>
#include "shmem_mutex.h"
#include "shmemu.h"
#include "shmemc.h"

#ifdef ENABLE_PSHMEM
#pragma weak shmem_short_wait_until_any = pshmem_short_wait_until_any
#define shmem_short_wait_until_any pshmem_short_wait_until_any
#pragma weak shmem_int_wait_until_any = pshmem_int_wait_until_any
#define shmem_int_wait_until_any pshmem_int_wait_until_any
#pragma weak shmem_long_wait_until_any = pshmem_long_wait_until_any
#define shmem_long_wait_until_any pshmem_long_wait_until_any
#pragma weak shmem_longlong_wait_until_any = pshmem_longlong_wait_until_any
#define shmem_longlong_wait_until_any pshmem_longlong_wait_until_any
#pragma weak shmem_ushort_wait_until_any = pshmem_ushort_wait_until_any
#define shmem_ushort_wait_until_any pshmem_ushort_wait_until_any
#pragma weak shmem_uint_wait_until_any = pshmem_uint_wait_until_any
#define shmem_uint_wait_until_any pshmem_uint_wait_until_any
#pragma weak shmem_ulong_wait_until_any = pshmem_ulong_wait_until_any
#define shmem_ulong_wait_until_any pshmem_ulong_wait_until_any
#pragma weak shmem_ulonglong_wait_until_any = pshmem_ulonglong_wait_until_any
#define shmem_ulonglong_wait_until_any pshmem_ulonglong_wait_until_any
#pragma weak shmem_int32_wait_until_any = pshmem_int32_wait_until_any
#define shmem_int32_wait_until_any pshmem_int32_wait_until_any
#pragma weak shmem_int64_wait_until_any = pshmem_int64_wait_until_any
#define shmem_int64_wait_until_any pshmem_int64_wait_until_any
#pragma weak shmem_uint32_wait_until_any = pshmem_uint32_wait_until_any
#define shmem_uint32_wait_until_any pshmem_uint32_wait_until_any
#pragma weak shmem_uint64_wait_until_any = pshmem_uint64_wait_until_any
#define shmem_uint64_wait_until_any pshmem_uint64_wait_until_any
#pragma weak shmem_size_wait_until_any = pshmem_size_wait_until_any
#define shmem_size_wait_until_any pshmem_size_wait_until_any
#pragma weak shmem_ptrdiff_wait_until_any = pshmem_ptrdiff_wait_until_any
#define shmem_ptrdiff_wait_until_any pshmem_ptrdiff_wait_until_any
#endif /* ENABLE_PSHMEM */

/**
 * @brief Waits until any element in an array meets specified comparison
 * criteria
 *
 * @param _opname Base name of the operation (e.g. short, int, etc)
 * @param _type C data type for the operation
 * @param _size Size in bits (16, 32, or 64)
 *
 * Blocks until any element in an array meets specified comparison criteria.
 * Returns the index of the first element that satisfied the criteria.
 *
 * @param ivars Array of variables to be tested
 * @param nelems Number of elements in the array
 * @param status Array indicating which elements to test (1=test, 0=skip)
 * @param cmp Comparison operator (SHMEM_CMP_EQ, NE, GT, LE, LT, GE)
 * @param cmp_value Value to compare against
 *
 * @return Index of the first element that satisfied the criteria, or -1 on
 * error
 */
#define SHMEM_TYPE_WAIT_UNTIL_ANY(_opname, _type, _size)                       \
  size_t shmem_##_opname##_wait_until_any(_type *ivars, size_t nelems,         \
                                          const int *status, int cmp,          \
                                          _type cmp_value) {                   \
    SHMEMT_MUTEX_PROTECT(switch (cmp) {                                        \
      case SHMEM_CMP_EQ:                                                       \
        return shmemc_ctx_wait_until_any_eq##_size(SHMEM_CTX_DEFAULT,          \
                                                   (int##_size##_t *)ivars,    \
                                                   nelems, status, cmp_value); \
        break;                                                                 \
      case SHMEM_CMP_NE:                                                       \
        return shmemc_ctx_wait_until_any_ne##_size(SHMEM_CTX_DEFAULT,          \
                                                   (int##_size##_t *)ivars,    \
                                                   nelems, status, cmp_value); \
        break;                                                                 \
      case SHMEM_CMP_GT:                                                       \
        return shmemc_ctx_wait_until_any_gt##_size(SHMEM_CTX_DEFAULT,          \
                                                   (int##_size##_t *)ivars,    \
                                                   nelems, status, cmp_value); \
        break;                                                                 \
      case SHMEM_CMP_LE:                                                       \
        return shmemc_ctx_wait_until_any_le##_size(SHMEM_CTX_DEFAULT,          \
                                                   (int##_size##_t *)ivars,    \
                                                   nelems, status, cmp_value); \
        break;                                                                 \
      case SHMEM_CMP_LT:                                                       \
        return shmemc_ctx_wait_until_any_lt##_size(SHMEM_CTX_DEFAULT,          \
                                                   (int##_size##_t *)ivars,    \
                                                   nelems, status, cmp_value); \
        break;                                                                 \
      case SHMEM_CMP_GE:                                                       \
        return shmemc_ctx_wait_until_any_ge##_size(SHMEM_CTX_DEFAULT,          \
                                                   (int##_size##_t *)ivars,    \
                                                   nelems, status, cmp_value); \
        break;                                                                 \
      default:                                                                 \
        shmemu_fatal("unknown operator (code %d) in \"%s\"", cmp, __func__);   \
        return -1;                                                             \
        /* NOT REACHED */                                                      \
        break;                                                                 \
    });                                                                        \
  }


#define SHMEM_TYPE_WAIT_UNTIL_ANY_HELPER(CTYPE, SHMTYPE) \
  SHMEM_APPLY(SHMEM_TYPE_WAIT_UNTIL_ANY, SHMTYPE, CTYPE, SHMEM_TYPE_BITSOF_##SHMTYPE)

/* shorts are not in the table */
SHMEM_TYPE_WAIT_UNTIL_ANY_HELPER(short, short)
SHMEM_TYPE_WAIT_UNTIL_ANY_HELPER(unsigned short, ushort)

C11_SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_TYPE_WAIT_UNTIL_ANY_HELPER)
