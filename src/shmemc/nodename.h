/**
 * @file nodename.h
 * @brief Node name handling for OpenSHMEM communications layer
 *
 * This header defines the interface for initializing and finalizing node name
 * handling functionality. Node names are used to identify individual nodes in
 * the OpenSHMEM runtime environment.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/**
 * @brief Initialize node name handling
 *
 * Sets up node name functionality. Must be called during initialization
 * before any node name operations are performed.
 */
void shmemc_nodename_init(void);

/**
 * @brief Clean up node name handling
 *
 * Performs cleanup of node name resources. Must be called during
 * finalization to ensure proper shutdown.
 */
void shmemc_nodename_finalize(void);
