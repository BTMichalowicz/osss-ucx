/* For license: see LICENSE file at top-level */

/**
 * @file put_signal.h
 * @brief Header file defining OpenSHMEM signal-based put operations
 *
 * This file contains macro definitions for implementing both blocking and
 * non-blocking signal-based put operations in OpenSHMEM. These macros define
 * type-specific and size-specific variants of the put_signal operations.
 */

#ifndef _SHMEM_PUTGET_SIGNAL_H
#define _SHMEM_PUTGET_SIGNAL_H 1

#include "shmemu.h"
#include "shmemc.h"
#include "shmem_mutex.h"

/**
 * @brief Macro to define a typed blocking put_signal operation with context
 *
 * @param _name Type name suffix for the operation
 * @param _type Actual C type for the operation
 *
 * Defines a function that performs a blocking put operation with signaling for
 * a specific data type, using a communication context.
 */
#define SHMEM_CTX_TYPED_PUT_SIGNAL(_name, _type)                               \
  void shmem_ctx_##_name##_put_signal(                                         \
      shmem_ctx_t ctx, _type *dest, const _type *src, size_t nelems,           \
      uint64_t *sig_addr, uint64_t signal, int sig_op, int pe) {               \
    const size_t nb = sizeof(_type) * nelems;                                  \
                                                                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 8);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 2);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 5);                                       \
                                                                               \
    SHMEMT_MUTEX_PROTECT(shmemc_ctx_put_signal(ctx, dest, src, nb, sig_addr,   \
                                               signal, sig_op, pe));           \
  }

/**
 * @brief Macro to declare a typed blocking put_signal operation
 *
 * @param _name Type name suffix for the operation
 * @param _type Actual C type for the operation
 *
 * Declares a function that performs a blocking put operation with signaling for
 * a specific data type, using the default context.
 */
#define API_DECL_TYPED_PUT_SIGNAL(_name, _type)                                \
  void shmem_##_name##_put_signal(_type *dest, const _type *src,               \
                                  size_t nelems, uint64_t *sig_addr,           \
                                  uint64_t signal, int sig_op, int pe) {       \
    const size_t nb = sizeof(_type) * nelems;                                  \
                                                                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 7);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 1);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 4);                                       \
                                                                               \
    SHMEMT_MUTEX_PROTECT(shmemc_ctx_put_signal(                                \
        SHMEM_CTX_DEFAULT, dest, src, nb, sig_addr, signal, sig_op, pe));      \
  }

/**
 * @brief Macro to define a sized blocking put_signal operation with context
 *
 * @param _size Bit size for the operation
 *
 * Defines a function that performs a blocking put operation with signaling for
 * a specific bit size, using a communication context.
 */
#define SHMEM_CTX_DECL_SIZED_PUT_SIGNAL(_size)                                 \
  void shmem_ctx_put##_size##_signal(                                          \
      shmem_ctx_t ctx, void *dest, const void *src, size_t nelems,             \
      uint64_t *sig_addr, uint64_t signal, int sig_op, int pe) {               \
    const size_t nb = BITS2BYTES(_size) * nelems;                              \
                                                                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 8);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 2);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 5);                                       \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_put_signal(ctx, dest, src, nb, sig_addr, \
                                                 signal, sig_op, pe));         \
  }

/**
 * @brief Macro to declare a sized blocking put_signal operation
 *
 * @param _size Bit size for the operation
 *
 * Declares a function that performs a blocking put operation with signaling for
 * a specific bit size, using the default context.
 */
#define API_DECL_SIZED_PUT_SIGNAL(_size)                                       \
  void shmem_put##_size##_signal(void *dest, const void *src, size_t nelems,   \
                                 uint64_t *sig_addr, uint64_t signal,          \
                                 int sig_op, int pe) {                         \
    const size_t nb = BITS2BYTES(_size) * nelems;                              \
                                                                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 7);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 1);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 4);                                       \
                                                                               \
    SHMEMT_MUTEX_PROTECT(shmemc_ctx_put_signal(                                \
        SHMEM_CTX_DEFAULT, dest, src, nb, sig_addr, signal, sig_op, pe));      \
  }

/**
 * @brief Macro to define a memory blocking put_signal operation with context
 *
 * Defines a function that performs a blocking put operation with signaling for
 * arbitrary memory regions, using a communication context.
 */
#define SHMEM_CTX_DECL_PUTMEM_SIGNAL()                                         \
  void shmem_ctx_putmem_signal(shmem_ctx_t ctx, void *dest, const void *src,   \
                               size_t nelems, uint64_t *sig_addr,              \
                               uint64_t signal, int sig_op, int pe) {          \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 8);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 2);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 5);                                       \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_put_signal(                              \
        ctx, dest, src, nelems, sig_addr, signal, sig_op, pe));                \
  }

/**
 * @brief Macro to declare a memory blocking put_signal operation
 *
 * Declares a function that performs a blocking put operation with signaling for
 * arbitrary memory regions, using the default context.
 */
#define API_DECL_PUTMEM_SIGNAL()                                               \
  void shmem_putmem_signal(void *dest, const void *src, size_t nelems,         \
                           uint64_t *sig_addr, uint64_t signal, int sig_op,    \
                           int pe) {                                           \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 7);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 1);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 4);                                       \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_put_signal(                              \
        SHMEM_CTX_DEFAULT, dest, src, nelems, sig_addr, signal, sig_op, pe));  \
  }

