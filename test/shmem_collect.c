/* For license: see LICENSE file at top-level */
/* shmem_collect.c */

#include "../build/build/include/shmem.h"
#include <stdio.h>
#include <stdlib.h>

#define NELEMS 1 // Collect one element per PE

void test_collect_simple() {
  int npes = shmem_n_pes();
  int me = shmem_my_pe();
  int test_passed = 1;

  // Correct Allocation with proper alignment and symmetric heap
  int *source = (int *)shmem_malloc(sizeof(int));
  int *dest = (int *)shmem_malloc(npes * sizeof(int));
  int *all_passed = (int *)shmem_malloc(npes * sizeof(int)); // Correct Allocation

  if (source == NULL || dest == NULL || all_passed == NULL) {
    printf("PE %d: Memory allocation failed\n", me);
    if (source) shmem_free(source);
    if (dest) shmem_free(dest);
    if (all_passed) shmem_free(all_passed);
    shmem_global_exit(1);
    return;
  }

  // Initialize arrays
  *source = me;
  for (int i = 0; i < npes; i++) {
    dest[i] = -1;
    all_passed[i] = 1; // Initialize all_passed to 1
  }

  shmem_barrier_all();

  printf("PE %d: Before collect, source = %d\n", me, *source);

  // Perform collect
  shmem_team_t world_team = SHMEM_TEAM_WORLD;
  int ret = shmem_int_collect(world_team, dest, source, NELEMS);

  shmem_barrier_all();

  // Verify results
  if (ret == 0) {
    printf("PE %d: After collect: ", me);
    for (int i = 0; i < npes; i++) {
      printf("%d ", dest[i]);
      if (dest[i] != i) {
        test_passed = 0;
        printf("\nPE %d: Verification failed at position %d: expected %d, got %d\n", 
               me, i, i, dest[i]);
      }
    }
    printf("\n");
  } else {
    printf("PE %d: Collect failed with ret = %d\n", me, ret);
    test_passed = 0;
  }

  // Share pass/fail status with PE 0
  if (me != 0) {
    printf("PE %d: Setting test_passed = %d\n", me, test_passed);
    shmem_int_put(&all_passed[me], &test_passed, 1, 0);  // Write to index [me]
  }
  
  shmem_barrier_all();

  // PE 0 checks if any PE failed
  if (me == 0) {
    printf("PE 0: Checking results. Local test_passed = %d\n", test_passed);
    for (int i = 1; i < npes; i++) {
      printf("PE 0: all_passed[%d] = %d\n", i, all_passed[i]);
    }
    if (!test_passed) {
      printf("Test FAILED: PE 0 detected errors\n");
    } else {
      // Check status from other PEs
      int all_tests_passed = 1;
      for (int i = 1; i < npes && all_tests_passed; i++) {
        if (!all_passed[i]) {
          all_tests_passed = 0;
        }
      }
      if (all_tests_passed) {
        printf("Test PASSED: All PEs successfully collected data in correct order\n");
      } else {
        printf("Test FAILED: Some PEs detected errors\n");
      }
    }
  }

  shmem_barrier_all();
  
  shmem_free(source);
  shmem_free(dest);
  shmem_free(all_passed);
}

int main(int argc, char *argv[]) {
  shmem_init();

  test_collect_simple();

  shmem_finalize();
  return 0;
}









// /* For license: see LICENSE file at top-level */
// /* shmem_collect.c */

// #include "../build/build/include/shmem.h"
// #include <stdio.h>
// #include <stdlib.h>

// #define NELEMS 1 // Collect one element per PE

// void test_collect_simple() {
//   int npes = shmem_n_pes();
//   int me = shmem_my_pe();
//   int test_passed = 1;

//   // Allocate with proper alignment and symmetric heap
//   int *source = (int *)shmem_malloc(sizeof(int));
//   int *dest = (int *)shmem_malloc(npes * sizeof(int));
//   int *all_passed = (int *)shmem_malloc(sizeof(int)); // For collecting pass/fail status

//   if (source == NULL || dest == NULL || all_passed == NULL) {
//     printf("PE %d: Memory allocation failed\n", me);
//     if (source) shmem_free(source);
//     if (dest) shmem_free(dest);
//     if (all_passed) shmem_free(all_passed);
//     shmem_global_exit(1);
//     return;
//   }

//   // Initialize arrays
//   *source = me;
//   for (int i = 0; i < npes; i++) {
//     dest[i] = -1;
//   }
//   *all_passed = 1;

//   shmem_barrier_all();

//   printf("PE %d: Before collect, source = %d\n", me, *source);

//   // Perform collect
//   shmem_team_t world_team = SHMEM_TEAM_WORLD;
//   int ret = shmem_int_collect(world_team, dest, source, 1);
  
//   shmem_barrier_all();

//   // Verify results
//   if (ret == 0) {
//     printf("PE %d: After collect: ", me);
//     for (int i = 0; i < npes; i++) {
//       printf("%d ", dest[i]);
//       if (dest[i] != i) {
//         test_passed = 0;
//         printf("\nPE %d: Verification failed at position %d: expected %d, got %d\n", 
//                me, i, i, dest[i]);
//       }
//     }
//     printf("\n");
//   } else {
//     printf("PE %d: Collect failed with ret = %d\n", me, ret);
//     test_passed = 0;
//   }

//   // Share pass/fail status with PE 0
//   if (me != 0) {
//     printf("PE %d: Setting test_passed = %d\n", me, test_passed);
//     shmem_int_put(&all_passed[me], &test_passed, 1, 0);  // Write to index [me]
//   }
  
//   shmem_barrier_all();

//   // PE 0 checks if any PE failed
//   if (me == 0) {
//     printf("PE 0: Checking results. Local test_passed = %d\n", test_passed);
//     for (int i = 1; i < npes; i++) {
//       printf("PE 0: all_passed[%d] = %d\n", i, all_passed[i]);
//     }
//     if (!test_passed) {
//       printf("Test FAILED: PE 0 detected errors\n");
//     } else {
//       // Check status from other PEs
//       int all_tests_passed = 1;
//       for (int i = 1; i < npes && all_tests_passed; i++) {
//         if (!all_passed[i]) {
//           all_tests_passed = 0;
//         }
//       }
//       if (all_tests_passed) {
//         printf("Test PASSED: All PEs successfully collected data in correct order\n");
//       } else {
//         printf("Test FAILED: Some PEs detected errors\n");
//       }
//     }
//   }

//   shmem_barrier_all();
  
//   shmem_free(source);
//   shmem_free(dest);
//   shmem_free(all_passed);
// }

// int main(int argc, char *argv[]) {
//   shmem_init();

//   test_collect_simple();

//   shmem_finalize();
//   return 0;
// }
