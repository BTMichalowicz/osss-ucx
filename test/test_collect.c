/* For license: see LICENSE file at top-level */
/* shmem_collect.c */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 1 /* Collect one element per PE */

#define HLINE "----------------------------------------\n"

/* Test sized collect (32/64 bit) */
void test_collect64() {
    int mype = shmem_my_pe();
    int npes = shmem_n_pes();
    
    /* Allocate arrays */
    long *source = shmem_malloc(sizeof(long));
    long *dest = shmem_malloc(sizeof(long) * npes);
    long *pSync = shmem_malloc(SHMEM_COLLECT_SYNC_SIZE * sizeof(long));

    /* Initialize pSync */
    for (int i = 0; i < SHMEM_COLLECT_SYNC_SIZE; i++) {
        pSync[i] = SHMEM_SYNC_VALUE;
    }

    /* Initialize source with PE number */
    source[0] = mype;
    
    if (mype == 0) {
        printf("PE %d: source = %ld\n", mype, source[0]);
    }

    shmem_barrier_all();
    
    /* Perform collect */
    shmem_collect64(dest, source, NELEMS, 0, 0, npes, pSync);

    /* Verify results */
    if (mype == 0) {
        printf("PE %d: Result = ", mype);
        for (int i = 0; i < npes; i++) {
            printf("%ld ", dest[i]);
        }
        printf("\n");
    }

    shmem_barrier_all();
    shmem_free(source);
    shmem_free(dest);
    shmem_free(pSync);
}

/* Test typed collect */
void test_collect_type() {
    int mype = shmem_my_pe();
    int npes = shmem_n_pes();
    shmem_team_t team = SHMEM_TEAM_WORLD;

    /* Allocate arrays */
    int *source = shmem_malloc(sizeof(int));
    int *dest = shmem_malloc(sizeof(int) * npes);

    /* Initialize source with PE number */
    source[0] = mype;
    
    if (mype == 0) {
        printf("PE %d: source = %d\n", mype, source[0]);
    }

    shmem_barrier_all();
    
    /* Perform collect */
    shmem_int_collect(team, dest, source, NELEMS);

    /* Verify results */
    if (mype == 0) {
        printf("PE %d: Result = ", mype);
        for (int i = 0; i < npes; i++) {
            printf("%d ", dest[i]);
        }
        printf("\n");
    }

    shmem_barrier_all();
    shmem_free(source);
    shmem_free(dest);
}

/* Test collectmem */
void test_collectmem() {
    int mype = shmem_my_pe();
    int npes = shmem_n_pes();
    shmem_team_t team = SHMEM_TEAM_WORLD;

    /* Allocate arrays */
    char *source = shmem_malloc(NELEMS);
    char *dest = shmem_malloc(NELEMS * npes);

    /* Initialize source with PE character (A, B, C, ...) */
    source[0] = 'A' + mype;
    
    if (mype == 0) {
        printf("PE %d: source = %c\n", mype, source[0]);
    }

    shmem_barrier_all();
    
    /* Perform collect */
    shmem_collectmem(team, dest, source, NELEMS);

    /* Verify results */
    if (mype == 0) {
        printf("PE %d: Result = ", mype);
        for (int i = 0; i < npes; i++) {
            printf("%c ", dest[i]);
        }
        printf("\n");
    }

    shmem_barrier_all();
    shmem_free(source);
    shmem_free(dest);
}

int main(int argc, char *argv[]) {
    shmem_init();

    int mype = shmem_my_pe();
    int npes = shmem_n_pes();

    if (mype == 0) {
        printf(HLINE);
        printf("  Running collect64 test\n");
        printf(HLINE);
    }
    shmem_barrier_all();
    test_collect64();
    shmem_barrier_all();

    if (mype == 0) {
        printf(HLINE);
        printf("  Running collect_type test\n");
        printf(HLINE);
    }
    shmem_barrier_all();
    test_collect_type();
    shmem_barrier_all();
    
    if (mype == 0) {
        printf(HLINE);
        printf("  Running collectmem test\n");
        printf(HLINE);
    }
    shmem_barrier_all();
    test_collectmem();
    shmem_barrier_all();

    shmem_finalize();
    return 0;
}

