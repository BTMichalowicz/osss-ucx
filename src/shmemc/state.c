/**
 * @file state.c
 * @brief Processing Element (PE) state management
 *
 * This file manages the state information for each Processing Element (PE)
 * in the OpenSHMEM runtime environment. It maintains initialization status,
 * reference counting, rank information, and other PE-specific details.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "thispe.h"

/**
 * @brief Global PE state information structure
 *
 * Contains the initial state for this Processing Element (PE), tracking
 * initialization status, reference counting, rank, progress thread status,
 * and node identification.
 */
thispe_info_t proc = {
    .status = SHMEMC_PE_UNKNOWN, /* uninitialized */
    .refcount = 0,               /* init never called */
    .li.rank = -1,               /* undefined position */
    .progress_thread = false,    /* no special progress */
    .nodename = NULL,            /* node on which this PE runs */
};
