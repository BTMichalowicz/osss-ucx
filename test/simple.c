#include "../build/install/include/shmem.h"
#include "../build/install/include/shmem/defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main() {
    shmem_init();
    int mype = shmem_my_pe();
    int npes = shmem_n_pes();
    int *data = shmem_malloc(sizeof(int));
    int *recv = shmem_malloc(sizeof(int));
    if (!data || !recv) {
        printf("PE %d: Memory allocation failed\n", mype);
        shmem_finalize();
        return 1;
    }
    *data = mype;
    *recv = -1;
    int dest_pe = (mype + 1) % npes;
    shmem_put(recv, data, sizeof(int), dest_pe);
    shmem_quiet();
    shmem_barrier_all();
    printf("PE %d: recv = %d (expected %d)\n", mype, *recv, (mype - 1 + npes) % npes);
    shmem_free(data);
    shmem_free(recv);
    shmem_finalize();
    return 0;
}