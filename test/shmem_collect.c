/* For license: see LICENSE file at top-level */
/* shmem_collect.c */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 1 // Collect one element per PE

void test_collect_simple() {
  int npes = shmem_n_pes();
  int me = shmem_my_pe();

  // Allocate and initialize source and destination arrays
  int *source = shmem_malloc(
      NELEMS * sizeof(int)); // Only need NELEMS elements for source
  int *dest =
      shmem_malloc(NELEMS * npes * sizeof(int)); // Need npes * NELEMS for dest

  if (source == NULL || dest == NULL) {
    printf("PE %d: Memory allocation failed\n", me);
    if (source)
      shmem_free(source); // Free if allocated
    if (dest)
      shmem_free(dest);   // Free if allocated
    shmem_global_exit(1); // Exit all PEs on error
    return;
  }

  // Initialize source array: each PE sets its source to its PE number
  source[0] = me;

  // Initialize destination array to -1 for verification
  for (int j = 0; j < npes * NELEMS; j++) {
    dest[j] = -1;
  }

  // Print initial source value
  printf("PE %d: Initial source value: %d\n", me, source[0]);

  // Synchronize before collect
  shmem_barrier_all();

  // Perform collect operation
  int ret = shmem_int_collect(SHMEM_TEAM_WORLD, dest, source, NELEMS);
  if (ret != 0) {
    printf("PE %d: Collect operation failed with return code %d\n", me, ret);
    shmem_free(source);
    shmem_free(dest);
    shmem_global_exit(1);
    return;
  }

  // Synchronize after collect
  shmem_barrier_all();

  // Print resulting destination array
  printf("PE %d: Resulting destination array: ", me);
  for (int j = 0; j < npes; j++) {
    printf("%d ", dest[j]);
  }
  printf("\n");

  // Verify the results: dest[j] should be PE number j
  int errors = 0;
  for (int j = 0; j < npes; j++) {
    if (dest[j] != j) {
      printf("PE %d: Error at index %d, expected %d, got %d\n", me, j, j,
             dest[j]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: Collect test passed\n", me);
  } else {
    printf("PE %d: Collect test failed with %d errors\n", me, errors);
  }

  // Free allocated memory
  shmem_free(source);
  shmem_free(dest);
}

int main(int argc, char *argv[]) {
  shmem_init();

  test_collect_simple();

  shmem_finalize();
  return 0;
}