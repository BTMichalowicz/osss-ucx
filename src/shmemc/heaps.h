/**
 * @file heaps.h
 * @brief Symmetric heap management for OpenSHMEM communications layer
 *
 * This header defines the interface for managing symmetric heaps in the
 * OpenSHMEM communications layer. Symmetric heaps are memory regions that are
 * allocated at the same virtual address across all processing elements (PEs).
 *
 * @copyright See LICENSE file at top-level
 */

#ifndef _SHMEMC_HEAPS_H
#define _SHMEMC_HEAPS_H 1

/**
 * @brief Initialize symmetric heaps
 *
 * Sets up and allocates the symmetric heap memory regions used for OpenSHMEM
 * communications. Must be called during initialization before any heap
 * operations.
 */
void shmemc_heaps_init(void);

/**
 * @brief Clean up and free symmetric heaps
 *
 * Deallocates all symmetric heap memory regions and performs necessary cleanup.
 * Must be called during finalization to prevent memory leaks.
 */
void shmemc_heaps_finalize(void);

#endif /* ! _SHMEMC_HEAPS_H */
