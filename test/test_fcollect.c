/* For license: see LICENSE file at top-level */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 1 /* Fcollect one element per PE */
#define HLINE "----------------------------------------\n"

/* Test sized fcollect (32/64 bit) */
void test_fcollect64() {
  int mype = shmem_my_pe();
  int npes = shmem_n_pes();

  /* Allocate arrays */
  uint64_t *source = shmem_malloc(sizeof(uint64_t));
  uint64_t *dest = shmem_malloc(sizeof(uint64_t) * npes);
  long *pSync = shmem_malloc(SHMEM_COLLECT_SYNC_SIZE * sizeof(long));

  /* Initialize pSync */
  for (int i = 0; i < SHMEM_COLLECT_SYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }

  /* Initialize source with PE number */
  source[0] = mype;

  if (mype == 0) {
    printf("PE %d: source = %lu\n", mype, source[0]);
  }

  shmem_barrier_all();

  /* Perform fcollect */
  shmem_fcollect64(dest, source, NELEMS, 0, 0, npes, pSync);

  /* Verify results */
  if (mype == 0) {
    printf("PE %d: Result = ", mype);
    for (int i = 0; i < npes; i++) {
      printf("%lu ", dest[i]);
    }
    printf("\n");
  }

  shmem_barrier_all();
  shmem_free(source);
  shmem_free(dest);
  shmem_free(pSync);
}

/* Test typed fcollect */
void test_fcollect_type() {
  int mype = shmem_my_pe();
  int npes = shmem_n_pes();
  shmem_team_t team = SHMEM_TEAM_WORLD;

  /* Allocate arrays */
  int *source = shmem_malloc(sizeof(int));
  int *dest = shmem_malloc(sizeof(int) * npes);
  long *pSync = shmem_malloc(SHMEM_COLLECT_SYNC_SIZE * sizeof(long));

  /* Initialize pSync */
  for (int i = 0; i < SHMEM_COLLECT_SYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }

  /* Initialize source with PE number */
  source[0] = mype;

  if (mype == 0) {
    printf("PE %d: source = %d\n", mype, source[0]);
  }

  shmem_barrier_all();

  /* Perform fcollect */
  shmem_fcollect32(dest, source, NELEMS, 0, 0, npes, pSync);

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
  shmem_free(pSync);
}

/* Test fcollectmem */
void test_fcollectmem() {
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

  /* Perform fcollect */
  shmem_fcollectmem(team, dest, source, NELEMS);

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

  if (mype == 0) {
    printf(HLINE);
    printf("    Running fcollect64 test\n");
    printf(HLINE);
  }
  test_fcollect64();

  if (mype == 0) {
    printf(HLINE);
    printf("    Running fcollect_type test\n");
    printf(HLINE);
  }
  test_fcollect_type();

  if (mype == 0) {
    printf(HLINE);
    printf("    Running fcollectmem test\n");
    printf(HLINE);
  }
  test_fcollectmem();

  shmem_finalize();
  return 0;
}
