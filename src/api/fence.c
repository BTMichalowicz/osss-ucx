/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"
#include "shmem_mutex.h"

/**
 * @file fence.c
 * @brief Implementation of OpenSHMEM fence synchronization routines
 *
 * This file contains implementations of fence operations that ensure ordering
 * of put operations. The fence guarantees that all previously issued put
 * operations are complete before any subsequent put operations can start.
 */

#ifdef ENABLE_PSHMEM
#pragma weak shmem_ctx_fence = pshmem_ctx_fence
#define shmem_ctx_fence pshmem_ctx_fence
#endif /* ENABLE_PSHMEM */

/**
 * @brief Fence operation for a specific context
 *
 * @param ctx Context on which to perform fence operation
 *
 * Ensures ordering of put operations on the given context. All puts issued
 * on the context before the fence will be completed before any puts after
 * the fence can start.
 */
void shmem_ctx_fence(shmem_ctx_t ctx) {
  logger(LOG_FENCE, "%s(ctx=%lu)", __func__, shmemc_context_id(ctx));

  SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_fence(ctx));
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_fence = pshmem_fence
#define shmem_fence pshmem_fence
#endif /* ENABLE_PSHMEM */

/**
 * @brief Fence operation on the default context
 *
 * Ensures ordering of put operations on the default context. All puts issued
 * before the fence will be completed before any puts after the fence can start.
 */
void shmem_fence(void) {
  logger(LOG_FENCE, "%s()", __func__);

  SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_fence(SHMEM_CTX_DEFAULT));
}
