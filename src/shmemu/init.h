/**
 * @file init.h
 * @brief Initialization and finalization functionality for OpenSHMEM utilities
 *
 * This header declares initialization and cleanup functions for various
 * OpenSHMEM utility components like deprecation tracking, logging, and timers.
 *
 * @copyright See LICENSE file at top-level
 */

#ifndef _SHMEMU_INIT_H
#define _SHMEMU_INIT_H 1

#ifdef ENABLE_LOGGING

/**
 * @brief Initialize the deprecation tracking system
 */
void shmemu_deprecate_init(void);

/**
 * @brief Clean up the deprecation tracking system
 */
void shmemu_deprecate_finalize(void);

/**
 * @brief Initialize the logging system
 */
void shmemu_logger_init(void);

/**
 * @brief Clean up the logging system
 */
void shmemu_logger_finalize(void);

#else /* ENABLE_LOGGING */

#define shmemu_logger_init()
#define shmemu_logger_finalize()

#define shmemu_deprecate_init()
#define shmemu_deprecate_finalize()

#endif /* ENABLE_LOGGING */

/**
 * @brief Initialize the timer system
 */
void shmemu_timer_init(void);

/**
 * @brief Clean up the timer system
 */
void shmemu_timer_finalize(void);

#endif /* ! _SHMEMU_INIT_H */
