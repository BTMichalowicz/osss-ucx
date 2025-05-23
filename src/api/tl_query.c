/**
 * @file tl_query.c
 * @brief Implementation of OpenSHMEM thread level query operation
 *
 * This file provides the shmem_query_thread operation which returns the level
 * of thread support provided by the library.
 *
 * For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "state.h"

#ifdef ENABLE_PSHMEM
#pragma weak shmem_query_thread = pshmem_query_thread
#define shmem_query_thread pshmem_query_thread
#endif /* ENABLE_PSHMEM */

/**
 * @brief Query the thread level support provided by the OpenSHMEM
 * implementation
 *
 * @param[out] provided Returns the thread level support value
 *
 * The function returns the thread level support that was established during
 * initialization. The value returned will be one of:
 * - SHMEM_THREAD_SINGLE
 * - SHMEM_THREAD_FUNNELED
 * - SHMEM_THREAD_SERIALIZED
 * - SHMEM_THREAD_MULTIPLE
 */
void shmem_query_thread(int *provided) {
  SHMEMU_CHECK_INIT();
  SHMEMU_CHECK_NOT_NULL(provided, 1);

  logger(LOG_INFO, "%s() -> %d", __func__, proc.td.osh_tl);

  *provided = proc.td.osh_tl;
}
