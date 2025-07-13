/* For license: see LICENSE file at top-level */

/**
 * @file accessible.c
 * @brief Implementation of OpenSHMEM accessibility check routines
 *
 * This file contains implementations of routines to check whether a PE
 * or memory address is accessible via OpenSHMEM operations.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"

#ifdef ENABLE_PSHMEM
#pragma weak shmem_pe_accessible = pshmem_pe_accessible
#define shmem_pe_accessible pshmem_pe_accessible
#pragma weak shmem_addr_accessible = pshmem_addr_accessible
#define shmem_addr_accessible pshmem_addr_accessible
#endif /* ENABLE_PSHMEM */

/**
 * @brief Check if a PE is accessible via OpenSHMEM operations
 *
 * @param pe Processing Element (PE) number to check
 * @return Non-zero if PE is accessible, 0 otherwise
 */
int shmem_pe_accessible(int pe) {
  const int s = shmemc_pe_accessible(pe);

  logger(LOG_INFO, "%s(pe=%d) -> %d", __func__, pe, s);

  return s;
}

/**
 * @brief Check if an address is accessible via OpenSHMEM operations
 *
 * @param addr Address to check
 * @param pe Processing Element (PE) number where address resides
 * @return Non-zero if address is accessible, 0 otherwise
 */
int shmem_addr_accessible(const void *addr, int pe) {
  const int s = shmemc_addr_accessible(addr, pe);

  logger(LOG_INFO, "%s(addr=%p, pe=%d) -> %d", __func__, addr, pe, s);

  return s;
}
