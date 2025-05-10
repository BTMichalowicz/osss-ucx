/**
 * @file threadlevels.c
 * @brief Thread level support for OpenSHMEM utilities
 *
 * This file implements functions for converting between thread level
 * constants and their string representations.
 *
 * @copyright For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmem.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

/**
 * @brief Helper macro to create thread level table entries
 * @param _thr Thread level name (e.g. SINGLE, MULTIPLE)
 */
#define GLUE(_thr)                                                             \
  { SHMEM_THREAD_##_thr, #_thr }

/** Value indicating unknown/invalid thread level */
#define NONE (SHMEM_THREAD_SINGLE - 1)

/** Structure mapping thread level constants to names */
static struct thread_encdec {
  int level;        /**< Thread level constant */
  const char *name; /**< Thread level name string */
} threads_table[] = {GLUE(SINGLE),
                     GLUE(FUNNELED),
                     GLUE(SERIALIZED),
                     GLUE(MULTIPLE),
                     {NONE, "unknown"}};

/**
 * @brief Get string name for thread level constant
 *
 * @param tl Thread level constant
 * @return String representation of thread level
 */
const char *shmemu_thread_name(int tl) {
  struct thread_encdec *tp = threads_table;

  while (tp->level != NONE) {
    if (tp->level == tl) {
      return tp->name;
      /* NOT REACHED */
    }
    ++tp;
  }
  return tp->name;
}

/**
 * @brief Get thread level constant from string name
 *
 * @param tn Thread level name string
 * @return Thread level constant, or NONE if not found
 */
int shmemu_thread_level(const char *tn) {
  struct thread_encdec *tp = threads_table;

  while (tp->level != NONE) {
    if (strncmp(tp->name, tn, strlen(tp->name)) == 0) {
      return tp->level;
      /* NOT REACHED */
    }
    ++tp;
  }
  return tp->level;
}
