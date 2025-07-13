/* For license: see LICENSE file at top-level */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

/* Send one element per PE */
#define NELEMS 1

/* Macro to test alltoall for a specific type */
#define TEST_ALLTOALL_TYPE(_type, _typename)                                   \
  void test_alltoall_##_typename() {                                           \
    int npes = shmem_n_pes();                                                  \
    int me = shmem_my_pe();                                                    \
                                                                               \
    /* Allocate and initialize source and destination arrays */                \
    _type *source = shmem_malloc(NELEMS * npes * sizeof(_type));               \
    _type *dest = shmem_malloc(NELEMS * npes * sizeof(_type));                 \
                                                                               \
    if (source == NULL || dest == NULL) {                                      \
      printf("PE %d: Memory allocation failed\n", me);                         \
      shmem_finalize();                                                        \
      exit(1);                                                                 \
    }                                                                          \
                                                                               \
    /* Initialize source array: each PE sets source[j] = me + 1 */             \
    for (int j = 0; j < npes; j++) {                                           \
      source[j * NELEMS] = me + 1;                                             \
    }                                                                          \
                                                                               \
    /* Initialize destination array */                                         \
    for (int j = 0; j < npes * NELEMS; j++) {                                  \
      dest[j] = -1;                                                            \
    }                                                                          \
                                                                               \
    /* Print initial source array */                                           \
    printf("PE %d: Initial source array: ", me);                               \
    for (int j = 0; j < npes; j++) {                                           \
      printf("%d ", (int)source[j * NELEMS]);                                  \
    }                                                                          \
    printf("\n");                                                              \
                                                                               \
    /* Synchronize before all-to-all */                                        \
    shmem_barrier_all();                                                       \
                                                                               \
    /* Perform all-to-all communication */                                     \
    int ret =                                                                  \
        shmem_##_typename##_alltoall(SHMEM_TEAM_WORLD, dest, source, NELEMS);  \
    if (ret != 0) {                                                            \
      printf("PE %d: shmem_%s_alltoall failed with return code %d\n", me,      \
             #_typename, ret);                                                 \
      shmem_free(source);                                                      \
      shmem_free(dest);                                                        \
      shmem_finalize();                                                        \
      exit(1);                                                                 \
    }                                                                          \
                                                                               \
    /* Synchronize after all-to-all */                                         \
    shmem_barrier_all();                                                       \
                                                                               \
    /* Print resulting destination array */                                    \
    printf("PE %d: Resulting destination array: ", me);                        \
    for (int j = 0; j < npes; j++) {                                           \
      printf("%d ", (int)dest[j * NELEMS]);                                    \
    }                                                                          \
    printf("\n");                                                              \
                                                                               \
    /* Verify the results: dest[j] should be j + 1 */                          \
    int errors = 0;                                                            \
    for (int j = 0; j < npes; j++) {                                           \
      _type expected = j + 1;                                                  \
      if (dest[j * NELEMS] != expected) {                                      \
        printf("PE %d: Error at index %d, expected %d, got %d\n", me,          \
               j *NELEMS, (int)expected, (int)dest[j * NELEMS]);               \
        errors++;                                                              \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (errors == 0) {                                                         \
      printf("PE %d: Alltoall %s test passed\n", me, #_typename);              \
    } else {                                                                   \
      printf("PE %d: Alltoall %s test failed with %d errors\n", me,            \
             #_typename, errors);                                              \
    }                                                                          \
                                                                               \
    /* Free allocated memory */                                                \
    shmem_free(source);                                                        \
    shmem_free(dest);                                                          \
  }

/* Test alltoall for all standard types */
TEST_ALLTOALL_TYPE(float, float)
TEST_ALLTOALL_TYPE(double, double)
TEST_ALLTOALL_TYPE(long double, longdouble)
TEST_ALLTOALL_TYPE(char, char)
TEST_ALLTOALL_TYPE(signed char, schar)
TEST_ALLTOALL_TYPE(short, short)
TEST_ALLTOALL_TYPE(int, int)
TEST_ALLTOALL_TYPE(long, long)
TEST_ALLTOALL_TYPE(long long, longlong)
TEST_ALLTOALL_TYPE(unsigned char, uchar)
TEST_ALLTOALL_TYPE(unsigned short, ushort)
TEST_ALLTOALL_TYPE(unsigned int, uint)
TEST_ALLTOALL_TYPE(unsigned long, ulong)
TEST_ALLTOALL_TYPE(unsigned long long, ulonglong)
TEST_ALLTOALL_TYPE(int8_t, int8)
TEST_ALLTOALL_TYPE(int16_t, int16)
TEST_ALLTOALL_TYPE(int32_t, int32)
TEST_ALLTOALL_TYPE(int64_t, int64)
TEST_ALLTOALL_TYPE(uint8_t, uint8)
TEST_ALLTOALL_TYPE(uint16_t, uint16)
TEST_ALLTOALL_TYPE(uint32_t, uint32)
TEST_ALLTOALL_TYPE(uint64_t, uint64)
TEST_ALLTOALL_TYPE(size_t, size)
TEST_ALLTOALL_TYPE(ptrdiff_t, ptrdiff)

/* Test alltoall32 */
void test_alltoall32() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();

  /* Allocate and initialize source and destination arrays */
  uint32_t *source = shmem_malloc(NELEMS * npes * sizeof(uint32_t));
  uint32_t *dest = shmem_malloc(NELEMS * npes * sizeof(uint32_t));
  long *pSync = shmem_malloc(SHMEM_ALLTOALL_SYNC_SIZE * sizeof(long));

  if (source == NULL || dest == NULL || pSync == NULL) {
    printf("PE %d: Memory allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Initialize pSync */
  for (int i = 0; i < SHMEM_ALLTOALL_SYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }

  /* Initialize source array: PE i sends value (i+1) to each PE */
  for (int i = 0; i < npes * NELEMS; i++) {
    source[i] = me + 1;
  }

  /* Initialize destination array */
  for (int i = 0; i < npes * NELEMS; i++) {
    dest[i] = 0;
  }

  /* Print initial source array */
  printf("PE %d: Initial source array: ", me);
  for (int i = 0; i < npes; i++) {
    printf("%u ", source[i * NELEMS]);
  }
  printf("\n");

  /* Synchronize before alltoall */
  shmem_barrier_all();

  /* Perform alltoall32 */
  shmem_alltoall32(dest, source, NELEMS, 0, 0, npes, pSync);

  /* Synchronize after alltoall */
  shmem_barrier_all();

  /* Print resulting destination array */
  printf("PE %d: Resulting destination array: ", me);
  for (int i = 0; i < npes; i++) {
    printf("%u ", dest[i * NELEMS]);
  }
  printf("\n");

  /* Verify the results: dest[j] should be j+1 */
  int errors = 0;
  for (int i = 0; i < npes; i++) {
    uint32_t expected = i + 1;
    if (dest[i * NELEMS] != expected) {
      printf("PE %d: Error at index %d, expected %u, got %u\n", me, i * NELEMS,
             expected, dest[i * NELEMS]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: Alltoall32 test passed\n", me);
  } else {
    printf("PE %d: Alltoall32 test failed with %d errors\n", me, errors);
  }

  shmem_free(source);
  shmem_free(dest);
  shmem_free(pSync);
}

/* Test alltoall64 */
void test_alltoall64() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();

  /* Allocate and initialize source and destination arrays */
  uint64_t *source = shmem_malloc(NELEMS * npes * sizeof(uint64_t));
  uint64_t *dest = shmem_malloc(NELEMS * npes * sizeof(uint64_t));
  long *pSync = shmem_malloc(SHMEM_ALLTOALL_SYNC_SIZE * sizeof(long));

  if (source == NULL || dest == NULL || pSync == NULL) {
    printf("PE %d: Memory allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Initialize pSync */
  for (int i = 0; i < SHMEM_ALLTOALL_SYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }

  /* Initialize source array: PE i sends value (i+1) to each PE */
  for (int i = 0; i < npes * NELEMS; i++) {
    source[i] = me + 1;
  }

  /* Initialize destination array */
  for (int i = 0; i < npes * NELEMS; i++) {
    dest[i] = 0;
  }

  /* Print initial source array */
  printf("PE %d: Initial source array: ", me);
  for (int i = 0; i < npes; i++) {
    printf("%lu ", source[i * NELEMS]);
  }
  printf("\n");

  /* Synchronize before alltoall */
  shmem_barrier_all();

  /* Perform alltoall64 */
  shmem_alltoall64(dest, source, NELEMS, 0, 0, npes, pSync);

  /* Synchronize after alltoall */
  shmem_barrier_all();

  /* Print resulting destination array */
  printf("PE %d: Resulting destination array: ", me);
  for (int i = 0; i < npes; i++) {
    printf("%lu ", dest[i * NELEMS]);
  }
  printf("\n");

  /* Verify the results: dest[j] should be j+1 */
  int errors = 0;
  for (int i = 0; i < npes; i++) {
    uint64_t expected = i + 1;
    if (dest[i * NELEMS] != expected) {
      printf("PE %d: Error at index %d, expected %lu, got %lu\n", me,
             i * NELEMS, expected, dest[i * NELEMS]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: Alltoall64 test passed\n", me);
  } else {
    printf("PE %d: Alltoall64 test failed with %d errors\n", me, errors);
  }

  shmem_free(source);
  shmem_free(dest);
  shmem_free(pSync);
}

/* Test alltoallmem */
void test_alltoallmem() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();

  /* Allocate and initialize source and destination arrays */
  char *source = shmem_malloc(NELEMS * npes * sizeof(char));
  char *dest = shmem_malloc(NELEMS * npes * sizeof(char));

  if (source == NULL || dest == NULL) {
    printf("PE %d: Memory allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Initialize source array: PE i sends value ('A'+i) to each PE */
  for (int i = 0; i < npes * NELEMS; i++) {
    source[i] = 'A' + me;
  }

  /* Initialize destination array */
  for (int i = 0; i < npes * NELEMS; i++) {
    dest[i] = 'X';
  }

  /* Print initial source array */
  printf("PE %d: Initial source array: ", me);
  for (int i = 0; i < npes; i++) {
    printf("%c ", source[i * NELEMS]);
  }
  printf("\n");

  /* Synchronize before alltoall */
  shmem_barrier_all();

  /* Perform alltoallmem */
  int ret = shmem_alltoallmem(SHMEM_TEAM_WORLD, dest, source, NELEMS);
  if (ret != 0) {
    printf("PE %d: shmem_alltoallmem failed with return code %d\n", me, ret);
    shmem_free(source);
    shmem_free(dest);
    shmem_finalize();
    exit(1);
  }

  /* Synchronize after alltoall */
  shmem_barrier_all();

  /* Print resulting destination array */
  printf("PE %d: Resulting destination array: ", me);
  for (int i = 0; i < npes; i++) {
    printf("%c ", dest[i * NELEMS]);
  }
  printf("\n");

  /* Verify the results: dest[j] should be 'A'+j */
  int errors = 0;
  for (int i = 0; i < npes; i++) {
    char expected = 'A' + i;
    if (dest[i * NELEMS] != expected) {
      printf("PE %d: Error at index %d, expected %c, got %c\n", me, i * NELEMS,
             expected, dest[i * NELEMS]);
      errors++;
    }
  }

  if (errors == 0) {
    printf("PE %d: Alltoallmem test passed\n", me);
  } else {
    printf("PE %d: Alltoallmem test failed with %d errors\n", me, errors);
  }

  shmem_free(source);
  shmem_free(dest);
}

/**
 * @brief Main function that runs all alltoall tests
 *
 * Initializes SHMEM, runs the alltoall tests for each type with barriers
 * between tests, and finalizes SHMEM.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return 0 on success
 */
int main(int argc, char *argv[]) {
  shmem_init();
  int me = shmem_my_pe();

  /* Run tests for each type */
  // if (me == 0)
  // printf("Testing alltoall for all types...\n");

  test_alltoall_float();
  test_alltoall_double();
  test_alltoall_longdouble();
  test_alltoall_char();
  test_alltoall_schar();
  test_alltoall_short();
  test_alltoall_int();
  test_alltoall_long();
  test_alltoall_longlong();
  test_alltoall_uchar();
  test_alltoall_ushort();
  test_alltoall_uint();
  test_alltoall_ulong();
  test_alltoall_ulonglong();
  test_alltoall_int8();
  test_alltoall_int16();
  test_alltoall_int32();
  test_alltoall_int64();
  test_alltoall_uint8();
  test_alltoall_uint16();
  test_alltoall_uint32();
  test_alltoall_uint64();
  test_alltoall_size();
  test_alltoall_ptrdiff();

  /* Run tests for sized and memory-based alltoall */
  if (me == 0)
    printf("\nTesting sized and memory-based alltoall...\n");

  test_alltoall32();
  shmem_barrier_all();

  test_alltoall64();
  shmem_barrier_all();

  test_alltoallmem();
  shmem_barrier_all();

  shmem_finalize();
  return 0;
}
