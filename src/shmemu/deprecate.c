/**
 * @file deprecate.c
 * @brief Deprecation tracking and reporting functionality
 *
 * This file implements functionality to track and report deprecated API usage
 * in OpenSHMEM. It uses a hash table to ensure each deprecated function is
 * only reported once.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "boolean.h"

#include "../klib/khash.h"

/** Initialize hash table mapping function names (string) to bool */
KHASH_MAP_INIT_STR(deprecations, bool)

/** Hash table to track which deprecated functions have been reported */
static khash_t(deprecations) * table;

/**
 * @brief Report deprecated function usage
 *
 * Records and reports the first usage of a deprecated function. Subsequent
 * calls for the same function are silently ignored to avoid excessive noise.
 *
 * @param fn_name Name of the deprecated function
 * @param vp Pointer to version info indicating when function was deprecated,
 *           or NULL if version unknown
 */
void shmemu_deprecate(const char *fn_name, const shmemu_version_t *vp) {
  khiter_t k;
  int ret;

  /* already there? */
  k = kh_get(deprecations, table, fn_name);
  if (k != kh_end(table)) {
    return;
    /* NOT REACHED */
  }

  k = kh_put(deprecations, table, fn_name, &ret);
  /* ignore return status */

  kh_value(table, k) = true;

  if (vp != NULL) {
    logger(LOG_DEPRECATE, "\"%s\" is deprecated as of specification %d.%d",
           fn_name, vp->major, vp->minor);
  } else {
    logger(LOG_DEPRECATE, "\"%s\" is deprecated", fn_name);
  }
}

/**
 * @brief Initialize the deprecation tracking system
 *
 * Creates the hash table used to track deprecated function usage.
 */
void shmemu_deprecate_init(void) { table = kh_init(deprecations); }

/**
 * @brief Clean up the deprecation tracking system
 *
 * Destroys the hash table used to track deprecated function usage.
 */
void shmemu_deprecate_finalize(void) { kh_destroy(deprecations, table); }
