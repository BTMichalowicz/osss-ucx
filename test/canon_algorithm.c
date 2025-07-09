#include "../build/install/include/shmem.h"
#include "../build/install/include/shmem/defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Function to perform local matrix multiplication: C += A * B
void local_matmul(double *A, double *B, double *C, int n) {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      for (int k = 0; k < n; k++) {
        C[i * n + j] += A[i * n + k] * B[k * n + j];
      }
    }
  }
}

int main(int argc, char *argv[]) {
  // Initialize OpenSHMEM
  shmem_init();
  int mype = shmem_my_pe();
  int npes = shmem_n_pes();

  // Check if number of PEs is a perfect square
  int p = (int)sqrt((double)npes);
  if (p * p != npes) {
    if (mype == 0) {
      printf("Error: Number of PEs (%d) must be a perfect square.\n", npes);
    }
    shmem_finalize();
    return 1;
  }

  // Matrix size (assumed global N, divisible by p)
  int N = 512; // Example size, adjustable
  if (argc > 1) {
    N = atoi(argv[1]);
  }
  int n = N / p; // Local block size
  if (N % p != 0) {
    if (mype == 0) {
      printf(
          "Error: Matrix size N (%d) must be divisible by grid size p (%d).\n",
          N, p);
    }
    shmem_finalize();
    return 1;
  }

  // Compute 2D grid coordinates
  int row = mype / p;
  int col = mype % p;

  // Allocate symmetric memory for local blocks
  double *A = (double *)shmem_malloc(n * n * sizeof(double));
  double *B = (double *)shmem_malloc(n * n * sizeof(double));
  double *C = (double *)shmem_malloc(n * n * sizeof(double));

  // Allocate local (non-symmetric) buffers for receiving data
  double *recv_A = (double *)malloc(n * n * sizeof(double));
  double *recv_B = (double *)malloc(n * n * sizeof(double));

  if (!A || !B || !C || !recv_A || !recv_B) {
    if (mype == 0) {
      printf("Error: Memory allocation failed.\n");
    }
    shmem_finalize();
    return 1;
  }

  // Initialize local matrices
  for (int i = 0; i < n * n; i++) {
    A[i] = (double)(row * n + (i / n)) + (col * n + (i % n)) * 0.01;
    B[i] = (double)(row * n + (i / n)) * 0.01 + (col * n + (i % n));
    C[i] = 0.0;
    recv_A[i] = 0.0;
    recv_B[i] = 0.0;
  }

  shmem_barrier_all();

  // Initial skew of A: shift left by row steps
  int left_pe = row * p + (col - row + p) % p;
  if (left_pe < 0 || left_pe >= npes) {
    printf("PE %d: Error: Invalid left_pe %d\n", mype, left_pe);
    shmem_finalize();
    return 1;
  }
  shmem_put(A, A, n * n * sizeof(double), left_pe);
  shmem_quiet();
  shmem_barrier_all();

  // Initial skew of B: shift up by col steps
  int up_pe = ((row - col + p) % p) * p + col;
  if (up_pe < 0 || up_pe >= npes) {
    printf("PE %d: Error: Invalid up_pe %d\n", mype, up_pe);
    shmem_finalize();
    return 1;
  }
  shmem_put(B, B, n * n * sizeof(double), up_pe);
  shmem_quiet();
  shmem_barrier_all();

  // Cannon's algorithm: p iterations
  for (int k = 0; k < p; k++) {
    // Local matrix multiplication
    local_matmul(A, B, C, n);

    // Shift A left: send to (row, (col-1) mod p), receive from (row, (col+1)
    // mod p)
    int left_dest = row * p + (col - 1 + p) % p;
    int right_src = row * p + (col + 1) % p;
    if (left_dest < 0 || left_dest >= npes || right_src < 0 ||
        right_src >= npes) {
      printf("PE %d: Error: Invalid A shift PEs (left_dest=%d, right_src=%d)\n",
             mype, left_dest, right_src);
      shmem_finalize();
      return 1;
    }
    shmem_put(recv_A, A, n * n * sizeof(double), left_dest);
    shmem_quiet();
    shmem_barrier_all();
    for (int i = 0; i < n * n; i++) {
      A[i] = recv_A[i];
    }

    // Shift B up: send to ((row-1) mod p, col), receive from ((row+1) mod p,
    // col)
    int up_dest = ((row - 1 + p) % p) * p + col;
    int down_src = ((row + 1) % p) * p + col;
    if (up_dest < 0 || up_dest >= npes || down_src < 0 || down_src >= npes) {
      printf("PE %d: Error: Invalid B shift PEs (up_dest=%d, down_src=%d)\n",
             mype, up_dest, down_src);
      shmem_finalize();
      return 1;
    }
    shmem_put(recv_B, B, n * n * sizeof(double), up_dest);
    shmem_quiet();
    shmem_barrier_all();
    for (int i = 0; i < n * n; i++) {
      B[i] = recv_B[i];
    }
  }

  // Print a sample result from PE 0
  if (mype == 0) {
    printf("Sample result from PE 0 (top-left 4 elements of C block):\n");
    for (int i = 0; i < 2 && i < n; i++) {
      for (int j = 0; j < 2 && j < n; j++) {
        printf("%.2f ", C[i * n + j]);
      }
      printf("\n");
    }
  }

  // Cleanup
  shmem_free(A);
  shmem_free(B);
  shmem_free(C);
  free(recv_A);
  free(recv_B);

  shmem_finalize();
  return 0;
}