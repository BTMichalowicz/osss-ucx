/**
 * @file env.h
 * @brief Environment variable handling for OpenSHMEM communications layer
 *
 * This header defines the interface for initializing and finalizing environment
 * variable settings that control OpenSHMEM runtime behavior.
 *
 * @copyright See LICENSE file at top-level
 */

#ifndef _SHMEMC_READENV_H
#define _SHMEMC_READENV_H 1

/**
 * @brief Initialize environment variable handling
 *
 * Sets up and reads environment variables that control OpenSHMEM runtime
 * behavior. Must be called during initialization before any environment
 * settings are accessed.
 */
void shmemc_env_init(void);

/**
 * @brief Clean up environment variable handling
 *
 * Performs cleanup of environment variable resources. Must be called during
 * finalization to ensure proper shutdown.
 */
void shmemc_env_finalize(void);

#endif /* ! _SHMEMC_READENV_H */
