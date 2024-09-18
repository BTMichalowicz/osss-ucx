#include <shmem.h>
#include <stdio.h>

int main(void) {
  shmem_init();

  int mype = shmem_my_pe();
  int npes = shmem_n_pes();

  /* Check for OpenSHMEM version by querying version-specific features */
  int major, minor;
  shmem_info_get_version(&major, &minor);
  
  if ( mype == 0 ) {
    printf("OpenSHMEM version: %d.%d\n", major, minor);
  }

  /* Check for the presence of teams and contexts */
  if ( mype == 0 ) {
    #ifdef SHMEM_HAS_TEAMS
    printf("This implementation supports teams and contexts.\n");
    #else
    printf("This implementation does not support teams and contexts.\n");
    #endif
  }

  shmem_finalize();
  return 0;
}

