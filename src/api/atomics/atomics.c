/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmem_mutex.h"
#include "shmemu.h"
#include "shmemc.h"

#include <bits/wordsize.h>
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

/* ------------------------------------------------------------------------ */

/**
 * @brief Context-free atomic operations
 *
 * These macros define atomic operations that use the default context
 * (SHMEM_CTX_DEFAULT). They provide wrappers around the context-based
 * atomic operations.
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
