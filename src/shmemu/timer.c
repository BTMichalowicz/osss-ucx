/**
 * @file timer.c
 * @brief Timer functionality for OpenSHMEM utilities
 *
 * This file implements high-resolution timing functions used by OpenSHMEM
 * utilities for performance measurements and timing operations.
 *
 * @copyright For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <sys/time.h>

/** Timestamp of program start used as epoch reference */
static double epoch;

/**
 * @brief Get current time with microsecond precision
 *
 * @return Current time in seconds as a double
 */
inline static double shmemu_read_time(void) {
  struct timeval now;

  if (gettimeofday(&now, NULL) != 0) {
    return 0.0;
    /* NOT REACHED */
  }

  return (double)(now.tv_sec + (now.tv_usec / 1.0e6));
}

/**
 * @brief Initialize timer by setting epoch to current time
 */
void shmemu_timer_init(void) { epoch = shmemu_read_time(); }

/**
 * @brief Finalize timer (currently a no-op)
 */
void shmemu_timer_finalize(void) { return; }

/**
 * @brief Get elapsed time since timer initialization
 *
 * @return Time in seconds since timer was initialized
 */
double shmemu_timer(void) { return shmemu_read_time() - epoch; }
