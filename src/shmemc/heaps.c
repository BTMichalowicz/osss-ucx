/**
 * @file heaps.c
 * @brief Implementation of symmetric heap management functions
 *
 * This file provides the implementation for initializing and finalizing
 * symmetric heaps in the OpenSHMEM communications layer. Symmetric heaps
 * are memory regions allocated at the same virtual address across all PEs.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "state.h"
#include "shmemu.h"
#include "module.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Initialize symmetric heaps
 *
 * Allocates and initializes the symmetric heap memory regions:
 * - Sets number of heaps (currently fixed at 1)
 * - Allocates array to store heap sizes
 * - Parses heap size specification from environment
 *
 * Asserts that memory allocation succeeds and heap size parsing is valid.
 */
void shmemc_heaps_init(void) {
  size_t hs;
  int r;

  /* for now: could change with multiple heaps */
  proc.heaps.nheaps = 1;

  hs = proc.heaps.nheaps * sizeof(*proc.heaps.heapsize);

  proc.heaps.heapsize = (size_t *)malloc(hs);

  shmemu_assert(proc.heaps.heapsize != NULL,
                MODULE ": can't allocate memory for %lu heap%s",
                (unsigned long)proc.heaps.nheaps,
                shmemu_plural(proc.heaps.nheaps));

  r = shmemu_parse_size(proc.env.heap_spec, &proc.heaps.heapsize[0]);
  shmemu_assert(r == 0, MODULE ": couldn't work out requested heap size \"%s\"",
                proc.env.heap_spec);
}

/**
 * @brief Clean up and free symmetric heaps
 *
 * Frees the memory allocated for storing heap sizes during initialization.
 */
void shmemc_heaps_finalize(void) { free(proc.heaps.heapsize); }
