/**
 * @file globalexit.c
 * @brief Implementation of global exit functionality for OpenSHMEM
 * communications layer
 *
 * This file provides the implementation for managing global exit operations
 * across all processing elements (PEs) in an OpenSHMEM program. It handles
 * initialization, cleanup, and coordinated program termination.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemc.h"
#include "pmi_client.h"

#include <unistd.h>

/**
 * @brief Initialize global exit handling
 *
 * Sets up the infrastructure needed for coordinated program termination
 * across all PEs. Must be called during initialization before any global
 * exit operations can be performed.
 */
void shmemc_globalexit_init(void) {}

/**
 * @brief Clean up global exit handling
 *
 * Performs cleanup of global exit resources. Must be called during finalization
 * to ensure proper shutdown of the exit monitoring system.
 */
void shmemc_globalexit_finalize(void) {}

/**
 * @brief Terminate the program across all PEs
 *
 * Initiates a coordinated shutdown of the entire OpenSHMEM program across
 * all processing elements. This function does not return.
 *
 * @param status The exit status code to be returned by the program
 */
void shmemc_global_exit(int status) {
  shmemc_pmi_client_abort("global_exit", status);
}
