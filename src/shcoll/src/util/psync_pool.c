/**
 * @file psync_pool.c
 * @brief Implementation of SHCOLL pSync pool management.
 */

/**
 * NOTE: THIS IS NOT CURRENTLY IN USE
 * TODO: WORK THIS INTO A FUTURE RELEASE
 */

#include "psync_pool.h"
#include "../shcoll.h"
#include "shmemu.h"
#include "shmemc.h"
#include "state.h"
#include <shmem.h> /* Needed for shmem_team_sync, SHMEM_SUCCESS */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <math.h> /* For log2 */

/* --- Defines --- */
/* Value written to pSync[0] to indicate a slot is busy */
#define PSYNC_BUSY_MARKER (SHCOLL_SYNC_VALUE + 1)

/* --- Global SHCOLL Team State Variables (PE-local) --- */
shcoll_psync_team_state_t shcoll_psync_pool_world_state;
shcoll_psync_team_state_t shcoll_psync_pool_shared_state;
/* NOTE: State for dynamic teams is not managed here yet. */

/* --- Internal pSync Pool (Symmetric Memory) --- */
long *shcoll_psync_pool_buffer = NULL;
static size_t psync_pool_total_longs = 0;

/* Helper to calculate pSync array offset in the pool */
static inline long *shcoll_psync_pool_get_offset(int team_psync_idx,
                                                 int slot_idx) {
  if (team_psync_idx < 0 || team_psync_idx >= SHCOLL_MAX_TEAMS) {
    shmemu_warn("Invalid team_psync_idx %d passed to %s", team_psync_idx,
                __func__);
    return NULL;
  }
  if (!shcoll_psync_pool_buffer) {
    shmemu_warn("%s is NULL in %s", "shcoll_psync_pool_buffer", __func__);
    return NULL;
  }

  size_t sync_size; /* Size of pSync array needed for this slot */
  size_t base_offset;

  if (slot_idx < SHCOLL_N_PSYNC_PER_TEAM) {
    /* General collective pSync arrays */
    /* Use largest needed size for general collectives (currently alltoall) */
    sync_size = SHCOLL_ALLTOALL_SYNC_SIZE;
    base_offset = (size_t)team_psync_idx * SHCOLL_N_PSYNC_PER_TEAM * sync_size;
    base_offset += (size_t)slot_idx * sync_size;
  } else if (slot_idx == SHCOLL_N_PSYNC_PER_TEAM) {
    /* Barrier/Sync pSync arrays */
    sync_size = SHCOLL_BARRIER_SYNC_SIZE;
    /* Barrier arrays start after all general collective slots */
    base_offset = (size_t)SHCOLL_MAX_TEAMS * SHCOLL_N_PSYNC_PER_TEAM *
                  SHCOLL_ALLTOALL_SYNC_SIZE;
    base_offset += (size_t)team_psync_idx * sync_size;
  } else {
    /* Invalid slot index */
    shmemu_warn("Invalid slot_idx %d passed to %s for team %d", slot_idx,
                __func__, team_psync_idx);
    return NULL;
  }

  /* Check if the calculated offset is within the allocated pool */
  if (base_offset + sync_size > psync_pool_total_longs) {
    shmemu_warn("Calculated offset (%zu + %zu) exceeds pool size (%zu) for "
                "team %d, slot %d",
                base_offset, sync_size, psync_pool_total_longs, team_psync_idx,
                slot_idx);
    return NULL;
  }

  return shcoll_psync_pool_buffer + base_offset;
}

/* Helper to get the shmemc_team_h corresponding to shcoll state */
/* NOTE: Does not support dynamic teams */
static inline shmemc_team_h
shcoll_psync_pool_get_base_team(shcoll_psync_team_state_t *team_state) {
  if (team_state == &shcoll_psync_pool_world_state) {
    return &shmemc_team_world;
  }
  if (team_state == &shcoll_psync_pool_shared_state) {
    return &shmemc_team_shared;
  }
  /* Add lookup for dynamic teams here if/when supported */
  return NULL;
}

