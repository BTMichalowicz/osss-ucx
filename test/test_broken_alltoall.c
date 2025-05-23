/* For license: see LICENSE file at top-level */

#include <shmem.h> 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // For int/uint types
#include <stddef.h> // For size_t, ptrdiff_t

/* Using float for tests, NELEMS must be > 0 */
#define NELEMS 1 

/* ========================================================================= */
/* Sanity Check Tests (Designed to trigger SHMEMU check failures)            */
/* These require the library to be built with --enable-debug                 */
/* Successful execution of these tests means the program ABORTS.             */
/* ========================================================================= */

/* Test SHMEMU_CHECK_TEAM_VALID */
void test_check_invalid_team() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();
  float *source = shmem_malloc(NELEMS * npes * sizeof(float));
  float *dest = shmem_malloc(NELEMS * npes * sizeof(float));

  if (me == 0)
    printf("\n[[[ Testing SHMEMU_CHECK_TEAM_VALID (Expected Abort) ]]]\n");
  fflush(stdout); // Ensure message prints before potential abort
  shmem_barrier_all();


  if (source && dest) {
    /* This call should trigger SHMEMU_CHECK_TEAM_VALID and abort */
    shmem_float_alltoall(SHMEM_TEAM_INVALID, dest, source, NELEMS);

    /* If we reach here, the check failed to abort */
    fprintf(stderr, "PE %d: SANITY CHECK FAIL - Invalid team test did not abort as expected.\n", me);
    shmem_free(source);
    shmem_free(dest);
  } else {
    fprintf(stderr, "PE %d: SANITY CHECK FAIL - Memory allocation failed for invalid team test.\n", me);
  }
  /* We should not reach here if the test works */
  fprintf(stderr, "PE %d: SANITY CHECK FAIL - Program continued after invalid team test.\n", me);
  shmem_barrier_all(); // Allow other PEs to print failure before exiting
  shmem_finalize(); // Attempt cleanup if abort failed
  exit(1);          // Exit if abort failed
}

/* Test SHMEMU_CHECK_SYMMETRIC on dest */
void test_check_non_symmetric_dest() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();
  float *source = shmem_malloc(NELEMS * npes * sizeof(float));
  float dest_stack[NELEMS * npes]; /* Allocate dest on stack (non-symmetric) */

  if (me == 0)
    printf("\n[[[ Testing SHMEMU_CHECK_SYMMETRIC on dest (Expected Abort) ]]]\n");
  fflush(stdout);
  shmem_barrier_all();

  if (source) {
    /* This call should trigger SHMEMU_CHECK_SYMMETRIC(dest, ...) and abort */
    shmem_float_alltoall(SHMEM_TEAM_WORLD, dest_stack, source, NELEMS);

    /* If we reach here, the check failed to abort */
    fprintf(stderr, "PE %d: SANITY CHECK FAIL - Non-symmetric dest test did not abort as expected.\n", me);
    shmem_free(source);
  } else {
    fprintf(stderr, "PE %d: SANITY CHECK FAIL - Memory allocation failed for non-symmetric dest test.\n", me);
  }
  /* We should not reach here if the test works */
  fprintf(stderr, "PE %d: SANITY CHECK FAIL - Program continued after non-symmetric dest test.\n", me);
  shmem_barrier_all(); 
  shmem_finalize(); 
  exit(1);          
}

/* Test SHMEMU_CHECK_SYMMETRIC on source */
void test_check_non_symmetric_source() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();
  float *dest = shmem_malloc(NELEMS * npes * sizeof(float));
  float source_stack[NELEMS * npes]; /* Allocate source on stack (non-symmetric) */

  if (me == 0)
    printf("\n[[[ Testing SHMEMU_CHECK_SYMMETRIC on source (Expected Abort) ]]]\n");
  fflush(stdout);
  shmem_barrier_all();

  if (dest) {
    /* Initialize stack source */
    for (int j = 0; j < npes * NELEMS; j++) {
      source_stack[j] = (float)(me + 1);
    }
    shmem_barrier_all(); // Ensure stack is initialized everywhere

    /* This call should trigger SHMEMU_CHECK_SYMMETRIC(source, ...) and abort */
    shmem_float_alltoall(SHMEM_TEAM_WORLD, dest, source_stack, NELEMS);

    /* If we reach here, the check failed to abort */
    fprintf(stderr, "PE %d: SANITY CHECK FAIL - Non-symmetric source test did not abort as expected.\n", me);
    shmem_free(dest);
  } else {
    fprintf(stderr, "PE %d: SANITY CHECK FAIL - Memory allocation failed for non-symmetric source test.\n", me);
  }
  /* We should not reach here if the test works */
  fprintf(stderr, "PE %d: SANITY CHECK FAIL - Program continued after non-symmetric source test.\n", me);
  shmem_barrier_all(); 
  shmem_finalize(); 
  exit(1);          
}

