/* For license: see LICENSE file at top-level */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <stdint.h>
#include <limits.h>

#define NELEMS 3 // Test multiple elements

/**
 * @brief Tests bitwise AND reduction
 */
void test_and_reduce() {
  int npes = shmem_n_pes();
  int me = shmem_my_pe();

  uint32_t *source = shmem_malloc(NELEMS * sizeof(uint32_t));
  uint32_t *target = shmem_malloc(NELEMS * sizeof(uint32_t));
  if (source == NULL || target == NULL) {
    printf("PE %d: Array allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Initialize with all bits set except one bit based on PE number */
  for (int i = 0; i < NELEMS; i++) {
    source[i] = UINT32_MAX ^ (1U << me);
    target[i] = 0;
  }

  printf("PE %d: AND Initial values: 0x%08x\n", me, source[0]);

  shmem_barrier_all();
  int ret = shmem_uint32_and_reduce(SHMEM_TEAM_WORLD, target, source, NELEMS);
  if (ret != 0) {
    printf("PE %d: shmem_uint32_and_reduce failed with return code %d\n", me,
           ret);
    shmem_finalize();
    exit(1);
  }
  shmem_barrier_all();

  /* Calculate expected: all bits set except those cleared by any PE */
  uint32_t expected = UINT32_MAX;
  for (int i = 0; i < npes; i++) {
    expected &= (UINT32_MAX ^ (1U << i));
  }

  int errors = 0;
  for (int i = 0; i < NELEMS; i++) {
    if (target[i] != expected) {
      printf("PE %d: AND Error at index %d, expected 0x%08x, got 0x%08x\n", me,
             i, expected, target[i]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: AND test passed, result = 0x%08x\n", me, target[0]);
  }

  shmem_free(source);
  shmem_free(target);
}

/**
 * @brief Tests bitwise OR reduction
 */
void test_or_reduce() {
  int npes = shmem_n_pes();
  int me = shmem_my_pe();

  uint32_t *source = shmem_malloc(NELEMS * sizeof(uint32_t));
  uint32_t *target = shmem_malloc(NELEMS * sizeof(uint32_t));
  if (source == NULL || target == NULL) {
    printf("PE %d: Array allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Each PE sets one bit */
  for (int i = 0; i < NELEMS; i++) {
    source[i] = 1U << me;
    target[i] = 0;
  }

  printf("PE %d: OR Initial values: 0x%08x\n", me, source[0]);

  shmem_barrier_all();
  int ret = shmem_uint32_or_reduce(SHMEM_TEAM_WORLD, target, source, NELEMS);
  if (ret != 0) {
    printf("PE %d: shmem_uint32_or_reduce failed with return code %d\n", me,
           ret);
    shmem_finalize();
    exit(1);
  }
  shmem_barrier_all();

  /* Calculate expected: bits set by any PE */
  uint32_t expected = 0;
  for (int i = 0; i < npes; i++) {
    expected |= (1U << i);
  }

  int errors = 0;
  for (int i = 0; i < NELEMS; i++) {
    if (target[i] != expected) {
      printf("PE %d: OR Error at index %d, expected 0x%08x, got 0x%08x\n", me,
             i, expected, target[i]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: OR test passed, result = 0x%08x\n", me, target[0]);
  }

  shmem_free(source);
  shmem_free(target);
}

/**
 * @brief Tests bitwise XOR reduction
 */
void test_xor_reduce() {
  int npes = shmem_n_pes();
  int me = shmem_my_pe();

  uint32_t *source = shmem_malloc(NELEMS * sizeof(uint32_t));
  uint32_t *target = shmem_malloc(NELEMS * sizeof(uint32_t));
  if (source == NULL || target == NULL) {
    printf("PE %d: Array allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Each PE contributes its PE number */
  for (int i = 0; i < NELEMS; i++) {
    source[i] = me;
    target[i] = 0;
  }

  printf("PE %d: XOR Initial values: 0x%08x\n", me, source[0]);

  shmem_barrier_all();
  int ret = shmem_uint32_xor_reduce(SHMEM_TEAM_WORLD, target, source, NELEMS);
  if (ret != 0) {
    printf("PE %d: shmem_uint32_xor_reduce failed with return code %d\n", me,
           ret);
    shmem_finalize();
    exit(1);
  }
  shmem_barrier_all();

  /* Calculate expected: XOR of all PE numbers */
  uint32_t expected = 0;
  for (int i = 0; i < npes; i++) {
    expected ^= i;
  }

  int errors = 0;
  for (int i = 0; i < NELEMS; i++) {
    if (target[i] != expected) {
      printf("PE %d: XOR Error at index %d, expected 0x%08x, got 0x%08x\n", me,
             i, expected, target[i]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: XOR test passed, result = 0x%08x\n", me, target[0]);
  }

  shmem_free(source);
  shmem_free(target);
}

/**
 * @brief Tests MAX reduction
 */
void test_max_reduce() {
  int npes = shmem_n_pes();
  int me = shmem_my_pe();

  int *source = shmem_malloc(NELEMS * sizeof(int));
  int *target = shmem_malloc(NELEMS * sizeof(int));
  if (source == NULL || target == NULL) {
    printf("PE %d: Array allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Each PE contributes its PE number */
  for (int i = 0; i < NELEMS; i++) {
    source[i] = me;
    target[i] = 0;
  }

  printf("PE %d: MAX Initial values: %d\n", me, source[0]);

  shmem_barrier_all();
  int ret = shmem_int_max_reduce(SHMEM_TEAM_WORLD, target, source, NELEMS);
  if (ret != 0) {
    printf("PE %d: shmem_int_max_reduce failed with return code %d\n", me, ret);
    shmem_finalize();
    exit(1);
  }
  shmem_barrier_all();

  /* Expected max is npes-1 */
  int expected = npes - 1;

  int errors = 0;
  for (int i = 0; i < NELEMS; i++) {
    if (target[i] != expected) {
      printf("PE %d: MAX Error at index %d, expected %d, got %d\n", me, i,
             expected, target[i]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: MAX test passed, result = %d\n", me, target[0]);
  }

  shmem_free(source);
  shmem_free(target);
}

/**
 * @brief Tests MIN reduction
 */
void test_min_reduce() {
  int npes = shmem_n_pes();
  int me = shmem_my_pe();

  int *source = shmem_malloc(NELEMS * sizeof(int));
  int *target = shmem_malloc(NELEMS * sizeof(int));
  if (source == NULL || target == NULL) {
    printf("PE %d: Array allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Each PE contributes its PE number */
  for (int i = 0; i < NELEMS; i++) {
    source[i] = me + 100; // Offset to make it more interesting
    target[i] = INT_MAX;
  }

  printf("PE %d: MIN Initial values: %d\n", me, source[0]);

  shmem_barrier_all();
  int ret = shmem_int_min_reduce(SHMEM_TEAM_WORLD, target, source, NELEMS);
  if (ret != 0) {
    printf("PE %d: shmem_int_min_reduce failed with return code %d\n", me, ret);
    shmem_finalize();
    exit(1);
  }
  shmem_barrier_all();

  /* Expected min is 100 (PE 0 + 100) */
  int expected = 100;

  int errors = 0;
  for (int i = 0; i < NELEMS; i++) {
    if (target[i] != expected) {
      printf("PE %d: MIN Error at index %d, expected %d, got %d\n", me, i,
             expected, target[i]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: MIN test passed, result = %d\n", me, target[0]);
  }

  shmem_free(source);
  shmem_free(target);
}

/**
 * @brief Tests SUM reduction
 */
void test_sum_reduce() {
  int npes = shmem_n_pes();
  int me = shmem_my_pe();

  int *source = shmem_malloc(NELEMS * sizeof(int));
  int *target = shmem_malloc(NELEMS * sizeof(int));
  if (source == NULL || target == NULL) {
    printf("PE %d: Array allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Each PE contributes its PE number + 1 */
  for (int i = 0; i < NELEMS; i++) {
    source[i] = me + 1;
    target[i] = 0;
  }

  printf("PE %d: SUM Initial values: %d\n", me, source[0]);

  shmem_barrier_all();
  int ret = shmem_int_sum_reduce(SHMEM_TEAM_WORLD, target, source, NELEMS);
  if (ret != 0) {
    printf("PE %d: shmem_int_sum_reduce failed with return code %d\n", me, ret);
    shmem_finalize();
    exit(1);
  }
  shmem_barrier_all();

  /* Expected sum is (n * (n + 1)) / 2 where n = npes */
  int expected = (npes * (npes + 1)) / 2;

  int errors = 0;
  for (int i = 0; i < NELEMS; i++) {
    if (target[i] != expected) {
      printf("PE %d: SUM Error at index %d, expected %d, got %d\n", me, i,
             expected, target[i]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: SUM test passed, result = %d\n", me, target[0]);
  }

  shmem_free(source);
  shmem_free(target);
}

/**
 * @brief Tests PROD reduction
 */
void test_prod_reduce() {
  int npes = shmem_n_pes();
  int me = shmem_my_pe();

  int *source = shmem_malloc(NELEMS * sizeof(int));
  int *target = shmem_malloc(NELEMS * sizeof(int));
  if (source == NULL || target == NULL) {
    printf("PE %d: Array allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Each PE contributes its PE number + 1 */
  for (int i = 0; i < NELEMS; i++) {
    source[i] = me + 1;
    target[i] = 0;
  }

  printf("PE %d: PROD Initial values: %d\n", me, source[0]);

  shmem_barrier_all();
  int ret = shmem_int_prod_reduce(SHMEM_TEAM_WORLD, target, source, NELEMS);
  if (ret != 0) {
    printf("PE %d: shmem_int_prod_reduce failed with return code %d\n", me,
           ret);
    shmem_finalize();
    exit(1);
  }
  shmem_barrier_all();

  /* Calculate expected product: factorial of npes */
  int expected = 1;
  for (int i = 1; i <= npes; i++) {
    expected *= i;
  }

  int errors = 0;
  for (int i = 0; i < NELEMS; i++) {
    if (target[i] != expected) {
      printf("PE %d: PROD Error at index %d, expected %d, got %d\n", me, i,
             expected, target[i]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: PROD test passed, result = %d\n", me, target[0]);
  }

  shmem_free(source);
  shmem_free(target);
}

int main(int argc, char *argv[]) {
  shmem_init();
  int me = shmem_my_pe();

  if (me == 0) {
    printf("----------------------------------------\n");
    printf("    Running AND reduction test\n");
    printf("----------------------------------------\n");
  }
  shmem_barrier_all();
  test_and_reduce();
  shmem_barrier_all();

  if (me == 0) {
    printf("----------------------------------------\n");
    printf("    Running OR reduction test\n");
    printf("----------------------------------------\n");
  }
  shmem_barrier_all();
  test_or_reduce();
  shmem_barrier_all();

  if (me == 0) {
    printf("----------------------------------------\n");
    printf("    Running XOR reduction test\n");
    printf("----------------------------------------\n");
  }
  shmem_barrier_all();
  test_xor_reduce();
  shmem_barrier_all();

  if (me == 0) {
    printf("----------------------------------------\n");
    printf("    Running MAX reduction test\n");
    printf("----------------------------------------\n");
  }
  shmem_barrier_all();
  test_max_reduce();
  shmem_barrier_all();

  if (me == 0) {
    printf("----------------------------------------\n");
    printf("    Running MIN reduction test\n");
    printf("----------------------------------------\n");
  }
  shmem_barrier_all();
  test_min_reduce();
  shmem_barrier_all();

  if (me == 0) {
    printf("----------------------------------------\n");
    printf("    Running SUM reduction test\n");
    printf("----------------------------------------\n");
  }
  shmem_barrier_all();
  test_sum_reduce();
  shmem_barrier_all();

  if (me == 0) {
    printf("----------------------------------------\n");
    printf("    Running PROD reduction test\n");
    printf("----------------------------------------\n");
  }
  shmem_barrier_all();
  test_prod_reduce();
  shmem_barrier_all();

  shmem_finalize();
  return 0;
}