/* Helper function to synchronize the team using its dedicated barrier pSync */
static void shcoll_psync_pool_sync_team(shcoll_psync_team_state_t *team_state) {
  if (!team_state) {
    shmemu_warn("Attempt to sync NULL team state");
    return;
  }

  shmemc_team_h base_team = shcoll_psync_pool_get_base_team(team_state);
  if (!base_team) {
    shmemu_warn("Cannot sync team: failed to get base team handle for state %p",
                (void *)team_state);
    /* Should not happen for predefined teams unless state is corrupted */
    return;
  }

  long *barrier_psync = shcoll_psync_pool_get_offset(team_state->psync_idx,
                                                     SHCOLL_N_PSYNC_PER_TEAM);
  if (!barrier_psync) {
    shmemu_warn("Cannot sync team %d: failed to get barrier pSync pointer. "
                "Falling back to shmem_barrier_all().",
                team_state->psync_idx);
    /* Fallback might be dangerous if not all PEs reach here */
    shmem_barrier_all();
    return;
  }

  /* Determine logPE_stride. Default to 0. */
  /* TODO: logPE_stride is needed for the internal barrier algorithm. */
  /*       Getting this correctly requires inspecting the base team's structure,
   */
  /*       which is straightforward for predefined teams (stride=1 -> log=0) */
  /*       but needs integration with dynamic team creation logic when added. */
  int logPE_stride = 0;
  /* For predefined teams, stride is 1, so logPE_stride is 0. */
  /* if (base_team == &shmemc_team_world) { logPE_stride = 0; } */
  /* else if (base_team == &shmemc_team_shared) { logPE_stride = 0; } */
  /* else { logPE_stride = ???; } */

  /* Use the internal barrier helper (assuming binomial tree implementation) */
  /* barrier_sync_helper_binomial_tree(base_team->start, logPE_stride,
                                    base_team->nranks, barrier_psync); */

  /* Use standard team sync instead */
  shmem_team_sync(base_team);
}

/* --- Management Routines Implementation --- */

int shcoll_psync_pool_init(void) {

  /* Calculate total pSync pool size */
  size_t general_pool_size = (size_t)SHCOLL_MAX_TEAMS *
                             SHCOLL_N_PSYNC_PER_TEAM *
                             SHCOLL_ALLTOALL_SYNC_SIZE;
  size_t barrier_pool_size =
      (size_t)SHCOLL_MAX_TEAMS * SHCOLL_BARRIER_SYNC_SIZE;
  psync_pool_total_longs = general_pool_size + barrier_pool_size;

  if (psync_pool_total_longs == 0) {
    shmemu_warn("Calculated SHCOLL pSync pool size is zero. No pSyncs will be "
                "available.");
    shcoll_psync_pool_buffer = NULL;
  } else {
    /* Allocate the global SHCOLL pSync pool buffer */
    shcoll_psync_pool_buffer =
        shmem_malloc(sizeof(long) * psync_pool_total_longs);
    if (!shcoll_psync_pool_buffer) {
      shmemu_fatal(
          "Failed to allocate internal SHCOLL pSync pool of size %zu longs",
          psync_pool_total_longs);
      return -1; /* Return non-zero on error */
    }

    /* Initialize all pSync slots to the 'free' state */
    for (size_t i = 0; i < psync_pool_total_longs; ++i) {
      shcoll_psync_pool_buffer[i] = SHCOLL_SYNC_VALUE;
    }
    /* Barrier ensures visibility and completion of malloc/init across PEs */
    /* Relying on shmem_init barrier potentially run by caller, */
    /* but an explicit barrier here is safer. */
    shmem_barrier_all();
  }

  /* Initialize PE-local state for predefined teams */
  shcoll_psync_pool_world_state.psync_idx =
      0; /* Corresponds to shmemc_team_world index */

  shcoll_psync_pool_shared_state.psync_idx =
      1; /* Corresponds to shmemc_team_shared index */

  /* Remove initialization of psync_avail */

  /* NOTE: State for dynamic teams needs separate initialization when supported
   */

  return SHMEM_SUCCESS; /* Return standard success code */
}

