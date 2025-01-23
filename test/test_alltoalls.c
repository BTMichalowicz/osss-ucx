/* For license: see LICENSE file at top-level */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 1
#define HLINE "----------------------------------------"

/* Test sized alltoalls (64 bit) */
void test_alltoalls64() {
    int mype = shmem_my_pe();
    int npes = shmem_n_pes();
    
    /* Allocate arrays */
    long *source = shmem_malloc(NELEMS * npes * sizeof(long));
    long *dest = shmem_malloc(NELEMS * npes * sizeof(long));
    long *pSync = shmem_malloc(SHMEM_ALLTOALLS_SYNC_SIZE * sizeof(long));

    /* Initialize pSync */
    for (int i = 0; i < SHMEM_ALLTOALLS_SYNC_SIZE; i++) {
        pSync[i] = SHMEM_SYNC_VALUE;
    }

    /* Initialize arrays */
    for (int i = 0; i < npes * NELEMS; i++) {
        source[i] = mype;
        dest[i] = -1;
    }
    
    if (mype == 0) {
        printf("PE %d: source = ", mype);
        for (int i = 0; i < npes * NELEMS; i++) {
            printf("%ld ", source[i]);
        }
        printf("\n");
    }

    shmem_barrier_all();
    
    /* Perform alltoalls */
    shmem_alltoalls64(dest, source, 1, 1, NELEMS, 0, 0, npes, pSync);

    /* Verify results */
    if (mype == 0) {
        printf("PE %d: Result = ", mype);
        for (int i = 0; i < npes * NELEMS; i++) {
            printf("%ld ", dest[i]);
        }
        printf("\n");
    }

    shmem_barrier_all();
    shmem_free(source);
    shmem_free(dest);
    shmem_free(pSync);
}

/* Test typed alltoalls */
void test_alltoalls_type() {
    int mype = shmem_my_pe();
    int npes = shmem_n_pes();
    
    /* Allocate arrays */
    int *source = shmem_malloc(NELEMS * npes * sizeof(int));
    int *dest = shmem_malloc(NELEMS * npes * sizeof(int));

    /* Initialize arrays */
    for (int i = 0; i < npes * NELEMS; i++) {
        source[i] = mype;
        dest[i] = -1;
    }
    
    if (mype == 0) {
        printf("PE %d: source = ", mype);
        for (int i = 0; i < npes * NELEMS; i++) {
            printf("%d ", source[i]);
        }
        printf("\n");
    }

    shmem_barrier_all();
    
    /* Perform alltoalls */
    shmem_int_alltoalls(SHMEM_TEAM_WORLD, dest, source, 1, 1, NELEMS);

    /* Verify results */
    if (mype == 0) {
        printf("PE %d: Result = ", mype);
        for (int i = 0; i < npes * NELEMS; i++) {
            printf("%d ", dest[i]);
        }
        printf("\n");
    }

    shmem_barrier_all();
    shmem_free(source);
    shmem_free(dest);
}

/* Test alltoallsmem */
void test_alltoallsmem() {
    int mype = shmem_my_pe();
    int npes = shmem_n_pes();
    
    /* Allocate arrays */
    int *source = shmem_malloc(NELEMS * npes * sizeof(int));
    int *dest = shmem_malloc(NELEMS * npes * sizeof(int));

    /* Initialize arrays */
    for (int i = 0; i < npes * NELEMS; i++) {
        source[i] = mype;
        dest[i] = -1;
    }
    
    if (mype == 0) {
        printf("PE %d: source = ", mype);
        for (int i = 0; i < npes * NELEMS; i++) {
            printf("%d ", source[i]);
        }
        printf("\n");
    }

    shmem_barrier_all();
    
    /* Perform alltoalls */
    shmem_alltoallsmem(SHMEM_TEAM_WORLD, dest, source, sizeof(int), sizeof(int), NELEMS);

    /* Verify results */
    if (mype == 0) {
        printf("PE %d: Result = ", mype);
        for (int i = 0; i < npes * NELEMS; i++) {
            printf("%d ", dest[i]);
        }
        printf("\n");
    }

    shmem_barrier_all();
    shmem_free(source);
    shmem_free(dest);
}

int main(void) {
    shmem_init();
    int mype = shmem_my_pe();

    if (mype == 0) {
        printf(HLINE "\n");
        printf("    Running alltoalls64 test\n");
        printf(HLINE "\n");
    }
    test_alltoalls64();

    if (mype == 0) {
        printf(HLINE "\n");
        printf("    Running alltoalls_type test\n");
        printf(HLINE "\n");
    }
    test_alltoalls_type();
    
    if (mype == 0) {
        printf(HLINE "\n");
        printf("    Running alltoallsmem test\n");
        printf(HLINE "\n");
    }
    test_alltoallsmem();

    shmem_finalize();
    return 0;
}

