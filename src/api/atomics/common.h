/**
 * @file common.h
 * @brief Common macros and definitions for SHMEM atomic operations
 *
 * For license: see LICENSE file at top-level
 */

#ifndef SHMEM_AMO_COMMON_H
#define SHMEM_AMO_COMMON_H 1

#include <bits/wordsize.h>
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup blocking_atomics Blocking Atomic Operations
 * @{
 */

/**
 * @brief Macro to define atomic operations with const target
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that performs an atomic operation on a const remote target
 * using the default context
 */
#define API_DEF_CONST_AMO1(_op, _name, _type)                                  \
  _type shmem_##_name##_atomic_##_op(const _type *target, int pe) {            \
    return shmem_ctx_##_name##_atomic_##_op(SHMEM_CTX_DEFAULT, target, pe);    \
  }

/**
 * @brief Macro to define atomic operations
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that performs an atomic operation on a remote target
 * using the default context
 */
#define API_DEF_AMO1(_op, _name, _type)                                        \
  _type shmem_##_name##_atomic_##_op(_type *target, int pe) {                  \
    return shmem_ctx_##_name##_atomic_##_op(SHMEM_CTX_DEFAULT, target, pe);    \
  }

/**
 * @brief Macro to define atomic operations with value parameter
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that performs an atomic operation with a value parameter
 * using the default context
 */
#define API_DEF_AMO2(_op, _name, _type)                                        \
  _type shmem_##_name##_atomic_##_op(_type *target, _type value, int pe) {     \
    return shmem_ctx_##_name##_atomic_##_op(SHMEM_CTX_DEFAULT, target, value,  \
                                            pe);                               \
  }

/**
 * @brief Macro to define atomic operations with conditional and value
 * parameters
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that performs a conditional atomic operation
 * using the default context
 */
#define API_DEF_AMO3(_op, _name, _type)                                        \
  _type shmem_##_name##_atomic_##_op(_type *target, _type cond, _type value,   \
                                     int pe) {                                 \
    return shmem_ctx_##_name##_atomic_##_op(SHMEM_CTX_DEFAULT, target, cond,   \
                                            value, pe);                        \
  }

/**
 * @brief Macro to define void atomic operations
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a void function that performs an atomic operation
 * using the default context
 */
#define API_DEF_VOID_AMO1(_op, _name, _type)                                   \
  void shmem_##_name##_atomic_##_op(_type *target, int pe) {                   \
    shmem_ctx_##_name##_atomic_##_op(SHMEM_CTX_DEFAULT, target, pe);           \
  }

/**
 * @brief Macro to define void atomic operations with value parameter
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a void function that performs an atomic operation with a value
 * parameter using the default context
 */
#define API_DEF_VOID_AMO2(_op, _name, _type)                                   \
  void shmem_##_name##_atomic_##_op(_type *target, _type value, int pe) {      \
    shmem_ctx_##_name##_atomic_##_op(SHMEM_CTX_DEFAULT, target, value, pe);    \
  }

/**
 * @brief Macro to define bitwise atomic operations
 *
 * @param _opname Operation name (and, or, xor)
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a void function that performs a bitwise atomic operation
 * using the specified context
 */
#define SHMEM_CTX_TYPE_BITWISE(_opname, _name, _type)                          \
  void shmem_ctx_##_name##_atomic_##_opname(shmem_ctx_t ctx, _type *target,    \
                                            _type value, int pe) {             \
    SHMEMT_MUTEX_NOPROTECT(                                                    \
        shmemc_ctx_##_opname(ctx, target, &value, sizeof(value), pe));         \
  }

/**
 * @brief Macro to define fetch bitwise atomic operations
 *
 * @param _opname Operation name (and, or, xor)
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a function that performs a fetch bitwise atomic operation
 * using the specified context
 */
#define SHMEM_CTX_TYPE_FETCH_BITWISE(_opname, _name, _type)                    \
  _type shmem_ctx_##_name##_atomic_fetch_##_opname(                            \
      shmem_ctx_t ctx, _type *target, _type value, int pe) {                   \
    _type v;                                                                   \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_fetch_##_opname(ctx, target, &value,     \
                                                      sizeof(value), pe, &v)); \
    return v;                                                                  \
  }

/** @} */

/**
 * @defgroup nonblocking_atomics Non-blocking Atomic Operations
 * @{
 */

/**
 * @brief Macro to define non-blocking atomic operations with const target
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a non-blocking atomic operation on a const remote target
 * using the default context
 */
#define API_DEF_CONST_AMO1_NBI(_op, _name, _type)                              \
  void shmem_##_name##_atomic_##_op##_nbi(_type *fetch, const _type *target,   \
                                          int pe) {                            \
    shmem_ctx_##_name##_atomic_##_op##_nbi(SHMEM_CTX_DEFAULT, fetch, target,   \
                                           pe);                                \
  }

/**
 * @brief Macro to define non-blocking atomic operations
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a non-blocking atomic operation using the default context
 */
#define API_DEF_AMO1_NBI(_op, _name, _type)                                    \
  void shmem_##_name##_atomic_##_op##_nbi(_type *fetch, _type *target,         \
                                          int pe) {                            \
    shmem_ctx_##_name##_atomic_##_op##_nbi(SHMEM_CTX_DEFAULT, fetch, target,   \
                                           pe);                                \
  }

/**
 * @brief Macro to define non-blocking atomic operations with value parameter
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a non-blocking atomic operation with a value parameter
 * using the default context
 */
#define API_DEF_AMO2_NBI(_op, _name, _type)                                    \
  void shmem_##_name##_atomic_##_op##_nbi(_type *fetch, _type *target,         \
                                          _type value, int pe) {               \
    shmem_ctx_##_name##_atomic_##_op##_nbi(SHMEM_CTX_DEFAULT, fetch, target,   \
                                           value, pe);                         \
  }

/**
 * @brief Macro to define non-blocking atomic operations with conditional and
 * value parameters
 *
 * @param _op Operation name
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a non-blocking conditional atomic operation using the default context
 */
#define API_DEF_AMO3_NBI(_op, _name, _type)                                    \
  void shmem_##_name##_atomic_##_op##_nbi(_type *fetch, _type *target,         \
                                          _type cond, _type value, int pe) {   \
    shmem_ctx_##_name##_atomic_##_op##_nbi(SHMEM_CTX_DEFAULT, fetch, target,   \
                                           cond, value, pe);                   \
  }

/**
 * @brief Macro to define non-blocking fetch bitwise atomic operations
 *
 * @param _opname Operation name (and, or, xor)
 * @param _name Type name string
 * @param _type Data type
 *
 * Defines a non-blocking fetch bitwise atomic operation using the specified
 * context
 */
#define SHMEM_CTX_TYPE_FETCH_BITWISE_NBI(_opname, _name, _type)                \
  void shmem_ctx_##_name##_atomic_fetch_##_opname##_nbi(                       \
      shmem_ctx_t ctx, _type *fetch, _type *target, _type value, int pe) {     \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_fetch_##_opname(                         \
        ctx, target, &value, sizeof(value), pe, fetch));                       \
  }

/** @} */

#endif /* ! SHMEM_AMO_COMMON_H */
