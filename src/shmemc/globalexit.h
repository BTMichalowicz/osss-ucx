/**
 * @file globalexit.h
 * @brief Global exit handling for OpenSHMEM communications layer
 *
 * This header defines the interface for managing global exit functionality
 * in the OpenSHMEM communications layer. Global exit ensures proper cleanup
 * and termination across all processing elements (PEs).
 *
 * @copyright See LICENSE file at top-level
 */

#ifndef _SHMEMC_GLOBALEXIT_H
#define _SHMEMC_GLOBALEXIT_H 1

/**
 * @brief Initialize global exit handling
 *
 * Sets up the global exit infrastructure. Must be called during initialization
 * before any global exit operations can be performed.
 */
void shmemc_globalexit_init(void);

/**
 * @brief Clean up global exit handling
 *
 * Performs cleanup of global exit resources. Must be called during finalization
 * to ensure proper shutdown.
 */
void shmemc_globalexit_finalize(void);

#endif /* ! _SHMEMC_GLOBALEXIT_H */
