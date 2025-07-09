#include "../build/install/include/shmem.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <complex.h>
#include <inttypes.h>

#define N 4

// Macro for arithmetic types (sum/prod)
#define TEST_TOALL_ARITH(TYPE, TYPENAME, INIT)                                 \
  do {                                                                         \
    TYPE *src = shmem_malloc(N * sizeof(TYPE));                                \
    TYPE *dst = shmem_malloc(N * sizeof(TYPE));                                \
    TYPE *pWrk = shmem_malloc(SHMEM_REDUCE_MIN_WRKDATA_SIZE * sizeof(TYPE));   \
    long *pSync = shmem_malloc(SHMEM_REDUCE_SYNC_SIZE * sizeof(long));         \
    for (int i = 0; i < SHMEM_REDUCE_SYNC_SIZE; i++)                           \
      pSync[i] = SHMEM_SYNC_VALUE;                                             \
    for (int i = 0; i < N; i++)                                                \
      src[i] = (TYPE)(INIT + shmem_my_pe() + i);                               \
    for (int i = 0; i < N; i++)                                                \
      dst[i] = 0;                                                              \
    shmem_barrier_all();                                                       \
    shmem_sum_to_all(dst, src, N, 0, 0, shmem_n_pes(), pWrk, pSync);           \
    shmem_barrier_all();                                                       \
    for (int i = 0; i < N; i++)                                                \
      dst[i] = 0;                                                              \
    for (int i = 0; i < SHMEM_REDUCE_SYNC_SIZE; i++)                           \
      pSync[i] = SHMEM_SYNC_VALUE;                                             \
    shmem_prod_to_all(dst, src, N, 0, 0, shmem_n_pes(), pWrk, pSync);          \
    shmem_barrier_all();                                                       \
    shmem_free(src);                                                           \
    shmem_free(dst);                                                           \
    shmem_free(pWrk);                                                          \
    shmem_free(pSync);                                                         \
  } while (0)

// Macro for bitwise types (and/or/xor)
#define TEST_TOALL_BITWISE(TYPE, TYPENAME, INIT)                               \
  do {                                                                         \
    TYPE *src = shmem_malloc(N * sizeof(TYPE));                                \
    TYPE *dst = shmem_malloc(N * sizeof(TYPE));                                \
    TYPE *pWrk = shmem_malloc(SHMEM_REDUCE_MIN_WRKDATA_SIZE * sizeof(TYPE));   \
    long *pSync = shmem_malloc(SHMEM_REDUCE_SYNC_SIZE * sizeof(long));         \
    for (int i = 0; i < SHMEM_REDUCE_SYNC_SIZE; i++)                           \
      pSync[i] = SHMEM_SYNC_VALUE;                                             \
    for (int i = 0; i < N; i++)                                                \
      src[i] = (TYPE)(INIT + shmem_my_pe() + i);                               \
    for (int i = 0; i < N; i++)                                                \
      dst[i] = 0;                                                              \
    shmem_barrier_all();                                                       \
    shmem_and_to_all(dst, src, N, 0, 0, shmem_n_pes(), pWrk, pSync);           \
    shmem_barrier_all();                                                       \
    for (int i = 0; i < N; i++)                                                \
      dst[i] = 0;                                                              \
    for (int i = 0; i < SHMEM_REDUCE_SYNC_SIZE; i++)                           \
      pSync[i] = SHMEM_SYNC_VALUE;                                             \
    shmem_or_to_all(dst, src, N, 0, 0, shmem_n_pes(), pWrk, pSync);            \
    shmem_barrier_all();                                                       \
    for (int i = 0; i < N; i++)                                                \
      dst[i] = 0;                                                              \
    for (int i = 0; i < SHMEM_REDUCE_SYNC_SIZE; i++)                           \
      pSync[i] = SHMEM_SYNC_VALUE;                                             \
    shmem_xor_to_all(dst, src, N, 0, 0, shmem_n_pes(), pWrk, pSync);           \
    shmem_barrier_all();                                                       \
    shmem_free(src);                                                           \
    shmem_free(dst);                                                           \
    shmem_free(pWrk);                                                          \
    shmem_free(pSync);                                                         \
  } while (0)

// Macro for min/max types
#define TEST_TOALL_MINMAX(TYPE, TYPENAME, INIT)                                \
  do {                                                                         \
    TYPE *src = shmem_malloc(N * sizeof(TYPE));                                \
    TYPE *dst = shmem_malloc(N * sizeof(TYPE));                                \
    TYPE *pWrk = shmem_malloc(SHMEM_REDUCE_MIN_WRKDATA_SIZE * sizeof(TYPE));   \
    long *pSync = shmem_malloc(SHMEM_REDUCE_SYNC_SIZE * sizeof(long));         \
    for (int i = 0; i < SHMEM_REDUCE_SYNC_SIZE; i++)                           \
      pSync[i] = SHMEM_SYNC_VALUE;                                             \
    for (int i = 0; i < N; i++)                                                \
      src[i] = (TYPE)(INIT + shmem_my_pe() + i);                               \
    for (int i = 0; i < N; i++)                                                \
      dst[i] = 0;                                                              \
    shmem_barrier_all();                                                       \
    shmem_max_to_all(dst, src, N, 0, 0, shmem_n_pes(), pWrk, pSync);           \
    shmem_barrier_all();                                                       \
    for (int i = 0; i < N; i++)                                                \
      dst[i] = 0;                                                              \
    for (int i = 0; i < SHMEM_REDUCE_SYNC_SIZE; i++)                           \
      pSync[i] = SHMEM_SYNC_VALUE;                                             \
    shmem_min_to_all(dst, src, N, 0, 0, shmem_n_pes(), pWrk, pSync);           \
    shmem_barrier_all();                                                       \
    shmem_free(src);                                                           \
    shmem_free(dst);                                                           \
    shmem_free(pWrk);                                                          \
    shmem_free(pSync);                                                         \
  } while (0)

int main(void) {
  shmem_init();

  if (shmem_my_pe() == 0)
    printf("==== to_all reductions smoke test ====\n");

  // Arithmetic (sum/prod)
  TEST_TOALL_ARITH(short, short, 1);
  TEST_TOALL_ARITH(int, int, 1);
  TEST_TOALL_ARITH(long, long, 1);
  TEST_TOALL_ARITH(long long, longlong, 1);
  TEST_TOALL_ARITH(float, float, 1.0f);
  TEST_TOALL_ARITH(double, double, 1.0);
  TEST_TOALL_ARITH(long double, longdouble, 1.0);
  // TEST_TOALL_ARITH(double _Complex, complexd, 1.0 + 0.0 * I);
  // TEST_TOALL_ARITH(float _Complex, complexf, 1.0f + 0.0f * I);

  // Bitwise (and/or/xor)
  TEST_TOALL_BITWISE(short, short, 1);
  TEST_TOALL_BITWISE(int, int, 1);
  TEST_TOALL_BITWISE(long, long, 1);
  TEST_TOALL_BITWISE(long long, longlong, 1);

  // Min/max
  TEST_TOALL_MINMAX(short, short, 1);
  TEST_TOALL_MINMAX(int, int, 1);
  TEST_TOALL_MINMAX(long, long, 1);
  TEST_TOALL_MINMAX(long long, longlong, 1);
  TEST_TOALL_MINMAX(float, float, 1.0f);
  TEST_TOALL_MINMAX(double, double, 1.0);
  TEST_TOALL_MINMAX(long double, longdouble, 1.0);

  shmem_finalize();
  if (shmem_my_pe() == 0)
    printf("==== to_all reductions test PASSED ====\n");
  return 0;
}
