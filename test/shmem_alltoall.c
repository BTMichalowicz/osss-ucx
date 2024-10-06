/* For license: see LICENSE file at top-level */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 1  // Send one element per PE

void test_alltoall_simple() {
    int npes = shmem_n_pes();
    int me = shmem_my_pe();

    // Allocate and initialize source and destination arrays
    int *source = shmem_malloc(NELEMS * npes * sizeof(int));
    int *dest = shmem_malloc(NELEMS * npes * sizeof(int));

    if (source == NULL || dest == NULL) {
        printf("PE %d: Memory allocation failed\n", me);
        shmem_finalize();
        exit(1);
    }

    // Initialize source array: each PE sets source[j] = me + 1
    for (int j = 0; j < npes; j++) {
        source[j * NELEMS] = me + 1;
    }

    // Initialize destination array to -1 for verification
    for (int j = 0; j < npes * NELEMS; j++) {
        dest[j] = -1;
    }

    // Print initial source array
    printf("PE %d: Initial source array: ", me);
    for (int j = 0; j < npes; j++) {
        printf("%d ", source[j * NELEMS]);
    }
    printf("\n");

    // Synchronize before all-to-all
    shmem_barrier_all();

    // Perform all-to-all communication
    int ret = shmem_int_alltoall(SHMEM_TEAM_WORLD, dest, source, NELEMS);
    if (ret != 0) {
        printf("PE %d: shmem_int_alltoall failed with return code %d\n", me, ret);
        shmem_free(source);
        shmem_free(dest);
        shmem_finalize();
        exit(1);
    }

    // Synchronize after all-to-all
    shmem_barrier_all();

    // Print resulting destination array
    printf("PE %d: Resulting destination array: ", me);
    for (int j = 0; j < npes; j++) {
        printf("%d ", dest[j * NELEMS]);
    }
    printf("\n");

    // Verify the results: dest[j] should be j + 1
    int errors = 0;
    for (int j = 0; j < npes; j++) {
        int expected = j + 1;  // Since source[j] = j + 1
        if (dest[j * NELEMS] != expected) {
            printf("PE %d: Error at index %d, expected %d, got %d\n",
                   me, j * NELEMS, expected, dest[j * NELEMS]);
            errors++;
        }
    }

    if (errors == 0) {
        printf("PE %d: Alltoall test passed\n", me);
    } else {
        printf("PE %d: Alltoall test failed with %d errors\n", me, errors);
    }

    // Free allocated memory
    shmem_free(source);
    shmem_free(dest);
}

int main(int argc, char *argv[]) {
    shmem_init();

    test_alltoall_simple();

    shmem_finalize();
    return 0;
}