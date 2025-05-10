/**
 * @file unitparse.c
 * @brief Unit parsing and formatting utilities for OpenSHMEM
 *
 * This file implements functions for parsing size units (KB, MB, GB, etc.)
 * and formatting human-readable number representations.
 *
 * For license: see LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>

/**
 * Define accepted size units in ascending order, fits in size_t
 *
 * See section 3.1 in http://physics.nist.gov/Pubs/SP330/sp330.pdf
 */
static const char units_string[] = "KMGTPE";
static const size_t multiplier = 1024;

/**
 * @brief Parse a scaling unit character and calculate its numeric value
 *
 * @param u Unit character to parse (K, M, G, T, P, E)
 * @param sp Pointer to store the calculated numeric value
 * @return 0 on success, -1 if unit not recognized
 */
static int parse_unit(char u, size_t *sp) {
  int foundit = 0;
  char *usp = (char *)units_string;
  size_t bytes = multiplier;

  u = toupper(u);
  while (*usp != '\0') {
    if (*usp == u) {
      foundit = 1;
      break;
      /* NOT REACHED */
    }
    bytes *= multiplier;
    ++usp;
  }

  if (foundit) {
    *sp = bytes;
    return 0;
  } else {
    *sp = 0;
    return -1;
  }
}

/**
 * @brief Parse a size string with optional scaling units
 *
 * Segment size can be expressed with scaling units. This function parses
 * those and returns the scaled size in bytes.
 *
 * @param size_str String containing size with optional unit suffix
 * @param bytes_p Pointer to store the calculated size in bytes
 * @return 0 on success, -1 on parsing error
 */
int shmemu_parse_size(const char *size_str, size_t *bytes_p) {
  char *units; /* scaling factor if given */
  double bytes;

  bytes = strtod(size_str, &units);
  if (bytes < 0.0) {
    return -1;
    /* NOT REACHED */
  }

  if (*units != '\0') {
    /* something that could be a unit */
    size_t b;

    /* but we don't know what that unit is */
    if (parse_unit(*units, &b) != 0) {
      return -1;
      /* NOT REACHED */
    }

    /* scale for return */
    bytes *= b;
  }

  *bytes_p = (size_t)bytes;

  return 0;
}

/**
 * @brief Format a byte count into a human-readable string
 *
 * Does the reverse of parse_size: converts a byte count into a human-readable
 * form with appropriate units (K, M, G, etc.)
 *
 * @param bytes Number of bytes to format
 * @param buf Buffer to store formatted string
 * @param buflen Length of output buffer
 * @return 0 on success, -1 on error
 */
int shmemu_human_number(double bytes, char *buf, size_t buflen) {
  char *walk = (char *)units_string;
  unsigned wc = 0; /* walk count */
  size_t divvy = multiplier;
  double b = bytes;
  char unit = '\0';

  while (*walk) {
    const size_t d = (size_t)bytes / divvy;

    /* find when we've gone too far */
    if (d == 0) {
      if (wc > 0) {
        walk -= 1;
        unit = *walk;
      }
      break;
      /* NOT REACHED */
    }
    ++wc, ++walk;
    divvy *= multiplier;
    b /= multiplier;
  }

  snprintf(buf, buflen, "%.1f%c", b, unit);

  return 0;
}

/**
 * @brief Convert boolean value to human-readable string
 *
 * @param v Boolean value (0 or non-zero)
 * @return "no" for 0, "yes" for non-zero
 */
const char *shmemu_human_option(int v) { return (v == 0) ? "no" : "yes"; }
