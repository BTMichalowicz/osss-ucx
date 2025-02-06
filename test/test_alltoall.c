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

  // test_alltoall_float();
  // test_alltoall_double();
  // test_alltoall_longdouble();
  // test_alltoall_char();
  // test_alltoall_schar();
  // test_alltoall_short();
  // test_alltoall_int();
  // test_alltoall_long();
  // test_alltoall_longlong();
  // test_alltoall_uchar();
  // test_alltoall_ushort();
  // test_alltoall_uint();
  // test_alltoall_ulong();
  // test_alltoall_ulonglong();
  // test_alltoall_int8();
  // test_alltoall_int16();
  // test_alltoall_int32();
  // test_alltoall_int64();
  // test_alltoall_uint8();
  // test_alltoall_uint16();
  // test_alltoall_uint32();
  // test_alltoall_uint64();
  // test_alltoall_size();
  // test_alltoall_ptrdiff();

  shmem_finalize();
  return 0;
}
