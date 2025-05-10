/* For license: see LICENSE file at top-level */

/**
 * @file globalexit.c
 * @brief Implementation of OpenSHMEM global exit routine
 *
 * This file contains the implementation of the global exit operation that
 * allows any PE to force termination of an entire OpenSHMEM application.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"

#ifdef ENABLE_PSHMEM
#pragma weak shmem_global_exit = pshmem_global_exit
#define shmem_global_exit pshmem_global_exit
#endif /* ENABLE_PSHMEM */

/**
 * @brief Terminate all PEs with the given status code
 *
 * @param status Exit status code to be returned by all PEs
 *
 * This routine allows any PE to force termination of an OpenSHMEM program
 * for all PEs, passing an exit status to the execution environment. When
 * any PE calls this routine, it results in the immediate notification to
 * all PEs to terminate.
 */
void shmem_global_exit(int status) {
  logger(LOG_FINALIZE, "%s(status=%d)", __func__, status);

  shmemc_global_exit(status);
}
