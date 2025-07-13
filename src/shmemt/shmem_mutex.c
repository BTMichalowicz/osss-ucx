/**
 * @file shmem_mutex.c
 * @brief Threading mutex implementation for OpenSHMEM
 *
 * This file implements mutex functionality for thread synchronization in
 * OpenSHMEM. The mutex operations are only enabled when threading support is
 * compiled in.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/*
 * This only gets used if threading enabled
 */

#ifdef ENABLE_THREADS

#include "state.h"
#include "shmem_mutex.h"
#include "shmem/defs.h"
#include "threading.h"

/** Global mutex for protecting communications */
static threadwrap_mutex_t comms_mutex;

/**
 * @brief Initialize the threading subsystem
 *
 * Initializes the global communications mutex.
 */
void shmemt_init(void) { threadwrap_mutex_init(&comms_mutex); }

/**
 * @brief Initialize mutex for thread synchronization
 *
 * Initializes the communications mutex if thread level is
 * SHMEM_THREAD_MULTIPLE.
 */
void shmemt_mutex_init(void) {
  if (proc.td.osh_tl == SHMEM_THREAD_MULTIPLE) {
    threadwrap_mutex_init(&comms_mutex);
  }
}

/**
 * @brief Destroy mutex used for thread synchronization
 *
 * Cleans up the communications mutex if thread level is SHMEM_THREAD_MULTIPLE.
 */
void shmemt_mutex_destroy(void) {
  if (proc.td.osh_tl == SHMEM_THREAD_MULTIPLE) {
    threadwrap_mutex_destroy(&comms_mutex);
  }
}

/**
 * @brief Acquire the communications mutex lock
 *
 * Locks the mutex if thread level is SHMEM_THREAD_MULTIPLE.
 */
void shmemt_mutex_lock(void) {
  if (proc.td.osh_tl == SHMEM_THREAD_MULTIPLE) {
    threadwrap_mutex_lock(&comms_mutex);
  }
}

/**
 * @brief Release the communications mutex lock
 *
 * Unlocks the mutex if thread level is SHMEM_THREAD_MULTIPLE.
 */
void shmemt_mutex_unlock(void) {
  if (proc.td.osh_tl == SHMEM_THREAD_MULTIPLE) {
    threadwrap_mutex_unlock(&comms_mutex);
  }
}

#endif /* ENABLE_THREADS */
