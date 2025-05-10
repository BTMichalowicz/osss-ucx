/**
 * @file plural.c
 * @brief Pluralization helper for OpenSHMEM utilities
 *
 * This file implements a simple pluralization function that returns
 * the appropriate suffix for making English nouns plural based on
 * their ending and count.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <string.h>

/**
 * @brief Get the plural suffix for a noun based on count
 *
 * Returns the appropriate English plural suffix ("s" or "es") for a noun
 * based on its ending character and count. Returns empty string for
 * singular (n=1).
 *
 * Currently handles:
 * - Words ending in 'h' get "es" (e.g. "match" -> "matches")
 * - All other words get "s"
 *
 * @param noun The noun to pluralize
 * @param n The count/number of items
 * @return The plural suffix to append ("", "s", or "es")
 */
const char *shmemu_plural(const char *noun, size_t n) {
  int eos = strlen(noun);
  char last = noun[eos - 1];
  const char *ess = "h";

  if (n == 1) {
    return "";
    /* NOT REACHED */
  }

  if (strchr(ess, last) != NULL) {
    return "es";
  } else {
    return "s";
  }
}
