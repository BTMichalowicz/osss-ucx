/**
 * @file state.h
 * @brief Processing Element (PE) state management interface
 *
 * This header provides access to the global PE state information used by
 * the OpenSHMEM communications layer. It exposes the main state structure
 * that tracks initialization status, reference counting, rank information,
 * and other PE-specific details.
 *
 * @copyright See LICENSE file at top-level
 */

#ifndef _SHMEMC_STATE_H
#define _SHMEMC_STATE_H 1

#include "thispe.h"

/**
 * @brief Global PE state information
 *
 * External declaration of the global state structure that contains
 * initialization status, reference counting, rank, progress thread status,
 * and node identification for this Processing Element (PE).
 */
extern thispe_info_t proc;

#endif /* ! _SHMEMC_STATE_H */
