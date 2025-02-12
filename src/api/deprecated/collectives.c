#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmem_mutex.h"
#include "shmemu.h"
#include "shmemc.h"
#include "shmem/api.h"

#include <bits/wordsize.h>
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

/*
 * ---------------------------------------------------------------------
 *
 * Deprecations as of 1.5.
 *
 */

static const shmemu_version_t v = {.major = 1, .minor = 5};
