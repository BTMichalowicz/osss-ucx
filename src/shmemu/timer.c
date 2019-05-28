/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"

#include <stdio.h>
#include <sys/time.h>

static double epoch;

inline static double
shmemu_read_time(void)
{
    struct timeval t;

    if (gettimeofday(&t, NULL) != 0) {
        return 0.0;
        /* NOT REACHED */
    }

    return (double) (t.tv_sec + (t.tv_usec / 1.0e6));
}

void
shmemu_timer_init(void)
{
    epoch = shmemu_read_time();
}

void
shmemu_timer_finalize(void)
{
    return;
}

double
shmemu_timer(void)
{
    return shmemu_read_time() - epoch;
}
