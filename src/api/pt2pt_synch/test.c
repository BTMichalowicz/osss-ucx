/**
 * @file test.c
 * @brief Implementation of OpenSHMEM test operations
 *
 * This file provides test operations that check if a variable meets specified
 * comparison criteria. The operations are non-blocking and return immediately.
 *
 * For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <shmem/generics.h>
#include "shmem_mutex.h"
#include "module.h"
#include "shmemu.h"
#include "shmemc.h"

#ifdef ENABLE_PSHMEM
#pragma weak shmem_short_test = pshmem_short_test
#define shmem_short_test pshmem_short_test
#pragma weak shmem_int_test = pshmem_int_test
#define shmem_int_test pshmem_int_test
#pragma weak shmem_long_test = pshmem_long_test
#define shmem_long_test pshmem_long_test
#pragma weak shmem_longlong_test = pshmem_longlong_test
#define shmem_longlong_test pshmem_longlong_test
#pragma weak shmem_ushort_test = pshmem_ushort_test
#define shmem_ushort_test pshmem_ushort_test
#pragma weak shmem_uint_test = pshmem_uint_test
#define shmem_uint_test pshmem_uint_test
#pragma weak shmem_ulong_test = pshmem_ulong_test
#define shmem_ulong_test pshmem_ulong_test
#pragma weak shmem_ulonglong_test = pshmem_ulonglong_test
#define shmem_ulonglong_test pshmem_ulonglong_test
#pragma weak shmem_int32_test = pshmem_int32_test
#define shmem_int32_test pshmem_int32_test
#pragma weak shmem_int64_test = pshmem_int64_test
#define shmem_int64_test pshmem_int64_test
#pragma weak shmem_uint32_test = pshmem_uint32_test
#define shmem_uint32_test pshmem_uint32_test
#pragma weak shmem_uint64_test = pshmem_uint64_test
#define shmem_uint64_test pshmem_uint64_test
#pragma weak shmem_size_test = pshmem_size_test
#define shmem_size_test pshmem_size_test
#pragma weak shmem_ptrdiff_test = pshmem_ptrdiff_test
#define shmem_ptrdiff_test pshmem_ptrdiff_test
#endif /* ENABLE_PSHMEM */

/**
 * @brief Declares the shmem_test routine for testing variables against
 * comparison criteria
 *
 * @param _opname Base name of the operation (e.g. short, int, etc)
 * @param _type C data type for the operation
 * @param _size Size in bits (16, 32, or 64)
 *
 * Tests if a variable meets specified comparison criteria. The function returns
 * immediately without blocking.
 *
 * @return Returns 1 if the comparison evaluates to true, 0 if it evaluates to
 * false
 */
#define SHMEM_TYPE_TEST_INTERNAL(_opname, _type, _size)                                 \
  int shmem_##_opname##_test(_type *ivar, int cmp, _type cmp_value) {          \
    SHMEMT_MUTEX_NOPROTECT(switch (cmp) {                                      \
      case SHMEM_CMP_EQ:                                                       \
        return shmemc_ctx_test_eq##_size(SHMEM_CTX_DEFAULT,                    \
                                         (int##_size##_t *)ivar, cmp_value);   \
        break;                                                                 \
      case SHMEM_CMP_NE:                                                       \
        return shmemc_ctx_test_ne##_size(SHMEM_CTX_DEFAULT,                    \
                                         (int##_size##_t *)ivar, cmp_value);   \
        break;                                                                 \
      case SHMEM_CMP_GT:                                                       \
        return shmemc_ctx_test_gt##_size(SHMEM_CTX_DEFAULT,                    \
                                         (int##_size##_t *)ivar, cmp_value);   \
        break;                                                                 \
      case SHMEM_CMP_LE:                                                       \
        return shmemc_ctx_test_le##_size(SHMEM_CTX_DEFAULT,                    \
                                         (int##_size##_t *)ivar, cmp_value);   \
        break;                                                                 \
      case SHMEM_CMP_LT:                                                       \
        return shmemc_ctx_test_lt##_size(SHMEM_CTX_DEFAULT,                    \
                                         (int##_size##_t *)ivar, cmp_value);   \
        break;                                                                 \
      case SHMEM_CMP_GE:                                                       \
        return shmemc_ctx_test_ge##_size(SHMEM_CTX_DEFAULT,                    \
                                         (int##_size##_t *)ivar, cmp_value);   \
        break;                                                                 \
      default:                                                                 \
        shmemu_fatal(MODULE ": unknown operator (code %d) in \"%s\"", cmp,     \
                     __func__);                                                \
        return -1;                                                             \
        /* NOT REACHED */                                                      \
        break;                                                                 \
    });                                                                        \
  }

#define SHMEM_TYPE_TEST_HELPER(CTYPE, SHMTYPE) \
  SHMEM_APPLY(SHMEM_TYPE_TEST_INTERNAL, SHMTYPE, CTYPE, SHMEM_TYPE_BITSOF_##SHMTYPE)


/* shorts are not in the table */
SHMEM_TYPE_TEST_HELPER(short, short)
SHMEM_TYPE_TEST_HELPER(unsigned short, ushort)

C11_SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_TYPE_TEST_HELPER)