/* Test SHMEMU_CHECK_BUFFER_OVERLAP */
void test_check_overlapping_buffers() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();
  // Ensure NELEMS * npes > 1 for overlap to be possible if npes > 1
  size_t num_elements_total = (NELEMS * npes > 1) ? NELEMS * npes : 2; 
  size_t total_size_bytes = num_elements_total * sizeof(float);
  
  /* Allocate one large buffer */
  float *buffer = shmem_malloc(total_size_bytes * 2); // Allocate enough space

  if (me == 0)
    printf("\n[[[ Testing SHMEMU_CHECK_BUFFER_OVERLAP (Expected Abort) ]]]\n");
  fflush(stdout);
  shmem_barrier_all();

  if (buffer) {
    /* Make source and dest overlap within the larger buffer */
    float *source = buffer; 
    float *dest = buffer + (num_elements_total / 2); // Start dest partway into source space

    /* Initialize source part */
     for (int j = 0; j < num_elements_total; j++) {
      source[j] = (float)(me + 1);
    }
    shmem_barrier_all(); // Ensure buffer is initialized everywhere

    /* This call should trigger SHMEMU_CHECK_BUFFER_OVERLAP and abort */
    /* Use num_elements_total / npes for nelems per PE, minimum 1 */
    size_t nelems_per_pe = (num_elements_total / npes > 0) ? num_elements_total / npes : 1; 
    shmem_float_alltoall(SHMEM_TEAM_WORLD, dest, source, nelems_per_pe);

    /* If we reach here, the check failed to abort */
    fprintf(stderr, "PE %d: SANITY CHECK FAIL - Overlapping buffers test did not abort as expected.\n", me);
    shmem_free(buffer);
  } else {
     fprintf(stderr, "PE %d: SANITY CHECK FAIL - Memory allocation failed for overlapping buffers test.\n", me);
  }
  /* We should not reach here if the test works */
  fprintf(stderr, "PE %d: SANITY CHECK FAIL - Program continued after overlapping buffers test.\n", me);
  shmem_barrier_all(); 
  shmem_finalize(); 
  exit(1);          
}


/**
 * @brief Main function that runs the sanity check tests
 */
int main(int argc, char *argv[]) {
  shmem_init();
  int me = shmem_my_pe();
  int npes = shmem_n_pes();

  /* --- Run Sanity Check Tests (Expect Aborts if Debug Enabled) --- */
  if (me == 0) {
      printf("============================================================\n");
      printf(" Starting Sanity Check Tests (Requires --enable-debug)\n");
      printf(" NOTE: Each successful test below should ABORT the program.\n");
      printf("       (NELEMS=%d, NPES=%d)\n", NELEMS, npes);
      printf("============================================================\n");
  }
  shmem_barrier_all();

  // --- Test 1: Invalid Team ---
  // test_check_invalid_team(); 
  // If we reach here, the check failed. The function handles printing failure.

  // --- Test 2: Non-Symmetric Destination ---
  // test_check_non_symmetric_dest(); 
  // If we reach here, the check failed.

  // --- Test 3: Non-Symmetric Source ---
  // test_check_non_symmetric_source(); 
  // If we reach here, the check failed.

  // --- Test 4: Overlapping Buffers ---
  // Only run if npes > 1 or NELEMS > 1 to make overlap possible
  if (NELEMS * npes > 1) { 
      test_check_overlapping_buffers(); 
      // If we reach here, the check failed.
  } else if (me == 0) {
      printf("\n[[[ Skipping SHMEMU_CHECK_BUFFER_OVERLAP test (NELEMS * NPES <= 1) ]]]\n");
      // If overlap is skipped, we need a barrier before potentially hitting the final failure message
      shmem_barrier_all(); 
  }


  // --- If NO checks aborted ---
  // This section should only be reached if ALL sanity checks failed to abort
  if (me == 0) {
      printf("\n============================================================\n");
      fprintf(stderr, "MAJOR SANITY CHECK FAILURE: Program reached end of main.\n");
      fprintf(stderr, "NONE of the sanity checks aborted as expected.\n");
      fprintf(stderr, "Ensure library was built with --enable-debug.\n");
      printf("============================================================\n");
  }

  shmem_finalize();
  // Return non-zero indicates the sanity checks failed to abort
  return 1; 
}
