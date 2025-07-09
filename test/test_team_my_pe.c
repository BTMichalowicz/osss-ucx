#include <shmem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    shmem_init();
    
    int world_mype = shmem_my_pe();
    int world_npes = shmem_n_pes();
    
    printf("PE %d: Starting test (world has %d PEs)\n", world_mype, world_npes);
    
    // Test 1: Create a team that should be identical to SHMEM_TEAM_WORLD
    shmem_team_t team;
    int ret = shmem_team_split_strided(SHMEM_TEAM_WORLD, 0, 1, world_npes, NULL, 0, &team);
    
    if (ret != 0) {
        printf("PE %d: ERROR - Team split failed with code %d\n", world_mype, ret);
        shmem_finalize();
        return EXIT_FAILURE;
    }
    
    if (team == SHMEM_TEAM_INVALID) {
        printf("PE %d: ERROR - Got SHMEM_TEAM_INVALID\n", world_mype);
        shmem_finalize();
        return EXIT_FAILURE;
    }
    
    shmem_barrier_all();
    
    // Check team size
    int team_npes = shmem_team_n_pes(team);
    printf("PE %d: Team has %d PEs (expected %d)\n", world_mype, team_npes, world_npes);
    
    // Check my PE in team
    int team_mype = shmem_team_my_pe(team);
    printf("PE %d: My team PE = %d (should be %d)\n", world_mype, team_mype, world_mype);
    
    // Verify translation works both ways
    int translated_to_world = shmem_team_translate_pe(team, team_mype, SHMEM_TEAM_WORLD);
    int translated_to_team = shmem_team_translate_pe(SHMEM_TEAM_WORLD, world_mype, team);
    
    printf("PE %d: team->world translation: %d->%d (should be %d)\n", 
           world_mype, team_mype, translated_to_world, world_mype);
    printf("PE %d: world->team translation: %d->%d (should be %d)\n", 
           world_mype, world_mype, translated_to_team, world_mype);
    
    // Test passed if team_mype equals world_mype
    bool test_passed = (team_mype == world_mype) && 
                      (translated_to_world == world_mype) && 
                      (translated_to_team == world_mype);
    
    printf("PE %d: Test %s\n", world_mype, test_passed ? "PASSED" : "FAILED");
    
    shmem_team_destroy(team);
    shmem_finalize();
    
    return test_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
