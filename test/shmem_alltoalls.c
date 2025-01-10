/* For license: see LICENSE file at top-level */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 1

/**
 * @brief Tests the deprecated shmem_alltoalls64 collective operation
 */
void test_alltoalls64() {
    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    
    /* Allocate pSync from symmetric heap */
    long *pSync = shmem_malloc(SHMEM_ALLTOALLS_SYNC_SIZE * sizeof(long));
    if (pSync == NULL) {
        printf("PE %d: pSync allocation failed\n", me);
        shmem_finalize();
        exit(1);
    }

    /* Initialize pSync */
    for (int i = 0; i < SHMEM_ALLTOALLS_SYNC_SIZE; i++) {
        pSync[i] = SHMEM_SYNC_VALUE;
    }

    /* Allocate and initialize source and destination arrays */
    size_t src_size = NELEMS * npes * sizeof(long);
    size_t dst_size = NELEMS * npes * sizeof(long);
    
    long *source = shmem_malloc(src_size);
    long *dest = shmem_malloc(dst_size);

    if (source == NULL || dest == NULL) {
        printf("PE %d: Memory allocation failed\n", me);
        shmem_free(pSync);
        shmem_finalize();
        exit(1);
    }

    /* Initialize source array */
    for (int i = 0; i < npes * NELEMS; i++) {
        source[i] = me + 100;
        dest[i] = -1;
    }

    /* Ensure all PEs have initialized their arrays */
    shmem_barrier_all();

    /* Print initial values */
    printf("PE %d: Initial source values: ", me);
    for (int i = 0; i < npes * NELEMS; i++) {
        printf("%ld ", source[i]);
    }
    printf("\n");

    shmem_barrier_all();

    /* Debug info */
    if (me == 0) {
        printf("Calling shmem_alltoalls64 with:\n");
        printf("  dest stride: 1\n");
        printf("  source stride: 1\n");
        printf("  nelems: %d\n", NELEMS);
        printf("  PE_start: 0\n");
        printf("  logPE_stride: 0\n");
        printf("  PE_size: %d\n", npes);
    }

    /* Perform alltoalls */
    shmem_alltoalls64(dest,      /* dest */
                      source,    /* source */
                      1,        /* dst stride */
                      1,        /* sst stride */
                      NELEMS,   /* nelems */
                      0,        /* PE_start */
                      0,        /* logPE_stride */
                      npes,     /* PE_size */
                      pSync);

    /* Ensure all PEs have completed the operation */
    shmem_barrier_all();

    /* Print final values */
    printf("PE %d: Final dest values: ", me);
    for (int i = 0; i < npes * NELEMS; i++) {
        printf("%ld ", dest[i]);
    }
    printf("\n");

    /* Verify results */
    int errors = 0;
    for (int i = 0; i < npes; i++) {
        long expected = i + 100;
        if (dest[i * NELEMS] != expected) {
            printf("PE %d: Error at index %d: expected %ld, got %ld\n",
                   me, i, expected, dest[i * NELEMS]);
            errors++;
        }
    }

    if (errors == 0) {
        printf("PE %d: Alltoalls64 test passed\n", me);
    } else {
        printf("PE %d: Alltoalls64 test failed with %d errors\n", me, errors);
    }

    /* Ensure all PEs are done before cleanup */
    shmem_barrier_all();

    shmem_free(source);
    shmem_free(dest);
    shmem_free(pSync);
}

/**
 * @brief Tests the typed shmem_int_alltoalls collective operation
 */