/*
 * non-blocking variants
 */

/**
 * @brief Macro to define a typed non-blocking put_signal operation with context
 *
 * @param _name Type name suffix for the operation
 * @param _type Actual C type for the operation
 *
 * Defines a function that performs a non-blocking put operation with signaling
 * for a specific data type, using a communication context.
 */
#define SHMEM_CTX_TYPED_PUT_SIGNAL_NBI(_name, _type)                           \
  void shmem_ctx_##_name##_put_signal_nbi(                                     \
      shmem_ctx_t ctx, _type *dest, const _type *src, size_t nelems,           \
      uint64_t *sig_addr, uint64_t signal, int sig_op, int pe) {               \
    const size_t nb = sizeof(_type) * nelems;                                  \
                                                                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 8);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 2);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 5);                                       \
                                                                               \
    SHMEMT_MUTEX_PROTECT(shmemc_ctx_put_signal_nbi(                            \
        ctx, dest, src, nb, sig_addr, signal, sig_op, pe));                    \
  }

/**
 * @brief Macro to declare a typed non-blocking put_signal operation
 *
 * @param _name Type name suffix for the operation
 * @param _type Actual C type for the operation
 *
 * Declares a function that performs a non-blocking put operation with signaling
 * for a specific data type, using the default context.
 */
#define API_DECL_TYPED_PUT_SIGNAL_NBI(_name, _type)                            \
  void shmem_##_name##_put_signal_nbi(_type *dest, const _type *src,           \
                                      size_t nelems, uint64_t *sig_addr,       \
                                      uint64_t signal, int sig_op, int pe) {   \
    const size_t nb = sizeof(_type) * nelems;                                  \
                                                                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 7);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 1);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 4);                                       \
                                                                               \
    SHMEMT_MUTEX_PROTECT(shmemc_ctx_put_signal_nbi(                            \
        SHMEM_CTX_DEFAULT, dest, src, nb, sig_addr, signal, sig_op, pe));      \
  }

/**
 * @brief Macro to define a sized non-blocking put_signal operation with context
 *
 * @param _size Bit size for the operation
 *
 * Defines a function that performs a non-blocking put operation with signaling
 * for a specific bit size, using a communication context.
 */
#define SHMEM_CTX_DECL_SIZED_PUT_SIGNAL_NBI(_size)                             \
  void shmem_ctx_put##_size##_signal_nbi(                                      \
      shmem_ctx_t ctx, void *dest, const void *src, size_t nelems,             \
      uint64_t *sig_addr, uint64_t signal, int sig_op, int pe) {               \
    const size_t nb = BITS2BYTES(_size) * nelems;                              \
                                                                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 8);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 2);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 5);                                       \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_put_signal_nbi(                          \
        ctx, dest, src, nb, sig_addr, signal, sig_op, pe));                    \
  }

/**
 * @brief Macro to declare a sized non-blocking put_signal operation
 *
 * @param _size Bit size for the operation
 *
 * Declares a function that performs a non-blocking put operation with signaling
 * for a specific bit size, using the default context.
 */
#define API_DECL_SIZED_PUT_SIGNAL_NBI(_size)                                   \
  void shmem_put##_size##_signal_nbi(void *dest, const void *src,              \
                                     size_t nelems, uint64_t *sig_addr,        \
                                     uint64_t signal, int sig_op, int pe) {    \
    const size_t nb = BITS2BYTES(_size) * nelems;                              \
                                                                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 7);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 1);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 4);                                       \
                                                                               \
    SHMEMT_MUTEX_PROTECT(shmemc_ctx_put_signal_nbi(                            \
        SHMEM_CTX_DEFAULT, dest, src, nb, sig_addr, signal, sig_op, pe));      \
  }

/**
 * @brief Macro to define a memory non-blocking put_signal operation with
 * context
 *
 * Defines a function that performs a non-blocking put operation with signaling
 * for arbitrary memory regions, using a communication context.
 */
#define SHMEM_CTX_DECL_PUTMEM_SIGNAL_NBI()                                     \
  void shmem_ctx_putmem_signal_nbi(                                            \
      shmem_ctx_t ctx, void *dest, const void *src, size_t nelems,             \
      uint64_t *sig_addr, uint64_t signal, int sig_op, int pe) {               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 8);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 2);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 5);                                       \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_put_signal_nbi(                          \
        ctx, dest, src, nelems, sig_addr, signal, sig_op, pe));                \
  }

/**
 * @brief Macro to declare a memory non-blocking put_signal operation
 *
 * Declares a function that performs a non-blocking put operation with signaling
 * for arbitrary memory regions, using the default context.
 */
#define API_DECL_PUTMEM_SIGNAL_NBI()                                           \
  void shmem_putmem_signal_nbi(void *dest, const void *src, size_t nelems,     \
                               uint64_t *sig_addr, uint64_t signal,            \
                               int sig_op, int pe) {                           \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_PE_ARG_RANGE(pe, 7);                                          \
    SHMEMU_CHECK_SYMMETRIC(dest, 1);                                           \
    SHMEMU_CHECK_SYMMETRIC(sig_addr, 4);                                       \
                                                                               \
    SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_put_signal_nbi(                          \
        SHMEM_CTX_DEFAULT, dest, src, nelems, sig_addr, signal, sig_op, pe));  \
  }

#endif /* ! _SHMEM_PUTGET_SIGNAL_H */