int shcoll_psync_pool_fini(void) {
  if (shcoll_psync_pool_buffer) {
    shmem_free(shcoll_psync_pool_buffer);
    shcoll_psync_pool_buffer = NULL;
    psync_pool_total_longs = 0;
  }
  /* NOTE: Free dynamic team state map here if/when implemented */
  return SHMEM_SUCCESS; /* Return standard success code */
}

/* Get the PE-local state corresponding to a public team handle */
shcoll_psync_team_state_t *shcoll_psync_pool_get_state(shmem_team_t team) {
  /* Directly compare handles for predefined teams */
  if (team == (shmem_team_t)&shmemc_team_world) {
    return &shcoll_psync_pool_world_state;
  }
  if (team == (shmem_team_t)&shmemc_team_shared) {
    return &shcoll_psync_pool_shared_state;
  }

  /* NOTE: Add lookup for dynamic teams here if/when supported */
  shmemu_warn("%s: Dynamic teams or invalid team handle %p provided. Returning "
              "NULL state.",
              __func__, (void *)team);
  return NULL;
}

/**
 * @brief Allocates a pSync slot using atomic CAS on pSync[0].
 *
 * Tries to atomically swap pSync[0] from SHCOLL_SYNC_VALUE (free) to
 * PSYNC_BUSY_MARKER. If all slots are busy, synchronizes the team and
 * retries once.
 */
int shcoll_psync_alloc(shcoll_psync_team_state_t *team_state,
                       shcoll_psync_op_t op, long **chosen_psync_ptr) {
  if (!team_state || team_state->psync_idx < 0) {
    shmemu_fatal("Invalid team_state provided to %s (psync_idx=%d)", __func__,
                 team_state ? team_state->psync_idx : -2);
    *chosen_psync_ptr = NULL;
    return -1;
  }
  if (!shcoll_psync_pool_buffer && psync_pool_total_longs > 0) {
    shmemu_fatal("%s called but pSync pool buffer is NULL", __func__);
    *chosen_psync_ptr = NULL;
    return -1;
  }

  *chosen_psync_ptr = NULL; /* Default to NULL */
  int my_pe = shmem_my_pe();

  /* Handle barrier/sync operations separately - they use a dedicated slot */
  if (op == SHCOLL_PSYNC_OP_SYNC) {
    *chosen_psync_ptr = shcoll_psync_pool_get_offset(team_state->psync_idx,
                                                     SHCOLL_N_PSYNC_PER_TEAM);
    if (!*chosen_psync_ptr) {
      shmemu_warn(
          "%s: Failed to get offset for dedicated sync slot (team_idx=%d)",
          __func__, team_state->psync_idx);
      return -1; /* Offset calculation failed */
    }
    /* Return special index indicating the dedicated sync/barrier slot */
    return SHCOLL_N_PSYNC_PER_TEAM;
  }

  /* Allocation attempts for general collective slots */
  for (int attempt = 0; attempt < 2; ++attempt) {
    for (int i = 0; i < SHCOLL_N_PSYNC_PER_TEAM; ++i) {
      long *psync_slot = shcoll_psync_pool_get_offset(team_state->psync_idx, i);
      if (!psync_slot) {
        /* Should not happen if pool init was successful and index is valid */
        shmemu_warn("%s: Failed to get offset for team %d slot %d (attempt %d)",
                    __func__, team_state->psync_idx, i, attempt);
        continue; /* Try next slot */
      }

      /* Attempt to atomically acquire the lock on pSync[0] */
      long old_val = shmem_long_atomic_compare_swap(
          psync_slot,        /* Target address (pSync[0]) */
          SHCOLL_SYNC_VALUE, /* Expected old value (free) */
          PSYNC_BUSY_MARKER, /* New value (busy) */
          my_pe);            /* Target PE for the atomic operation */

      if (old_val == SHCOLL_SYNC_VALUE) {
        /* Successfully acquired the lock (slot was free) */
        *chosen_psync_ptr = psync_slot;
        /* Ensure the pSync array (beyond index 0) is initialized if needed */
        /* Typically, the collective algorithm using it will initialize */
        return i; /* Return the acquired slot index */
      }
      /* Slot was busy (or CAS failed spuriously, though unlikely for SHMEM) */
    }

    /* If we are here after the first attempt, all slots were busy */
    if (attempt == 0) {
      shmemu_warn("No pSync slot available for team %d, op %d. Synchronizing "
                  "team and retrying.",
                  team_state->psync_idx, op);
      shcoll_psync_pool_sync_team(team_state);
      /* After sync, all previous operations using slots should be complete. */
      /* The slots *should* have been freed by those operations. */
      /* We don't explicitly reset them here; freeing is the responsibility */
      /* of the user of shcoll_psync_free. */
    } else {
      /* Failed even after sync and retry */
      shmemu_warn("%s: Failed to acquire pSync slot for team %d, op %d even "
                  "after sync.",
                  __func__, team_state->psync_idx, op);
    }
  }

  /* Failed to acquire a slot after two attempts */
  *chosen_psync_ptr = NULL;
  return -1;
}

