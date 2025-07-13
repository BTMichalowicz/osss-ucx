/**
 * @file nodename.c
 * @brief Node name initialization and finalization routines
 *
 * This file provides functionality to initialize and finalize the node name
 * for the current process. It attempts to get the node name using either
 * gethostname() or uname(), falling back to "unknown" if neither is available.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>

#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif /* HAVE_UNAME */

/**
 * @brief Initialize the node name for the current process
 *
 * Attempts to get the node name using gethostname() if available,
 * falls back to uname() if available, otherwise uses "unknown".
 * The node name is stored in proc.nodename and must be freed
 * by calling shmemc_nodename_finalize().
 */
void shmemc_nodename_init(void) {
#if defined(HAVE_GETHOSTNAME)

  char nodename[MAXHOSTNAMELEN];
  const int s = gethostname(nodename, MAXHOSTNAMELEN);

  if (s == 0) {
    proc.nodename = strdup(nodename); /* free@end */
    return;
    /* NOT REACHED */
  }

#elif defined(HAVE_UNAME)

  struct utsname u;
  const int s = uname(&u);

  if (s == 0) {
    proc.nodename = strdup(u.nodename); /* free@end */
    return;
    /* NOT REACHED */
  }

#endif /* hostname check */

  proc.nodename = strdup("unknown"); /* free@end */
}

/**
 * @brief Free the node name memory
 *
 * Frees the memory allocated for proc.nodename during initialization.
 */
void shmemc_nodename_finalize(void) { free(proc.nodename); }
