#if ENABLE_SHMEM_ENCRYPTION


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */



#include "shmemx.h"
#include "shmemu.h"


#ifdef ENABLE_PSHMEM
#pragma weak shmemx_sec_init = pshmemx_sec_init
#define shmemx_sec_init = pshmem_sec_init
#endif /* ENABLE_PSHMEM */




#endif /* ENABLE_SHMEM_ENCRYPTION */
