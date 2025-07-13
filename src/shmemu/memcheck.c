/**
 * @file memcheck.c
 * @brief Memory corruption and usage error handlers for symmetric heap
 *
 * This file implements handlers for detecting and reporting memory corruption
 * and usage errors in the symmetric heap allocation system.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "internal-malloc.h"
#include "shmemu.h"
#include "shmemc.h"
#include "state.h"

/**
 * @brief Handler for memory corruption detected in dlmalloc mspace
 *
 * Reports symmetric heap corruption and optionally terminates program
 * if memfatal environment variable is set.
 *
 * @param m The mspace where corruption was detected
 */
void report_corruption(mspace m) {
  shmemu_warn("SYMMETRIC HEAP CORRUPTION DETECTED IN SPACE %p", m);

  if (proc.env.memfatal) {
    shmemc_global_exit(1);
    /* NOT REACHED */
  }
}

/**
 * @brief Handler for memory usage errors like allocation overflow
 *
 * Reports symmetric heap usage errors and optionally terminates program
 * if memfatal environment variable is set.
 *
 * @param m The mspace where the error occurred
 * @param p The address that triggered the error
 */
void report_usage_error(mspace m, void *p) {
  shmemu_warn("SYMMETRIC HEAP ERROR DETECTED IN SPACE %p, ADDRESS %p", m, p);

  if (proc.env.memfatal) {
    shmemc_global_exit(1);
    /* NOT REACHED */
  }
}
