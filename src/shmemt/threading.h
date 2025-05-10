/**
 * @file threading.h
 * @brief Threading wrapper interface for OpenSHMEM
 *
 * This header provides a wrapper layer around pthread functionality for
 * thread management and synchronization in OpenSHMEM.
 *
 * @copyright See LICENSE file at top-level
 */

#ifndef _THREADWRAP_THREADING_H
#define _THREADWRAP_THREADING_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/** Opaque thread handle type */
typedef void *threadwrap_thread_t;

/** Opaque mutex type */
typedef void *threadwrap_mutex_t;

/**
 * @brief Initialize a mutex
 * @param tp Pointer to mutex to initialize
 * @return 0 on success, non-zero on error
 */
int threadwrap_mutex_init(threadwrap_mutex_t *tp);

/**
 * @brief Destroy a mutex
 * @param mp Pointer to mutex to destroy
 * @return 0 on success, non-zero on error
 */
int threadwrap_mutex_destroy(threadwrap_mutex_t *mp);

/**
 * @brief Lock a mutex
 * @param mp Pointer to mutex to lock
 * @return 0 on success, non-zero on error
 */
int threadwrap_mutex_lock(threadwrap_mutex_t *mp);

/**
 * @brief Unlock a mutex
 * @param mp Pointer to mutex to unlock
 * @return 0 on success, non-zero on error
 */
int threadwrap_mutex_unlock(threadwrap_mutex_t *mp);

/**
 * @brief Try to lock a mutex without blocking
 * @param mp Pointer to mutex to try locking
 * @return 0 on success, non-zero on error or if mutex is already locked
 */
int threadwrap_mutex_trylock(threadwrap_mutex_t *mp);

/**
 * @brief Create a new thread
 * @param threadp Pointer to store the thread handle
 * @param start_routine Function pointer to thread entry point
 * @param args Arguments to pass to start_routine
 * @return 0 on success, non-zero on error
 */
int threadwrap_thread_create(threadwrap_thread_t *threadp,
                             void *(*start_routine)(void *), void *args);

/**
 * @brief Wait for a thread to complete
 * @param thread Handle of thread to wait for
 * @param retval Pointer to store thread return value
 * @return 0 on success, non-zero on error
 */
int threadwrap_thread_join(threadwrap_thread_t thread, void **retval);

/**
 * @brief Get the calling thread's ID
 * @return Thread handle of calling thread
 */
threadwrap_thread_t threadwrap_thread_id(void);

/**
 * @brief Compare two thread handles for equality
 * @param t1 First thread handle
 * @param t2 Second thread handle
 * @return Non-zero if threads are equal, 0 if different
 */
int threadwrap_thread_equal(threadwrap_thread_t t1, threadwrap_thread_t t2);

#endif /* ! _THREADWRAP_THREADING_H */
