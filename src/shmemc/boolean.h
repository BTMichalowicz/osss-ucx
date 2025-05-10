/**
 * @file boolean.h
 * @brief Boolean type definitions for OpenSHMEM communications layer
 *
 * This header provides boolean type definitions, either using the standard
 * stdbool.h if available, or defining equivalent macros if not. This ensures
 * consistent boolean type usage across the codebase.
 *
 * @copyright See LICENSE file at top-level
 */

#ifndef _SHMEMC_BOOLEAN_H
#define _SHMEMC_BOOLEAN_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_STDBOOL_H

#include <stdbool.h>

#else

/**
 * @brief Boolean type definition when stdbool.h is not available
 */
#define bool _Bool

/**
 * @brief True value definition when stdbool.h is not available
 */
#define true 1

/**
 * @brief False value definition when stdbool.h is not available
 */
#define false 0

#endif /* HAVE_STDBOOL_H */

#endif /* ! _SHMEMC_BOOLEAN_H */