/**
 * @brief Releases a previously allocated pSync slot using atomic set on
 * pSync[0].
 */
void shcoll_psync_free(shcoll_psync_team_state_t *team_state,
                       shcoll_psync_op_t op, int used_slot_index) {
  if (!team_state || team_state->psync_idx < 0) {
    shmemu_warn("Attempt to free pSync with invalid team_state");
    return; /* Invalid state */
  }

  /* Don't release the dedicated barrier/sync slot - it's managed implicitly */
  if (op == SHCOLL_PSYNC_OP_SYNC ||
      used_slot_index == SHCOLL_N_PSYNC_PER_TEAM) {
    return;
  }

  /* Check if the slot index is valid for general collectives */
  if (used_slot_index >= 0 && used_slot_index < SHCOLL_N_PSYNC_PER_TEAM) {
    long *psync_slot =
        shcoll_psync_pool_get_offset(team_state->psync_idx, used_slot_index);
    if (!psync_slot) {
      shmemu_warn("%s: Failed to get offset for team %d slot %d", __func__,
                  team_state->psync_idx, used_slot_index);
      return; /* Cannot free if we can't get the pointer */
    }

    /* Atomically set slot back to available (SHCOLL_SYNC_VALUE) */
    /* This acts as unlocking the slot. */
    shmem_long_atomic_set(psync_slot, SHCOLL_SYNC_VALUE, shmem_my_pe());

  } else {
    shmemu_warn("Attempted to release invalid pSync slot index %d for team %d",
                used_slot_index, team_state->psync_idx);
  }
}

/**
 * @brief Queries the status of a pSync slot by checking pSync[0].
 */
int shcoll_psync_query(long *psync) {
  if (!psync) {
    shmemu_warn("%s: Called with NULL pSync pointer", __func__);
    return -1; /* Error: NULL pointer */
  }

  /* Check if the pSync array is accessible (basic check) */
  /* A more robust check might involve verifying it's within the known pool
   * range*/
  if (!shmem_addr_accessible(psync, shmem_my_pe())) {
    shmemu_warn("%s: Provided pSync pointer %p is not accessible", __func__,
                (void *)psync);
    return -1; /* Error: Not accessible */
  }

  /* Read the lock status from pSync[0] */
  /* Use atomic fetch or simple read depending on memory model guarantees */
  /* A simple read might suffice if atomics ensure visibility */
  long lock_value = psync[0]; /* Simple read, assumes visibility */
  /* Alternative: lock_value = shmem_long_atomic_fetch(psync, shmem_my_pe()); */

  if (lock_value == SHCOLL_SYNC_VALUE) {
    return 0; /* Free */
  } else {
    return 1; /* Busy */
  }
}