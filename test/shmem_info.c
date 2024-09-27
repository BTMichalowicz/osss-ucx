#include "../build/build/include/shmem.h"
#include <stdio.h>

int main(void) {
  shmem_init();

  int mype = shmem_my_pe();
  int npes = shmem_n_pes();

  int major, minor;
  shmem_info_get_version(&major, &minor);
  
  if ( mype == 0 ) {
    printf("OpenSHMEM version: %d.%d\n", major, minor);
  }

  char impl_name[256];
  shmem_info_get_name(impl_name);
  
  if (mype == 0) {
    printf("OpenSHMEM implementation: %s\n", impl_name);
  }

  shmem_finalize();
  return 0;
}

