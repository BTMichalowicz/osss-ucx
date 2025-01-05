/* For license: see LICENSE file at top-level */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 10

void test_broadcast_simple() {
    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    int test_passed = 1;

    /* Allocate arrays */
    int *source = (int *)shmem_malloc(NELEMS * sizeof(int));
    int *dest = (int *)shmem_malloc(NELEMS * sizeof(int));
    int *all_passed = (int *)shmem_malloc(npes * sizeof(int));

    if (source == NULL || dest == NULL || all_passed == NULL) {
        printf("PE %d: Memory allocation failed\n", me);
        if (source) shmem_free(source);
        if (dest) shmem_free(dest);
        if (all_passed) shmem_free(all_passed);
        shmem_global_exit(1);
        return;
    }

    /* Initialize arrays */
    if (me == 0) {
        for (int i = 0; i < NELEMS; i++) {
            source[i] = i + 1;
        }
    }
    for (int i = 0; i < NELEMS; i++) {
        dest[i] = -1;
    }
    for (int i = 0; i < npes; i++) {
        all_passed[i] = 1;
    }

    shmem_barrier_all();

    /* Print initial values */
    printf("PE %d: Before broadcast\n", me);

    /* Perform broadcast */
    int ret = shmem_int_broadcast(SHMEM_TEAM_WORLD, dest, source, NELEMS, 0);

    printf("PE %d: After broadcast, ret = %d\n", me, ret);

    /* Verify results */
    for (int i = 0; i < NELEMS; i++) {
        if (dest[i] != i + 1) {
            printf("PE %d: Verification failed at index %d: expected %d, got %d\n",
                   me, i, i + 1, dest[i]);
            test_passed = 0;
        }
    }

    /* Share results */
    if (me != 0) {
        shmem_int_p(&all_passed[me], test_passed, 0);
    } else {
        all_passed[0] = test_passed;
    }

    shmem_barrier_all();

    /* Report results */
    if (me == 0) {
        int all_tests_passed = 1;
        for (int i = 0; i < npes; i++) {
            if (!all_passed[i]) {
                all_tests_passed = 0;
                printf("PE %d failed the test\n", i);
            }
        }
        if (all_tests_passed) {
            printf("Test PASSED\n");
        } else {
            printf("Test FAILED\n");
        }
    }

    shmem_barrier_all();
    shmem_free(source);
    shmem_free(dest);
    shmem_free(all_passed);
}

void test_broadcastmem_simple() {
    int npes = shmem_n_pes();
    int me = shmem_my_pe();
    int test_passed = 1;

    /* Allocate arrays */
    char *source = (char *)shmem_malloc(NELEMS);
    char *dest = (char *)shmem_malloc(NELEMS);
    int *all_passed = (int *)shmem_malloc(sizeof(int));

    if (source == NULL || dest == NULL || all_passed == NULL) {
        printf("PE %d: Memory allocation failed\n", me);
        if (source) shmem_free(source);
        if (dest) shmem_free(dest);
        if (all_passed) shmem_free(all_passed);
        shmem_global_exit(1);
        return;
    }

    /* Initialize arrays */
    if (me == 0) {
        for (int i = 0; i < NELEMS; i++) {
            source[i] = 'A' + i;
        }
    }
    for (int i = 0; i < NELEMS; i++) {
        dest[i] = 'X';
    }
    *all_passed = 1;

    shmem_barrier_all();

    /* Perform broadcast */
    int ret = shmem_broadcastmem(SHMEM_TEAM_WORLD, dest, source, NELEMS, 0);

    if (ret != 0) {
        printf("PE %d: Broadcastmem failed with ret = %d\n", me, ret);
        test_passed = 0;
    }

    /* Verify results */
    for (int i = 0; i < NELEMS; i++) {
        if (dest[i] != 'A' + i) {
            printf("PE %d: Verification failed at index %d: expected %c, got %c\n",
                   me, i, 'A' + i, dest[i]);
            test_passed = 0;
        }
    }

    /* Share results */
    if (me != 0) {
        shmem_int_put(all_passed, &test_passed, 1, 0);
    }

    shmem_barrier_all();

    /* Report results */
    if (me == 0) {
        int all_tests_passed = test_passed;
        for (int i = 1; i < npes; i++) {
            if (!all_passed[i]) {
                all_tests_passed = 0;
                break;
            }
        }
        if (all_tests_passed) {
            printf("Test PASSED\n");
        } else {
            printf("Test FAILED\n");
        }
    }

    shmem_barrier_all();
    shmem_free(source);
    shmem_free(dest);
    shmem_free(all_passed);
}

int main(void) {
    shmem_init();
    
    test_broadcast_simple();
    test_broadcastmem_simple();
    
    shmem_finalize();
    return 0;
}
