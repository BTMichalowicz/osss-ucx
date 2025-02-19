/*
 * test_sync.c
 *
 * This test program demonstrates the use of both the deprecated active‑set
 * synchronization (via a 4-argument call to shmem_sync) and the new
 * C11 team‑based synchronization (via a one-argument call).
 *
 * The public headers define a variadic macro in generics.h that dispatches:
 *
 *   shmem_sync(PE_start, logPE_stride, PE_size, pSync)
 *      → shmem_sync_deprecated(PE_start, logPE_stride, PE_size, pSync)
 *
 *   shmem_sync(team)
 *      → shmem_team_sync(team)
 *
 * In this test the following steps are performed:
 *
 *  1. Initialize SHMEM and report the PE number and total PEs.
 *  2. Create a symmetric "pSync" array and call the deprecated active‑set
 * variant.
 *  3. Call the team‑based variant via a one-argument call (using
 * SHMEM_TEAM_WORLD).
 *  4. Split SHMEM_TEAM_WORLD into a new team, create a context for that team,
 *     and then perform an explicit team synchronization on that new team.
 *  5. Clean up by destroying the context and team, then finalize SHMEM.
 *
 * Note: This test is compiled as an external user application. Do not define
 * SHMEM_INTERNAL here.
 */

#include "../build/build/include/shmem.h"
#include <stdio.h>

#define PSYNC_SIZE 128

/* Function to test the deprecated active-set-based shmem_sync */
void test_active_set_sync() {
  int my_pe = shmem_my_pe();
  int npes = shmem_n_pes();

  /* Allocate symmetric memory for pSync */
  long *pSync = (long *)shmem_malloc(PSYNC_SIZE * sizeof(long));
  for (int i = 0; i < PSYNC_SIZE; i++) {
    pSync[i] = SHMEM_SYNC_VALUE;
  }

  /* Call the deprecated active-set-based shmem_sync */
  shmem_sync(0, 0, npes, pSync);

  if (my_pe == 0) {
    printf("Deprecated active-set shmem_sync executed.\n");
  }

  shmem_free(pSync);
}

/* Function to test the C11 team-based shmem_sync */
void test_team_based_sync() {
  int my_pe = shmem_my_pe();
  int npes = shmem_n_pes();

  shmem_team_t twos_team = SHMEM_TEAM_INVALID;
  shmem_team_t threes_team = SHMEM_TEAM_INVALID;
  shmem_team_config_t *config = NULL;

  if (npes > 2) {
    shmem_team_split_strided(SHMEM_TEAM_WORLD, 2, 2, (npes - 1) / 2, config, 0,
                             &twos_team);
  }
  if (npes > 3) {
    shmem_team_split_strided(SHMEM_TEAM_WORLD, 3, 3, (npes - 1) / 3, config, 0,
                             &threes_team);
  }

  int mype_twos = shmem_team_my_pe(twos_team);
  int mype_threes = shmem_team_my_pe(threes_team);
  int npes_twos = shmem_team_n_pes(twos_team);
  int npes_threes = shmem_team_n_pes(threes_team);

  if (twos_team != SHMEM_TEAM_INVALID) {
    int x = 10101;
    shmem_p(&x, 2,
            shmem_team_translate_pe(twos_team, (mype_twos + 1) % npes_twos,
                                    SHMEM_TEAM_WORLD));
    shmem_quiet();
    shmem_sync(twos_team);
  }

  shmem_sync(SHMEM_TEAM_WORLD);

  if (threes_team != SHMEM_TEAM_INVALID) {
    int x = 10101;
    shmem_p(&x, 3,
            shmem_team_translate_pe(threes_team,
                                    (mype_threes + 1) % npes_threes,
                                    SHMEM_TEAM_WORLD));
    shmem_quiet();
    shmem_sync(threes_team);
  }

  int x = 10101;
  if (my_pe && my_pe % 3 == 0) {
    if (x != 3)
      shmem_global_exit(3);
  } else if (my_pe && my_pe % 2 == 0) {
    if (x != 2)
      shmem_global_exit(2);
  } else if (x != 1) {
    shmem_global_exit(1);
  }
}

int main(void) {
  /* Initialize the SHMEM environment */
  shmem_init();

  /* Test the deprecated active-set-based shmem_sync */
  test_active_set_sync();

  /* Synchronize all PEs */
  shmem_barrier_all();

  /* Test the C11 team-based shmem_sync */
  test_team_based_sync();

  /* Finalize the SHMEM environment */
  shmem_finalize();

  return 0;
}
