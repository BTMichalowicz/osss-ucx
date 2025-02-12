#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../build/build/include/shmem.h"

#define NELEMS 4
#define DST_STRIDE 2
#define SRC_STRIDE 3

/* Test alltoalls for a specific type */
#define TEST_ALLTOALLS_TYPE(_type, _typename)                                  \
  void test_alltoalls_##_typename() {                                          \
    int me = shmem_my_pe();                                                    \
    int npes = shmem_n_pes();                                                  \
                                                                               \
    /* Allocate and initialize source and destination arrays */                \
    _type *source = shmem_malloc(NELEMS * npes * SRC_STRIDE * sizeof(_type));  \
    _type *dest = shmem_malloc(NELEMS * npes * DST_STRIDE * sizeof(_type));    \
                                                                               \
    if (source == NULL || dest == NULL) {                                      \
      printf("PE %d: Memory allocation failed\n", me);                         \
      shmem_finalize();                                                        \
      exit(1);                                                                 \
    }                                                                          \
                                                                               \
    /* Initialize source array: PE i sends value (i+1) to each PE */           \
    for (int pe = 0; pe < npes; pe++) {                                        \
      for (int i = 0; i < NELEMS; i++) {                                       \
        source[pe * NELEMS * SRC_STRIDE + i * SRC_STRIDE] = me + 1;            \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* Initialize destination array */                                         \
    for (int i = 0; i < npes * NELEMS * DST_STRIDE; i++) {                     \
      dest[i] = -1;                                                            \
    }                                                                          \
                                                                               \
    /* Print initial source array */                                           \
    printf("PE %d: Initial source array: ", me);                               \
    for (int pe = 0; pe < npes; pe++) {                                        \
      printf("%d ", (int)source[pe * NELEMS * SRC_STRIDE]);                    \
    }                                                                          \
    printf("\n");                                                              \
                                                                               \
    /* Synchronize before alltoalls */                                         \
    shmem_barrier_all();                                                       \
                                                                               \
    /* Perform alltoalls */                                                    \
    int ret = shmem_##_typename##_alltoalls(SHMEM_TEAM_WORLD, dest, source,    \
                                            DST_STRIDE, SRC_STRIDE, NELEMS);   \
    if (ret != 0) {                                                            \
      printf("PE %d: shmem_%s_alltoalls failed with return code %d\n", me,     \
             #_typename, ret);                                                 \
      shmem_free(source);                                                      \
      shmem_free(dest);                                                        \
      shmem_finalize();                                                        \
      exit(1);                                                                 \
    }                                                                          \
                                                                               \
    /* Synchronize after alltoalls */                                          \
    shmem_barrier_all();                                                       \
                                                                               \
    /* Print resulting destination array */                                    \
    printf("PE %d: Resulting destination array: ", me);                        \
    for (int pe = 0; pe < npes; pe++) {                                        \
      printf("%d ", (int)dest[pe * NELEMS * DST_STRIDE]);                      \
    }                                                                          \
    printf("\n");                                                              \
                                                                               \
    /* Verify the results: dest[j] should be j + 1 */                          \
    int errors = 0;                                                            \
    for (int pe = 0; pe < npes; pe++) {                                        \
      for (int i = 0; i < NELEMS; i++) {                                       \
        _type expected = pe + 1;                                               \
        if (dest[pe * NELEMS * DST_STRIDE + i * DST_STRIDE] != expected) {     \
          printf("PE %d: Error at block %d elem %d, expected %d, got %d\n",    \
                 me, pe, i, (int)expected,                                     \
                 (int)dest[pe * NELEMS * DST_STRIDE + i * DST_STRIDE]);        \
          errors++;                                                            \
        }                                                                      \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (errors == 0) {                                                         \
      printf("PE %d: Alltoalls %s test passed\n", me, #_typename);             \
    } else {                                                                   \
      printf("PE %d: Alltoalls %s test failed with %d errors\n", me,           \
             #_typename, errors);                                              \
    }                                                                          \
                                                                               \
    /* Free allocated memory */                                                \
    shmem_free(source);                                                        \
    shmem_free(dest);                                                          \
  }

/* Test alltoalls32 */
void test_alltoalls32() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();

  /* Allocate and initialize source and destination arrays */
  uint32_t *source =
      shmem_malloc(NELEMS * npes * SRC_STRIDE * sizeof(uint32_t));
  uint32_t *dest = shmem_malloc(NELEMS * npes * DST_STRIDE * sizeof(uint32_t));
  long *pSync = shmem_malloc(SHMEM_ALLTOALLS_SYNC_SIZE * sizeof(long));

  if (source == NULL || dest == NULL || pSync == NULL) {
    printf("PE %d: Memory allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Initialize pSync */
  for (int i = 0; i < SHMEM_ALLTOALLS_SYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }

  /* Initialize source array: PE i sends value (i+1) to each PE */
  for (int pe = 0; pe < npes; pe++) {
    for (int i = 0; i < NELEMS; i++) {
      source[pe * NELEMS * SRC_STRIDE + i * SRC_STRIDE] = me + 1;
    }
  }

  /* Initialize destination array */
  for (int i = 0; i < npes * NELEMS * DST_STRIDE; i++) {
    dest[i] = 0;
  }

  /* Print initial source array */
  printf("PE %d: Initial source array: ", me);
  for (int pe = 0; pe < npes; pe++) {
    printf("%u ", source[pe * NELEMS * SRC_STRIDE]);
  }
  printf("\n");

  /* Synchronize before alltoalls */
  shmem_barrier_all();

  /* Perform alltoalls32 */
  shmem_alltoalls32(dest, source, DST_STRIDE, SRC_STRIDE, NELEMS, 0, 0, npes,
                    pSync);

  /* Synchronize after alltoalls */
  shmem_barrier_all();

  /* Print resulting destination array */
  printf("PE %d: Resulting destination array: ", me);
  for (int pe = 0; pe < npes; pe++) {
    printf("%u ", dest[pe * NELEMS * DST_STRIDE]);
  }
  printf("\n");

  /* Verify the results: dest[j] should be j+1 */
  int errors = 0;
  for (int pe = 0; pe < npes; pe++) {
    for (int i = 0; i < NELEMS; i++) {
      uint32_t expected = pe + 1;
      if (dest[pe * NELEMS * DST_STRIDE + i * DST_STRIDE] != expected) {
        printf("PE %d: Error at block %d elem %d, expected %u, got %u\n", me,
               pe, i, expected,
               dest[pe * NELEMS * DST_STRIDE + i * DST_STRIDE]);
        errors++;
      }
    }
  }

  if (errors == 0) {
    printf("PE %d: Alltoalls32 test passed\n", me);
  } else {
    printf("PE %d: Alltoalls32 test failed with %d errors\n", me, errors);
  }

  shmem_free(source);
  shmem_free(dest);
  shmem_free(pSync);
}

/* Test alltoalls64 */
void test_alltoalls64() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();

  /* Allocate and initialize source and destination arrays */
  uint64_t *source =
      shmem_malloc(NELEMS * npes * SRC_STRIDE * sizeof(uint64_t));
  uint64_t *dest = shmem_malloc(NELEMS * npes * DST_STRIDE * sizeof(uint64_t));
  long *pSync = shmem_malloc(SHMEM_ALLTOALLS_SYNC_SIZE * sizeof(long));

  if (source == NULL || dest == NULL || pSync == NULL) {
    printf("PE %d: Memory allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Initialize pSync */
  for (int i = 0; i < SHMEM_ALLTOALLS_SYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }

  /* Initialize source array: PE i sends value (i+1) to each PE */
  for (int pe = 0; pe < npes; pe++) {
    for (int i = 0; i < NELEMS; i++) {
      source[pe * NELEMS * SRC_STRIDE + i * SRC_STRIDE] = me + 1;
    }
  }

  /* Initialize destination array */
  for (int i = 0; i < npes * NELEMS * DST_STRIDE; i++) {
    dest[i] = 0;
  }

  /* Print initial source array */
  printf("PE %d: Initial source array: ", me);
  for (int pe = 0; pe < npes; pe++) {
    printf("%lu ", source[pe * NELEMS * SRC_STRIDE]);
  }
  printf("\n");

  /* Synchronize before alltoalls */
  shmem_barrier_all();

  /* Perform alltoalls64 */
  shmem_alltoalls64(dest, source, DST_STRIDE, SRC_STRIDE, NELEMS, 0, 0, npes,
                    pSync);

  /* Synchronize after alltoalls */
  shmem_barrier_all();

  /* Print resulting destination array */
  printf("PE %d: Resulting destination array: ", me);
  for (int pe = 0; pe < npes; pe++) {
    printf("%lu ", dest[pe * NELEMS * DST_STRIDE]);
  }
  printf("\n");

  /* Verify the results: dest[j] should be j+1 */
  int errors = 0;
  for (int pe = 0; pe < npes; pe++) {
    for (int i = 0; i < NELEMS; i++) {
      uint64_t expected = pe + 1;
      if (dest[pe * NELEMS * DST_STRIDE + i * DST_STRIDE] != expected) {
        printf("PE %d: Error at block %d elem %d, expected %lu, got %lu\n", me,
               pe, i, expected,
               dest[pe * NELEMS * DST_STRIDE + i * DST_STRIDE]);
        errors++;
      }
    }
  }

  if (errors == 0) {
    printf("PE %d: Alltoalls64 test passed\n", me);
  } else {
    printf("PE %d: Alltoalls64 test failed with %d errors\n", me, errors);
  }

  shmem_free(source);
  shmem_free(dest);
  shmem_free(pSync);
}

/* Test alltoallsmem */
void test_alltoallsmem() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();

  /* Allocate and initialize source and destination arrays */
  char *source = shmem_malloc(NELEMS * npes * SRC_STRIDE);
  char *dest = shmem_malloc(NELEMS * npes * DST_STRIDE);

  if (source == NULL || dest == NULL) {
    printf("PE %d: Memory allocation failed\n", me);
    shmem_finalize();
    exit(1);
  }

  /* Initialize source array: PE i sends value ('A'+i) to each PE */
  for (int pe = 0; pe < npes; pe++) {
    for (int i = 0; i < NELEMS; i++) {
      source[pe * NELEMS * SRC_STRIDE + i * SRC_STRIDE] = 'A' + me;
    }
  }

  /* Initialize destination array */
  for (int i = 0; i < npes * NELEMS * DST_STRIDE; i++) {
    dest[i] = 'X';
  }

  /* Print initial source array */
  printf("PE %d: Initial source array: ", me);
  for (int pe = 0; pe < npes; pe++) {
    printf("%c ", source[pe * NELEMS * SRC_STRIDE]);
  }
  printf("\n");

  /* Synchronize before alltoalls */
  shmem_barrier_all();

  /* Perform alltoallsmem */
  int ret = shmem_alltoallsmem(SHMEM_TEAM_WORLD, dest, source, DST_STRIDE,
                               SRC_STRIDE, NELEMS);
  if (ret != 0) {
    printf("PE %d: shmem_alltoallsmem failed with return code %d\n", me, ret);
    shmem_free(source);
    shmem_free(dest);
    shmem_finalize();
    exit(1);
  }

  /* Synchronize after alltoalls */
  shmem_barrier_all();

  /* Print resulting destination array */
  printf("PE %d: Resulting destination array: ", me);
  for (int pe = 0; pe < npes; pe++) {
    printf("%c ", dest[pe * NELEMS * DST_STRIDE]);
  }
  printf("\n");

  /* Verify the results: dest[j] should be 'A'+j */
  int errors = 0;
  for (int pe = 0; pe < npes; pe++) {
    for (int i = 0; i < NELEMS; i++) {
      char expected = 'A' + pe;
      if (dest[pe * NELEMS * DST_STRIDE + i * DST_STRIDE] != expected) {
        printf("PE %d: Error at block %d elem %d, expected %c, got %c\n", me,
               pe, i, expected,
               dest[pe * NELEMS * DST_STRIDE + i * DST_STRIDE]);
        errors++;
      }
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

/* Test alltoalls for all standard types */
TEST_ALLTOALLS_TYPE(float, float)
TEST_ALLTOALLS_TYPE(double, double)
TEST_ALLTOALLS_TYPE(long double, longdouble)
TEST_ALLTOALLS_TYPE(char, char)
TEST_ALLTOALLS_TYPE(signed char, schar)
TEST_ALLTOALLS_TYPE(short, short)
TEST_ALLTOALLS_TYPE(int, int)
TEST_ALLTOALLS_TYPE(long, long)
TEST_ALLTOALLS_TYPE(long long, longlong)
TEST_ALLTOALLS_TYPE(unsigned char, uchar)
TEST_ALLTOALLS_TYPE(unsigned short, ushort)
TEST_ALLTOALLS_TYPE(unsigned int, uint)
TEST_ALLTOALLS_TYPE(unsigned long, ulong)
TEST_ALLTOALLS_TYPE(unsigned long long, ulonglong)
TEST_ALLTOALLS_TYPE(int8_t, int8)
TEST_ALLTOALLS_TYPE(int16_t, int16)
TEST_ALLTOALLS_TYPE(int32_t, int32)
TEST_ALLTOALLS_TYPE(int64_t, int64)
TEST_ALLTOALLS_TYPE(uint8_t, uint8)
TEST_ALLTOALLS_TYPE(uint16_t, uint16)
TEST_ALLTOALLS_TYPE(uint32_t, uint32)
TEST_ALLTOALLS_TYPE(uint64_t, uint64)
TEST_ALLTOALLS_TYPE(size_t, size)
TEST_ALLTOALLS_TYPE(ptrdiff_t, ptrdiff)

int main(void) {
  shmem_init();
  int me = shmem_my_pe();

  if (me == 0)
    printf("Starting alltoalls tests\n");

  /* Run tests for each type */
  // test_alltoalls_float();
  // test_alltoalls_double();
  // test_alltoalls_longdouble();
  // test_alltoalls_char();
  // test_alltoalls_schar();
  // test_alltoalls_short();
  // test_alltoalls_int();
  // test_alltoalls_long();
  // test_alltoalls_longlong();
  // test_alltoalls_uchar();
  // test_alltoalls_ushort();
  // test_alltoalls_uint();
  // test_alltoalls_ulong();
  // test_alltoalls_ulonglong();
  // test_alltoalls_int8();
  // test_alltoalls_int16();
  // test_alltoalls_int32();
  // test_alltoalls_int64();
  // test_alltoalls_uint8();
  // test_alltoalls_uint16();
  // test_alltoalls_uint32();
  // test_alltoalls_uint64();
  // test_alltoalls_size();
  // test_alltoalls_ptrdiff();

  shmem_barrier_all();

  // /* Run sized and memory-based tests */
  // if (me == 0)
  //   printf("\nTesting sized and memory-based alltoalls...\n");

  // test_alltoalls32();
  // shmem_barrier_all();

  // test_alltoalls64();
  // shmem_barrier_all();

  test_alltoallsmem();
  shmem_barrier_all();

  if (me == 0)
    printf("\nAll alltoalls tests completed\n");

  shmem_finalize();
  return 0;
}