/**
 * @file asr.c
 * @brief Address Space Randomization (ASR) checking functionality
 *
 * This file implements checks for ASR/ASLR mismatch between requested and
 * actual settings. Currently Linux-specific implementation. See
 * https://wiki.freebsd.org/ASLR for FreeBSD information.
 *
 * @copyright See LICENSE file at top-level
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"

#include <unistd.h>
#include <sys/file.h>
#include <sys/personality.h>

/*
 * if we claimed ASR isn't here, but it actually is, let's say
 * something
 *
 * Linux specific at moment
 *   (see https://wiki.freebsd.org/ASLR)
 */

/** Name of kernel randomization control variable */
#define RAND_VARIABLE "randomize_va_space"

/** Full path to kernel randomization control file */
#define RAND_FILE "/proc/sys/kernel/" RAND_VARIABLE

/** Special value to query personality without changing it */
#define PERSONALITY_QUERY 0xffffffff

/**
 * @brief Check for mismatch between requested ASR settings and system state
 *
 * Tests if ASR/ASLR is enabled on the system when aligned addresses were
 * requested. Only the leader process performs this check.
 *
 * The check involves:
 * 1. Querying process personality flags for ADDR_NO_RANDOMIZE
 * 2. Reading the kernel's randomize_va_space setting
 * 3. Warning if randomization appears to be enabled
 *
 * @return void
 */
void shmemu_test_asr_mismatch(void) {
  if (proc.leader) {
    int p;
    int fd;
    ssize_t n;
    char inp;

    p = personality(PERSONALITY_QUERY);
    if (p & ADDR_NO_RANDOMIZE) {
      return; /* ASR disabled in this process */
              /* NOT REACHED */
    }

    fd = open(RAND_FILE, O_RDONLY, 0);
    if (fd < 0) {
      return; /* no file, carry on */
              /* NOT REACHED */
    }

    n = read(fd, &inp, 1);
    if (n < 1) {
      goto close_ret; /* can't read file, carry on */
                      /* NOT REACHED */
    }

    if (inp == '0') {
      goto close_ret; /* file starts with "0", ASR turned off */
                      /* NOT REACHED */
    }

    shmemu_warn("aligned addresses requested, but this node (%s) "
                "appears to have randomization enabled "
                "(%s = %c)",
                proc.nodename, RAND_VARIABLE, inp);

  close_ret:
    (void)close(fd);
  }
}
