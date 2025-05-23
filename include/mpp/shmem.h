/**
 * @file shmem.h
 * @brief Legacy OpenSHMEM header file for backwards compatibility
 *
 * This header file is provided for backwards compatibility with older OpenSHMEM
 * code that uses the <mpp/shmem.h> include path. As of OpenSHMEM
 * specification 1.1, the standard header path is <shmem.h>.
 *
 * @note For license information, see LICENSE file at top-level
 * @deprecated Use <shmem.h> instead as per OpenSHMEM specification 1.1 and up
 */

#ifndef _MPP_SHMEM_H
#define _MPP_SHMEM_H 1

#warning                                                                       \
    "<mpp/shmem.h> is deprecated: with specification 1.1 and up, use <shmem.h> instead"

#include <shmem.h>

#endif /* _MPP_SHMEM_H */
