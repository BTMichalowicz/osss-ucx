/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "state.h"
#include "shmem/api.h"

#ifdef ENABLE_PSHMEM
#pragma weak shmem_query_thread = pshmem_query_thread
#define shmem_query_thread pshmem_query_thread
#endif /* ENABLE_PSHMEM */

/*
 * query thread level support
 */

void
shmem_query_thread(int *provided)
{
    SHMEMU_CHECK_INIT();

    shmemu_assert(provided != NULL,
                  "shmem: the argument of %s() may not be null",
                  __func__);

    logger(LOG_INFO,
           "%s() -> %d",
           __func__,
           proc.td.osh_tl
           );

    *provided = proc.td.osh_tl;
}
