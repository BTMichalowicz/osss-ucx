/**
 * @file carp.c
 * @brief Error reporting and handling functionality
 *
 * This file implements warning and fatal error reporting functions used
 * throughout the OpenSHMEM implementation.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"
#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 * @brief Internal macro for formatting and printing error messages
 *
 * @param _type Type of message (WARNING or FATAL)
 */
#define DO_CARP(_type)                                                         \
  do {                                                                         \
    va_list ap;                                                                \
                                                                               \
    fprintf(stderr, "*** PE %d: %s: ", proc.li.rank, #_type);                  \
    va_start(ap, fmt);                                                         \
    vfprintf(stderr, fmt, ap);                                                 \
    va_end(ap);                                                                \
    fprintf(stderr, " ***\n");                                                 \
    fflush(stderr);                                                            \
  } while (0)

/**
 * @brief Print a warning message to stderr
 *
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
void shmemu_warn(const char *fmt, ...) { DO_CARP(WARNING); }

/**
 * @brief Print a fatal error message and terminate the program
 *
 * Prints error message from rank 0 only and calls global exit
 *
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
void shmemu_fatal(const char *fmt, ...) {
  /* this test also handles an uninitialized state */
  if (proc.li.rank < 1) {
    DO_CARP(FATAL);
  }

  shmemc_global_exit(EXIT_FAILURE);
  /* NOT REACHED */
}
