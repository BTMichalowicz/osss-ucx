#include <shmem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    shmem_init();
    
    int world_mype = shmem_my_pe();
    int world_npes = shmem_n_pes();
    
    if (world_npes < 2) {
        if (world_mype == 0) {
            printf("This test requires at least 2 PEs\n");
        }
        shmem_finalize();
        return EXIT_SUCCESS;
    }
    
    printf("PE %d: Starting simple iput debug test\n", world_mype);
    
    // Test the exact pattern from VV test
    static float src[10], dest[10];
    
    // Initialize arrays
    for (int i = 0; i < 10; i++) {
        src[i] = i + world_mype;  // PE 0: [0,1,2,3,4,5,6,7,8,9], PE 1: [1,2,3,4,5,6,7,8,9,10]
        dest[i] = -999.0f;  // Initialize to sentinel value
    }
    
    printf("PE %d: Initialized arrays\n", world_mype);
    
    shmem_barrier_all();
    
    if (world_mype == 0) {
        printf("PE 0: About to call shmem_float_iput(dest, src, 2, 2, 5, 1)\n");
        printf("PE 0: This should send src[0,2,4,6,8] = [%g,%g,%g,%g,%g] to dest[0,2,4,6,8] on PE 1\n",
               src[0], src[2], src[4], src[6], src[8]);
        
        // This is the exact call from the VV test
        shmem_float_iput(dest, src, 2, 2, 5, 1);
        
        printf("PE 0: shmem_float_iput completed\n");
    }
    
    shmem_barrier_all();
    
    if (world_mype == 1) {
        printf("PE 1: Received dest array: [");
        for (int i = 0; i < 10; i++) {
            printf("%g", dest[i]);
            if (i < 9) printf(",");
        }
        printf("]\n");
        
        printf("PE 1: Validating received data:\n");
        bool success = true;
        for (int i = 0; i < 10; i += 2) {
            int expected = i / 2;  // 0,1,2,3,4
            printf("PE 1: dest[%d] = %g, expected %d\n", i, dest[i], expected);
            if (dest[i] != expected) {
                printf("PE 1: FAIL - dest[%d] = %g, expected %d\n", i, dest[i], expected);
                success = false;
            }
        }
        
        if (success) {
            printf("PE 1: SUCCESS - All values correct!\n");
        } else {
            printf("PE 1: FAILURE - Some values incorrect!\n");
        }
    }
    
    shmem_finalize();
    return EXIT_SUCCESS;
} 