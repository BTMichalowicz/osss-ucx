/**
 * @file waituntil.c
 * @brief Implementation of OpenSHMEM wait operations
 *
 * This file provides wait operations that block until a variable meets
 * specified comparison criteria.
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
#pragma weak shmem_short_wait_until = pshmem_short_wait_until
#define shmem_short_wait_until pshmem_short_wait_until
#pragma weak shmem_int_wait_until = pshmem_int_wait_until
#define shmem_int_wait_until pshmem_int_wait_until
#pragma weak shmem_long_wait_until = pshmem_long_wait_until
#define shmem_long_wait_until pshmem_long_wait_until
#pragma weak shmem_longlong_wait_until = pshmem_longlong_wait_until
#define shmem_longlong_wait_until pshmem_longlong_wait_until
#pragma weak shmem_ushort_wait_until = pshmem_ushort_wait_until
#define shmem_ushort_wait_until pshmem_ushort_wait_until
#pragma weak shmem_uint_wait_until = pshmem_uint_wait_until
#define shmem_uint_wait_until pshmem_uint_wait_until
#pragma weak shmem_ulong_wait_until = pshmem_ulong_wait_until
#define shmem_ulong_wait_until pshmem_ulong_wait_until
#pragma weak shmem_ulonglong_wait_until = pshmem_ulonglong_wait_until
#define shmem_ulonglong_wait_until pshmem_ulonglong_wait_until
#pragma weak shmem_int32_wait_until = pshmem_int32_wait_until
#define shmem_int32_wait_until pshmem_int32_wait_until
#pragma weak shmem_int64_wait_until = pshmem_int64_wait_until
#define shmem_int64_wait_until pshmem_int64_wait_until
#pragma weak shmem_uint32_wait_until = pshmem_uint32_wait_until
#define shmem_uint32_wait_until pshmem_uint32_wait_until
#pragma weak shmem_uint64_wait_until = pshmem_uint64_wait_until
#define shmem_uint64_wait_until pshmem_uint64_wait_until
#pragma weak shmem_size_wait_until = pshmem_size_wait_until
#define shmem_size_wait_until pshmem_size_wait_until
#pragma weak shmem_ptrdiff_wait_until = pshmem_ptrdiff_wait_until
#define shmem_ptrdiff_wait_until pshmem_ptrdiff_wait_until
#endif /* ENABLE_PSHMEM */

/**
 * @brief Waits until a variable meets specified comparison criteria
 *
 * @param _opname Base name of the operation (e.g. short, int, etc)
 * @param _type C data type for the operation
 * @param _size Size in bits (16, 32, or 64)
 *
 * Blocks until a variable meets specified comparison criteria.
 *
 * @param ivar Variable to be tested
 * @param cmp Comparison operator (SHMEM_CMP_EQ, NE, GT, LE, LT, GE)
 * @param cmp_value Value to compare against
 *
 * wait_until with operator dispatchers, type-parameterized.
 */
#define SHMEM_TYPE_WAIT_UNTIL(_opname, _type, _size)                           \
  void shmem_##_opname##_wait_until(_type *ivar, int cmp, _type cmp_value) {   \
    SHMEMT_MUTEX_NOPROTECT(switch (cmp) {                                      \
      case SHMEM_CMP_EQ:                                                       \
        shmemc_ctx_wait_until_eq##_size(SHMEM_CTX_DEFAULT,                     \
                                        (int##_size##_t *)ivar, cmp_value);    \
        break;                                                                 \
      case SHMEM_CMP_NE:                                                       \
        shmemc_ctx_wait_until_ne##_size(SHMEM_CTX_DEFAULT,                     \
                                        (int##_size##_t *)ivar, cmp_value);    \
        break;                                                                 \
      case SHMEM_CMP_GT:                                                       \
        shmemc_ctx_wait_until_gt##_size(SHMEM_CTX_DEFAULT,                     \
                                        (int##_size##_t *)ivar, cmp_value);    \
        break;                                                                 \
      case SHMEM_CMP_LE:                                                       \
        shmemc_ctx_wait_until_le##_size(SHMEM_CTX_DEFAULT,                     \
                                        (int##_size##_t *)ivar, cmp_value);    \
        break;                                                                 \
      case SHMEM_CMP_LT:                                                       \
        shmemc_ctx_wait_until_lt##_size(SHMEM_CTX_DEFAULT,                     \
                                        (int##_size##_t *)ivar, cmp_value);    \
        break;                                                                 \
      case SHMEM_CMP_GE:                                                       \
        shmemc_ctx_wait_until_ge##_size(SHMEM_CTX_DEFAULT,                     \
                                        (int##_size##_t *)ivar, cmp_value);    \
        break;                                                                 \
      default:                                                                 \
        shmemu_fatal("unknown operator (code %d) in \"%s\"", cmp, __func__);   \
        return;                                                                \
        /* NOT REACHED */                                                      \
        break;                                                                 \
    });                                                                        \
  }

#define SHMEM_TYPE_WAIT_UNTIL_HELPER(CTYPE, SHMTYPE) \
  SHMEM_APPLY(SHMEM_TYPE_WAIT_UNTIL, SHMTYPE, CTYPE, SHMEM_TYPE_BITSOF_##SHMTYPE)

/* shorts are not in the table */
SHMEM_TYPE_WAIT_UNTIL_HELPER(short, short)
SHMEM_TYPE_WAIT_UNTIL_HELPER(unsigned short, ushort)

C11_SHMEM_STANDARD_AMO_TYPE_TABLE(SHMEM_TYPE_WAIT_UNTIL_HELPER)

#ifdef ENABLE_PSHMEM
#pragma weak shmem_signal_wait_until = pshmem_signal_wait_until
#define shmem_signal_wait_until pshmem_signal_wait_until
#endif /* ENABLE_PSHMEM */

/**
 * @brief Waits until a signal variable meets specified comparison criteria
 *
 * @param sig_addr Signal variable to be tested
 * @param cmp Comparison operator (SHMEM_CMP_EQ, NE, GT, LE, LT, GE)
 * @param cmp_value Value to compare against
 *
 * @return Value of sig_addr after comparison is satisfied
 */
uint64_t shmem_signal_wait_until(uint64_t *sig_addr, int cmp,
                                 uint64_t cmp_value) {
  shmem_uint64_wait_until(sig_addr, cmp, cmp_value);

  return *sig_addr;
}
