/**
 * @file init.c
 * @brief Initialization and finalization functionality for OpenSHMEM utilities
 *
 * This file implements initialization and cleanup functions for various
 * OpenSHMEM utility components like deprecation tracking, timers, and logging.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "init.h"

/**
 * @brief Initialize OpenSHMEM utility components
 *
 * Initializes deprecation tracking, timer, and logging subsystems.
 * Must be called before using any utility functions.
 */
void shmemu_init(void) {
  shmemu_deprecate_init();
  shmemu_timer_init();
  shmemu_logger_init();
}

/**
 * @brief Clean up OpenSHMEM utility components
 *
 * Performs cleanup of logging, timer, and deprecation tracking subsystems.
 * Should be called during program shutdown to free resources.
 */
void shmemu_finalize(void) {
  shmemu_logger_finalize();
  shmemu_timer_finalize();
  shmemu_deprecate_finalize();
}
