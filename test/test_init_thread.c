#include "../build/build/include/shmem.h"
#include <stdio.h>

int main(void) {
  shmem_init_thread(SHMEM_THREAD_MULTIPLE, NULL);

  int mype = shmem_my_pe();
  int npes = shmem_n_pes();

  printf("Hello from PE %d of %d\n", mype, npes);

  shmem_finalize();
  return 0;
}
