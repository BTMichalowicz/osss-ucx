/**
 * @file shmemx.h
 * @brief Legacy OpenSHMEM experimental extensions header file for backwards
 * compatibility
 *
 * This header file is provided for backwards compatibility with older OpenSHMEM
 * code that uses the <mpp/shmemx.h> include path. As of OpenSHMEM
 * specification 1.1, the standard header path is <shmemx.h>.
 *
 * @note For license information, see LICENSE file at top-level
 * @deprecated Use <shmemx.h> instead as per OpenSHMEM specification 1.1 and up
 */

#ifndef _MPP_SHMEMX_H
#define _MPP_SHMEMX_H 1

#warning                                                                       \
    "<mpp/shmemx.h> is a deprecated location since specification 1.1, use <shmemx.h> instead"

#include <shmemx.h>

#endif /* _MPP_SHMEMX_H */
