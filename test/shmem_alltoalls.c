/* For license: see LICENSE file at top-level */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 1  // Send one element per PE

/**
 * @brief Alltoalls Test
 *
 * This test performs multiple all-to-all communications in succession to verify
 * the consistency and reliability of the shmem_alltoalls function over
 * multiple iterations. It ensures that repeated collective operations work 
 * correctly without data corruption or communication failures.
 */
void test_alltoalls() {
    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    const int NUM_ITERATIONS = 5;  // Number of all-to-all iterations

    // Allocate and initialize source and destination arrays
    int *source = shmem_malloc(NELEMS * npes * sizeof(int));
    int *dest = shmem_malloc(NELEMS * npes * sizeof(int));

    if (source == NULL || dest == NULL) {
        printf("PE %d: Memory allocation failed\n", me);
        shmem_finalize();
        exit(EXIT_FAILURE);
    }

    // Initialize source array: each PE sets source[j] = (me + 1) * 10
    for (int j = 0; j < npes; j++) {
        source[j * NELEMS] = (me + 1) * 10 + j;
    }

    // Initialize destination array to -1 for verification
    for (int j = 0; j < npes * NELEMS; j++) {
        dest[j] = -1;
    }

    // Perform multiple all-to-all communications
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        // Print iteration header
        printf("PE %d: Starting alltoalls iteration %d\n", me, iter + 1);

        // Synchronize before all-to-all
        shmem_barrier_all();

        // Perform all-to-all communication
        shmem_int_alltoalls(dest, source, NELEMS, 1, 1, 0, 0, npes, NULL, NULL);

        // Synchronize after all-to-all
        shmem_barrier_all();

        // Print resulting destination array
        printf("PE %d: Iteration %d destination array: ", me, iter + 1);
        for (int j = 0; j < npes; j++) {
            printf("%d ", dest[j * NELEMS]);
        }
        printf("\n");

        // Verify the results: dest[j] should be (j + 1) * 10 + me
        int errors = 0;
        for (int j = 0; j < npes; j++) {
            int expected = (j + 1) * 10 + me;
            if (dest[j * NELEMS] != expected) {
                printf("PE %d: Error at iteration %d, index %d, expected %d, got %d\n",
                       me, iter + 1, j * NELEMS, expected, dest[j * NELEMS]);
                errors++;
            }
        }

        if (errors == 0) {
            printf("PE %d: Alltoalls iteration %d passed\n", me, iter + 1);
        } else {
            printf("PE %d: Alltoalls iteration %d failed with %d errors\n", me, iter + 1, errors);
        }

        // Reinitialize destination array for the next iteration
        for (int j = 0; j < npes * NELEMS; j++) {
            dest[j] = -1;
        }
    }

    // Free allocated memory
    shmem_free(source);
    shmem_free(dest);
}
