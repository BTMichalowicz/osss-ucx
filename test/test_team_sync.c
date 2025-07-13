#include "../build/build/include/shmem.h"
#include <stdio.h>



int main(void) {
  /* Initialize the SHMEM environment */
  shmem_init();

  /* Get the current PE number and the total number of PEs */
  int my_pe = shmem_my_pe();
  int npes = shmem_n_pes();

  if (my_pe == 0) {
    printf("SHMEM initialized.\n");
    printf("PE %d out of %d PEs.\n", my_pe, npes);
  }

  /* Define a new team variable */
  shmem_team_t team;

  /* Split the SHMEM_TEAM_WORLD into a new team containing all PEs */
  /* Parameters: parent_team, start, stride, size, config, config_mask, new_team
   */
  int split_ret =
      shmem_team_split_strided(SHMEM_TEAM_WORLD, 0, 1, npes, NULL, 0, &team);
  if (split_ret != 0) {
    if (my_pe == 0) {
      printf("shmem_team_split_strided failed with return code: %d\n",
             split_ret);
    }
    shmem_finalize();
    return 1;
  }

  if (my_pe == 0) {
    printf("Team split successfully.\n");
  }

  /* Define a new context variable */
  shmem_ctx_t ctx;

  /* Create a new context for the newly created team */
  /* Parameters: team, options, new_context */
  int ret = shmem_team_create_ctx(team, 0, &ctx);
  if (ret != 0) {
    if (my_pe == 0) {
      printf("shmem_team_create_ctx failed with return code: %d\n", ret);
    }
    shmem_finalize();
    return 1;
  }

  if (my_pe == 0) {
    printf("Context created successfully.\n");
  }

  /* Perform team synchronization */
  /* Parameter: team */
  // ret = shmem_team_sync(team);
  ret = shmem_sync(team);
  if (ret != 0) {
    if (my_pe == 0) {
      printf("shmem_team_sync failed with return code: %d\n", ret);
    }
    shmem_finalize();
    return 1;
  }

  if (my_pe == 0) {
    printf("Team synchronization completed.\n");
    printf("shmem_team_sync succeeded on team with %d PEs.\n", npes);
  }

  /* Destroy the created context and team to clean up */
  shmem_ctx_destroy(ctx);
  shmem_team_destroy(team);

  if (my_pe == 0) {
    printf("Context and team destroyed successfully.\n");
  }

  /* Finalize the SHMEM environment */
  shmem_finalize();

  if (my_pe == 0) {
    printf("SHMEM finalized.\n");
  }

  return 0;
}
