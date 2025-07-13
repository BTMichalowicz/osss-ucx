/**
 * @file ranks.c
 * @brief Implementation of OpenSHMEM rank query operations
 *
 * For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemc.h"
#include "shmemu.h"

/**
 * @file ranks.c
 * @brief Implementation of OpenSHMEM rank query operations
 */

#ifdef ENABLE_PSHMEM
#pragma weak shmem_my_pe = pshmem_my_pe
#define shmem_my_pe pshmem_my_pe
#pragma weak shmem_n_pes = pshmem_n_pes
#define shmem_n_pes pshmem_n_pes
#endif /* ENABLE_PSHMEM */

/**
 * @brief Returns the number of the calling PE
 *
 * This routine returns the PE number of the calling PE.
 * The result is a number between 0 and N-1, where N is the total number of PEs.
 *
 * @return The number of the calling PE
 */
int shmem_my_pe(void) {
  int my;

  SHMEMU_CHECK_INIT();

  my = shmemc_my_pe();

  logger(LOG_RANKS, "%s() -> %d", __func__, my);

  return my;
}

/**
 * @brief Returns the number of PEs running in the program
 *
 * This routine returns the number of PEs running in the program.
 *
 * @return The number of PEs running in the program
 */
int shmem_n_pes(void) {
  int n;

  SHMEMU_CHECK_INIT();

  n = shmemc_n_pes();

  logger(LOG_RANKS, "%s() -> %d", __func__, n);

  return n;
}
