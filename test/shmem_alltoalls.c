/* For license: see LICENSE file at top-level */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 1

void test_alltoalls() {
    int npes = shmem_n_pes();
    int me = shmem_my_pe();

    // Allocate using symmetric heap allocation
    size_t array_size = NELEMS * npes * sizeof(int);
    int *source = shmem_malloc(array_size);
    int *dest = shmem_malloc(array_size);

    if (source == NULL || dest == NULL) {
        printf("PE %d: Memory allocation failed\n", me);
        shmem_global_exit(1);
        return;
    }

    // Initialize arrays with simpler pattern
    for (int i = 0; i < npes * NELEMS; i++) {
        source[i] = me;  // Each PE fills with its own PE number
        dest[i] = -1;    // Initialize dest with -1
    }

    // Debug print before operation
    if (me == 0) {
        printf("Array size: %zu bytes\n", array_size);
        printf("Number of PEs: %d\n", npes);
    }
    
    shmem_barrier_all();

    // Print initial values
    printf("PE %d: Initial source values: ", me);
    for (int i = 0; i < npes * NELEMS; i++) {
        printf("%d ", source[i]);
    }
    printf("\n");

    shmem_barrier_all();

    // Single alltoalls operation
    int ret = shmem_int_alltoalls(SHMEM_TEAM_WORLD, dest, source, 
                                 1,      // dst stride
                                 1,      // sst stride
                                 NELEMS); // number of elements

    if (ret != 0) {
        printf("PE %d: alltoalls returned error %d\n", me, ret);
    }

    shmem_barrier_all();

    // Print final values
    printf("PE %d: Final dest values: ", me);
    for (int i = 0; i < npes * NELEMS; i++) {
        printf("%d ", dest[i]);
    }
    printf("\n");

    // Verify results - each PE should have received values from all other PEs
    int errors = 0;
    for (int i = 0; i < npes; i++) {
        int expected = i;  // Should receive PE number i at position i
        if (dest[i * NELEMS] != expected) {
            printf("PE %d: Error at index %d: expected %d, got %d\n",
                   me, i, expected, dest[i * NELEMS]);
            errors++;
        }
    }

    shmem_barrier_all();

    if (me == 0) {
        if (errors == 0) {
            printf("Test completed successfully\n");
        } else {
            printf("Test failed with %d errors\n", errors);
        }
    }

    // Cleanup
    shmem_free(source);
    shmem_free(dest);
    shmem_barrier_all();
}

int main(void) {
    shmem_init();
    test_alltoalls();
    shmem_finalize();
    return 0;
}