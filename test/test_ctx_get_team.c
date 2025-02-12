#include "../build/build/include/shmem.h"
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

    // Get the team from the context
    shmem_team_t retrieved_team;
    ret = shmem_ctx_get_team(ctx, &retrieved_team);
    printf("shmem_ctx_get_team return value: %d\n", ret);
    printf("Retrieved team: %p\n", (void *)retrieved_team);

    if (ret != 0) {
        printf("shmem_ctx_get_team failed\n");
        shmem_finalize();
        return 1;
    }

    // Verify the team
    if (retrieved_team != team) {
        printf("shmem_ctx_get_team returned incorrect team\n");
        shmem_finalize();
        return 1;
    }

    // Destroy the context and team
    shmem_ctx_destroy(ctx);
    shmem_team_destroy(team);

    shmem_finalize();
    printf("shmem_ctx_get_team test passed\n");
    return 0;
}
