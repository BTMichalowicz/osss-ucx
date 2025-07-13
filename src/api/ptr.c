/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"

#ifdef ENABLE_PSHMEM
#pragma weak shmem_ptr = pshmem_ptr
#define shmem_ptr pshmem_ptr
#endif /* ENABLE_PSHMEM */

/**
 * @brief Get a local pointer to a symmetric data object on the specified PE
 *
 * @param target Pointer to a symmetric data object on the specified PE
 * @param pe PE number of the remote PE
 * @return Local pointer to the symmetric data object, or NULL if not accessible
 *
 * This routine returns a local pointer to a symmetric data object on the
 * specified PE. The target must be a symmetric data object that exists on the
 * calling PE. If the data object cannot be accessed via a local pointer, the
 * routine returns NULL.
 */
void *shmem_ptr(const void *target, int pe) {
  void *rw = shmemc_ctx_ptr(SHMEM_CTX_DEFAULT, target, pe);

  logger(LOG_MEMORY, "%s(target=%p, pe=%d) -> %p", __func__, target, pe, rw);

  return rw;
}