void test_alltoallstype() {
    int npes = shmem_n_pes();
    int me = shmem_my_pe();

    /* Allocate and initialize source and destination arrays */
    int *source = shmem_malloc(NELEMS * npes * sizeof(int));
    int *dest = shmem_malloc(NELEMS * npes * sizeof(int));

    if (source == NULL || dest == NULL) {
        printf("PE %d: Memory allocation failed\n", me);
        shmem_finalize();
        exit(1);
    }

    /* Initialize arrays */
    for (int i = 0; i < npes * NELEMS; i++) {
        source[i] = me + 1;
        dest[i] = -1;
    }

    /* Print initial values */
    printf("PE %d: Initial source values: ", me);
    for (int i = 0; i < npes * NELEMS; i++) {
        printf("%d ", source[i]);
    }
    printf("\n");

    shmem_barrier_all();

    /* Perform alltoalls */
    int ret = shmem_int_alltoalls(SHMEM_TEAM_WORLD, dest, source, 
                                 1,      // dst stride
                                 1,      // sst stride
                                 NELEMS);

    if (ret != 0) {
        printf("PE %d: shmem_int_alltoalls failed with return code %d\n", me, ret);
        shmem_finalize();
        exit(1);
    }

    shmem_barrier_all();

    /* Print final values */
    printf("PE %d: Final dest values: ", me);
    for (int i = 0; i < npes * NELEMS; i++) {
        printf("%d ", dest[i]);
    }
    printf("\n");

    /* Verify results */
    int errors = 0;
    for (int i = 0; i < npes; i++) {
        int expected = i + 1;
        if (dest[i * NELEMS] != expected) {
            printf("PE %d: Error at index %d: expected %d, got %d\n",
                   me, i, expected, dest[i * NELEMS]);
            errors++;
        }
    }

    if (errors == 0) {
        printf("PE %d: Alltoallstype test passed\n", me);
    } else {
        printf("PE %d: Alltoallstype test failed with %d errors\n", me, errors);
    }

    shmem_free(source);
    shmem_free(dest);
}

/**
 * @brief Tests the generic shmem_alltoallsmem collective operation
 */
void test_alltoallsmem() {
    int npes = shmem_n_pes();
    int me = shmem_my_pe();

    /* Allocate and initialize source and destination arrays */
    char *source = shmem_malloc(NELEMS * npes);
    char *dest = shmem_malloc(NELEMS * npes);

    if (source == NULL || dest == NULL) {
        printf("PE %d: Memory allocation failed\n", me);
        shmem_finalize();
        exit(1);
    }

    /* Initialize arrays */
    for (int i = 0; i < npes * NELEMS; i++) {
        source[i] = me + 'A';
        dest[i] = '?';
    }

    /* Print initial values */
    printf("PE %d: Initial source values: ", me);
    for (int i = 0; i < npes * NELEMS; i++) {
        printf("%c ", source[i]);
    }
    printf("\n");

    shmem_barrier_all();

    /* Perform alltoalls */
    int ret = shmem_alltoallsmem(SHMEM_TEAM_WORLD, dest, source,
                                1,      // dst stride
                                1,      // sst stride
                                NELEMS);

    if (ret != 0) {
        printf("PE %d: shmem_alltoallsmem failed with return code %d\n", me, ret);
        shmem_finalize();
        exit(1);
    }

    shmem_barrier_all();

    /* Print final values */
    printf("PE %d: Final dest values: ", me);
    for (int i = 0; i < npes * NELEMS; i++) {
        printf("%c ", dest[i]);
    }
    printf("\n");

    /* Verify results */
    int errors = 0;
    for (int i = 0; i < npes; i++) {
        char expected = i + 'A';
        if (dest[i * NELEMS] != expected) {
            printf("PE %d: Error at index %d: expected %c, got %c\n",
                   me, i, expected, dest[i * NELEMS]);
            errors++;
        }
    }

    if (errors == 0) {
        printf("PE %d: Alltoallsmem test passed\n", me);
    } else {
        printf("PE %d: Alltoallsmem test failed with %d errors\n", me, errors);
    }

    shmem_free(source);
    shmem_free(dest);
}

int main(void) {
    shmem_init();
    int me = shmem_my_pe();

    if (me == 0) {
        printf("----------------------------------------\n");
        printf("    Running alltoalls64 test\n");
        printf("----------------------------------------\n");
    }
    shmem_barrier_all();
    test_alltoalls64();
    shmem_barrier_all();

    if (me == 0) {
        printf("----------------------------------------\n");
        printf("    Running alltoallsmem test\n");
        printf("----------------------------------------\n");
    }
    shmem_barrier_all();
    test_alltoallsmem();
    shmem_barrier_all();

    if (me == 0) {
        printf("----------------------------------------\n");
        printf("    Running alltoallstype test\n");
        printf("----------------------------------------\n");
    }
    shmem_barrier_all();
    test_alltoallstype();
    shmem_barrier_all();

    shmem_finalize();
    return 0;
}