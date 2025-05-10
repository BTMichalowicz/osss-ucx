/**
 * @file quiet.c
 * @brief Implementation of OpenSHMEM quiet operations
 *
 * Quiet operations ensure completion of remote memory updates.
 *
 * For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"
#include "shmem_mutex.h"

/**
 * @file quiet.c
 * @brief Implementation of OpenSHMEM quiet operations
 *
 * Quiet operations ensure completion of remote memory updates.
 */

#ifdef ENABLE_PSHMEM
#pragma weak shmem_ctx_quiet = pshmem_ctx_quiet
#define shmem_ctx_quiet pshmem_ctx_quiet
#endif /* ENABLE_PSHMEM */

/**
 * @brief Ensures completion of all remote memory updates issued to a context
 *
 * This operation ensures completion of all remote memory updates issued to a
 * specific context prior to this call.
 *
 * @param ctx   The context on which to ensure completion of updates
 */
void shmem_ctx_quiet(shmem_ctx_t ctx) {
  logger(LOG_QUIET, "%s(ctx=%lu)", __func__, shmemc_context_id(ctx));

  SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_quiet(ctx));
}

#ifdef ENABLE_PSHMEM
#pragma weak shmem_quiet = pshmem_quiet
#define shmem_quiet pshmem_quiet
#endif /* ENABLE_PSHMEM */

/**
 * @brief Ensures completion of all remote memory updates
 *
 * This operation ensures completion of all remote memory updates issued by the
 * calling PE prior to this call using the default context.
 */
void shmem_quiet(void) {
  logger(LOG_QUIET, "%s()", __func__);

  SHMEMT_MUTEX_NOPROTECT(shmemc_ctx_quiet(SHMEM_CTX_DEFAULT));
}
