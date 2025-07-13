/**
 * @file testany_vector.c
 * @brief Implementation of OpenSHMEM test operations for vectors
 *
 * This file provides test operations for vectors that check if any elements
 * meet specified comparison criteria. The operations are non-blocking and
 * return immediately.
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
#pragma weak shmem_short_test_any_vector = pshmem_short_test_any_vector
#define shmem_short_test_any_vector pshmem_short_test_any_vector
#pragma weak shmem_int_test_any_vector = pshmem_int_test_any_vector
#define shmem_int_test_any_vector pshmem_int_test_any_vector
#pragma weak shmem_long_test_any_vector = pshmem_long_test_any_vector
#define shmem_long_test_any_vector pshmem_long_test_any_vector
#pragma weak shmem_longlong_test_any_vector = pshmem_longlong_test_any_vector
#define shmem_longlong_test_any_vector pshmem_longlong_test_any_vector
#pragma weak shmem_ushort_test_any_vector = pshmem_ushort_test_any_vector
#define shmem_ushort_test_any_vector pshmem_ushort_test_any_vector
#pragma weak shmem_uint_test_any_vector = pshmem_uint_test_any_vector
#define shmem_uint_test_any_vector pshmem_uint_test_any_vector
#pragma weak shmem_ulong_test_any_vector = pshmem_ulong_test_any_vector
#define shmem_ulong_test_any_vector pshmem_ulong_test_any_vector
#pragma weak shmem_ulonglong_test_any_vector = pshmem_ulonglong_test_any_vector
#define shmem_ulonglong_test_any_vector pshmem_ulonglong_test_any_vector
#pragma weak shmem_int32_test_any_vector = pshmem_int32_test_any_vector
#define shmem_int32_test_any_vector pshmem_int32_test_any_vector
#pragma weak shmem_int64_test_any_vector = pshmem_int64_test_any_vector
#define shmem_int64_test_any_vector pshmem_int64_test_any_vector
#pragma weak shmem_uint32_test_any_vector = pshmem_uint32_test_any_vector
#define shmem_uint32_test_any_vector pshmem_uint32_test_any_vector
#pragma weak shmem_uint64_test_any_vector = pshmem_uint64_test_any_vector
#define shmem_uint64_test_any_vector pshmem_uint64_test_any_vector
#pragma weak shmem_size_test_any_vector = pshmem_size_test_any_vector
#define shmem_size_test_any_vector pshmem_size_test_any_vector
#pragma weak shmem_ptrdiff_test_any_vector = pshmem_ptrdiff_test_any_vector
#define shmem_ptrdiff_test_any_vector pshmem_ptrdiff_test_any_vector
#endif /* ENABLE_PSHMEM */

/**
 * @brief Tests if any elements in a vector meet specified comparison criteria
 *
 * @param _opname Base name of the operation (e.g. short, int, etc)
 * @param _type C data type for the operation
 * @param _size Size in bits (16, 32, or 64)
 *
 * Tests if any elements in a vector meet specified comparison criteria. The
 * function returns immediately without blocking.
 *
 * @param ivars Array of variables to be tested
 * @param nelems Number of elements in the array
 * @param status Array indicating which elements to test (1=test, 0=skip)
 * @param cmp Comparison operator (SHMEM_CMP_EQ, NE, GT, LE, LT, GE)
 * @param cmp_values Array of values to compare against
 *
 * @return Returns index of first element that evaluates to true, or -1 if none
 * evaluate to true
 */
#define SHMEM_TYPE_TEST_ANY_VECTOR(_opname, _type, _size)                      \
  size_t shmem_##_opname##_test_any_vector(_type *ivars, size_t nelems,        \
                                           const int *status, int cmp,         \
                                           _type *cmp_values) {                \
    SHMEMT_MUTEX_PROTECT(switch (cmp) {                                        \
      case SHMEM_CMP_EQ:                                                       \
        return shmemc_ctx_test_any_vector_eq##_size(                           \
            SHMEM_CTX_DEFAULT, (int##_size##_t *)ivars, nelems, status,        \
            cmp_values);                                                       \
        break;                                                                 \
      case SHMEM_CMP_NE:                                                       \
        return shmemc_ctx_test_any_vector_ne##_size(                           \
            SHMEM_CTX_DEFAULT, (int##_size##_t *)ivars, nelems, status,        \
            cmp_values);                                                       \
        break;                                                                 \
      case SHMEM_CMP_GT:                                                       \
        return shmemc_ctx_test_any_vector_gt##_size(                           \
            SHMEM_CTX_DEFAULT, (int##_size##_t *)ivars, nelems, status,        \
            cmp_values);                                                       \
        break;                                                                 \
      case SHMEM_CMP_LE:                                                       \
        return shmemc_ctx_test_any_vector_le##_size(                           \
            SHMEM_CTX_DEFAULT, (int##_size##_t *)ivars, nelems, status,        \
            cmp_values);                                                       \
        break;                                                                 \
      case SHMEM_CMP_LT:                                                       \
        return shmemc_ctx_test_any_vector_lt##_size(                           \
            SHMEM_CTX_DEFAULT, (int##_size##_t *)ivars, nelems, status,        \
            cmp_values);                                                       \
        break;                                                                 \
      case SHMEM_CMP_GE:                                                       \
        return shmemc_ctx_test_any_vector_ge##_size(                           \
            SHMEM_CTX_DEFAULT, (int##_size##_t *)ivars, nelems, status,        \
            cmp_values);                                                       \
        break;                                                                 \
      default:                                                                 \
        shmemu_fatal(MODULE ": unknown operator (code %d) in \"%s\"", cmp,     \
                     __func__);                                                \
        return -1;                                                             \
        /* NOT REACHED */                                                      \
        break;                                                                 \
    });                                                                        \
  }

#define SHMEM_TYPE_TEST_ANY_VECTOR_HELPER(CTYPE, SHMTYPE)                      \
  SHMEM_APPLY(SHMEM_TYPE_TEST_ANY_VECTOR, SHMTYPE, CTYPE,                      \
              SHMEM_TYPE_BITSOF_##SHMTYPE)

C11_SHMEM_PT2PT_SYNC_TYPE_TABLE(SHMEM_TYPE_TEST_ANY_VECTOR_HELPER)
