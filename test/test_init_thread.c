#include "../build/build/include/shmem.h"
#include <stdio.h>

int main(void) {
  int provided, queried;
  shmem_init_thread(SHMEM_THREAD_MULTIPLE, &provided);


  int mype = shmem_my_pe();
  int npes = shmem_n_pes();


  for (int i = 0; i < npes; i++) {
    if (i == mype) {
      printf("Hello from PE %d\n", i);
    }
  }

  shmem_barrier_all();
  shmem_query_thread(&queried);

  if (mype == 0) {
    printf("Requested thread level: SHMEM_THREAD_MULTIPLE\n");
    printf("Provided thread level: %d\n", provided);
    printf("Queried thread level: %d\n", queried);
  }


  shmem_finalize();
  return 0;
}
