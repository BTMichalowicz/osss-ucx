#include <shmem.h>
#include <stdio.h>

int main(void) {
    shmem_init();

    shmem_team_t team;
    shmem_ctx_t ctx;

    // Split the world team into a new team
    shmem_team_split_strided(SHMEM_TEAM_WORLD, 0, 1, shmem_n_pes(), NULL, 0, &team);

    // Create a context for the new team
    int ret = shmem_team_create_ctx(team, 0, &ctx);
    if (ret != 0) {
        printf("shmem_team_create_ctx failed\n");
        shmem_finalize();
        return 1;
    }

    // Perform some operations with the context (optional)
    // ...

    // Debugging: Print context and team information
    printf("Context created: %p\n", (void *)ctx);
    printf("Team created: %p\n", (void *)team);

    // Destroy the context and team
    shmem_ctx_destroy(ctx);
    shmem_team_destroy(team);

    shmem_finalize();
    printf("shmem_team_create_ctx test passed\n");
    return 0;
}
