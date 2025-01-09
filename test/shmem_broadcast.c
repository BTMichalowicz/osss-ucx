#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../build/build/include/shmem.h"

#define ARRAY_SIZE 10

// Helper function to verify broadcast results
static int verify_broadcast(const void *dest, const void *source, size_t nelems,
                            size_t type_size) {
  int errors = 0;
  for (int i = 0; i < nelems; i++) {
    if (memcmp((char *)dest + i * type_size, (char *)source + i * type_size,
               type_size) != 0) {
      printf("PE %d: dest[%d]=%d != source[%d]=%d\n", shmem_my_pe(), i,
             *((int *)dest + i), i, *((int *)source + i));
      errors++;
    }
  }
  return errors;
}

// // Test type-specific broadcast (e.g. shmem_int_broadcast)
// static void test_typed_broadcast() {
//   int *source = shmem_malloc(ARRAY_SIZE * sizeof(int));
//   int *dest = shmem_malloc(ARRAY_SIZE * sizeof(int));

//   if (!source || !dest) {
//     printf("Failed to allocate symmetric memory\n");
//     return;
//   }

//   int me = shmem_my_pe();
//   int root = 0;

//   // Initialize arrays to known values
//   for (int i = 0; i < ARRAY_SIZE; i++) {
//     source[i] = (me == root) ? i + 1 : -1;
//     dest[i] = -1;
//   }

//   if (me == 0)
//     printf("Starting typed broadcast test\n");
//   shmem_barrier_all();

//   // Test int broadcast with team
//   shmem_int_broadcast(SHMEM_TEAM_WORLD, dest, source, ARRAY_SIZE, root);

//   shmem_barrier_all();
//   if (me == 0)
//     printf("Completed typed broadcast\n");

//   if (me == root) {
//     for (int i = 0; i < ARRAY_SIZE; i++) {
//       if (dest[i] != i + 1) {
//         printf("Error on PE %d: dest[%d]=%d, expected %d\n", me, i, dest[i],
//                i + 1);
//       }
//     }
//     printf("Typed broadcast test completed\n");
//   }

//   shmem_free(source);
//   shmem_free(dest);
// }

// // Test memory broadcast (shmem_broadcastmem)
// static void test_broadcast_mem() {
//   char *source = shmem_malloc(ARRAY_SIZE * sizeof(char));
//   char *dest = shmem_malloc(ARRAY_SIZE * sizeof(char));

//   if (!source || !dest) {
//     printf("Failed to allocate symmetric memory\n");
//     return;
//   }

//   int me = shmem_my_pe();
//   int root = 0;

//   // Initialize arrays to known values
//   for (int i = 0; i < ARRAY_SIZE; i++) {
//     source[i] = (me == root) ? ('a' + i) : 'x';
//     dest[i] = 'x';
//   }

//   if (me == 0)
//     printf("Starting memory broadcast test\n");
//   shmem_barrier_all();

//   // Test memory broadcast with team
//   shmem_broadcastmem(SHMEM_TEAM_WORLD, dest, source, ARRAY_SIZE, root);

//   shmem_barrier_all();
//   if (me == 0)
//     printf("Completed memory broadcast\n");

//   if (me == root) {
//     for (int i = 0; i < ARRAY_SIZE; i++) {
//       if (dest[i] != 'a' + i) {
//         printf("Error on PE %d: dest[%d]=%c, expected %c\n", me, i, dest[i],
//                'a' + i);
//       }
//     }
//     printf("Memory broadcast test completed\n");
//   }

//   shmem_free(source);
//   shmem_free(dest);
// }

// Test legacy sized broadcast (deprecated)
static void test_sized_broadcast() {
  int me = shmem_my_pe();
  int npes = shmem_n_pes();
  int root = 0;

  // Print sync size
  if (me == 0) {
    printf("SHMEM_BCAST_SYNC_SIZE = %d\n", SHMEM_BCAST_SYNC_SIZE);
  }

  // Ensure proper alignment for symmetric arrays
  int *source = (int *)shmem_malloc(ARRAY_SIZE * sizeof(int));
  int *dest = (int *)shmem_malloc(ARRAY_SIZE * sizeof(int));
  long *pSync = (long *)shmem_malloc(SHMEM_BCAST_SYNC_SIZE * sizeof(long));

  if (!source || !dest || !pSync) {
    printf("PE %d: Failed to allocate memory: source=%p dest=%p pSync=%p\n",
           me, (void*)source, (void*)dest, (void*)pSync);
    return;
  }

  // Print addresses
  printf("PE %d: source=%p dest=%p pSync=%p\n", 
         me, (void*)source, (void*)dest, (void*)pSync);

  // Initialize arrays to known values
  for (int i = 0; i < ARRAY_SIZE; i++) {
    source[i] = (me == root) ? i + 100 : -1;
    dest[i] = -1;
  }

  // Initialize pSync array
  for (int i = 0; i < SHMEM_BCAST_SYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }

  // Ensure all PEs have initialized arrays
  shmem_barrier_all();
  printf("PE %d: Arrays initialized\n", me);

  if (me == 0)
    printf("Starting sized broadcast test\n");

  // Test 32-bit broadcast
  printf("PE %d: Calling broadcast\n", me);
  shmem_broadcast32(dest,       // dest
                   source,     // source
                   ARRAY_SIZE, // nelems
                   root,       // PE_root
                   0,          // PE_start
                   0,          // logPE_stride
                   npes,       // PE_size
                   pSync);     // pSync

  printf("PE %d: Broadcast completed\n", me);
  shmem_barrier_all();

  // Verify results
  int errors = 0;
  for (int i = 0; i < ARRAY_SIZE; i++) {
    if (dest[i] != i + 100) {
      printf("Error on PE %d: dest[%d]=%d, expected %d\n", me, i, dest[i],
             i + 100);
      errors++;
    }
  }

  if (me == 0) {
    printf("Sized broadcast test %s\n",
           errors ? "failed" : "completed successfully");
  }

  // Reset pSync before freeing
  shmem_barrier_all();
  for (int i = 0; i < SHMEM_BCAST_SYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }
  shmem_barrier_all();

  shmem_free(source);
  shmem_free(dest);
  shmem_free(pSync);
}

int main(void) {
  shmem_init();

  int me = shmem_my_pe();
  if (me == 0)
    printf("Starting broadcast tests\n");

  test_sized_broadcast();
  shmem_barrier_all();

//   test_typed_broadcast();
//   shmem_barrier_all();

//   test_broadcast_mem();
//   shmem_barrier_all();

  if (me == 0)
    printf("All broadcast tests completed\n");

  shmem_finalize();
  return 0;
}
