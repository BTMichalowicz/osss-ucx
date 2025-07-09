/**
 * @file psync_pool.h
 * @brief Defines the pSync pool management for SHCOLL collectives.
 * @details Manages a pool of pSync arrays required by various collective
 * algorithms, complementing the base team structures in shmemc.h.
 */

#ifndef SHCOLL_PSYNC_POOL_H
#define SHCOLL_PSYNC_POOL_H

#include <shmem.h>
#include <shmemx.h>
#include <shmemc.h>

#include "shcoll.h"

/* --- Configuration (Moved from shcoll_teams.h) --- */

/* Number of pSync arrays allocated per team for general collectives */
#define SHCOLL_N_PSYNC_PER_TEAM 2

/* Maximum number of teams supported (influences pSync pool size) */
/* NOTE: Dynamic teams are not currently fully supported by this pool */
#define SHCOLL_MAX_TEAMS 16

/* --- Structures (Moved from shcoll_teams.h) --- */

/**
 * @brief Internal structure holding pSync pool state specific to a team.
 * This complements the main shmemc_team_t structure.
 * NOTE: This structure is local to each PE.
 */
typedef struct shcoll_psync_team_state_t { /* Renamed */
  int psync_idx; /* Index into the global pSync pool for this team's base */
                 /* (-1 if unassigned or not applicable) */
  /* Removed psync_avail array; synchronization is now done via pSync[0] */
} shcoll_psync_team_state_t;

/* --- Global Variables (Moved/Adapted from shcoll_teams.h) --- */

/* Extern declarations for predefined teams' pSync state (local to each PE) */
extern shcoll_psync_team_state_t shcoll_psync_pool_world_state;  /* Renamed */
extern shcoll_psync_team_state_t shcoll_psync_pool_shared_state; /* Renamed */

/* Internal pSync Pool buffer (in symmetric memory) */
extern long
    *shcoll_psync_pool_buffer; /* Renamed from shcoll_internal_psync_pool */

/* --- Collective Operation Types (Moved from shcoll_teams.h) --- */
/* Used by pSync allocation functions */
typedef enum shcoll_psync_op_t { /* Renamed */
                                 SHCOLL_PSYNC_OP_SYNC =
                                     0, /* Includes barrier */
                                 SHCOLL_PSYNC_OP_BCAST,
                                 SHCOLL_PSYNC_OP_REDUCE,
                                 SHCOLL_PSYNC_OP_COLLECT, /* Includes fcollect
                                                           */
                                 SHCOLL_PSYNC_OP_ALLTOALL /* Includes alltoalls
                                                           */
} shcoll_psync_op_t;

/* --- Management Routines --- */

/**
 * @brief Initializes the global pSync pool.
 * Needs to be called *after* shmemc_teams_init.
 * @return SHCOLL_SUCCESS on success, SHCOLL_ERROR_NOMEM on failure.
 */
int shcoll_psync_pool_init(void);

/**
 * @brief Finalizes the global pSync pool.
 * Needs to be called *before* shmemc_teams_fini.
 */
int shcoll_psync_pool_fini(void);

/**
 * @brief Gets the pSync state structure for a given team handle.
 * NOTE: Returns pointer to PE-local state. Dynamic teams not supported.
 * @param team The public team handle (shmem_team_t).
 * @return Pointer to the shcoll_psync_team_state_t structure, or NULL if not
 * found/supported.
 */
shcoll_psync_team_state_t *
shcoll_psync_pool_get_state(shmem_team_t team); /* Renamed */

/**
 * @brief Allocates an available pSync array from the pool for a specific
 * collective operation on a team. Handles contention and synchronization.
 * Uses atomic operations on pSync[0] for locking.
 * @param team_state Pointer to the team's pSync state structure (PE-local).
 * @param op The type of collective operation being performed.
 * @param [out] chosen_psync_ptr Pointer to store the address of the allocated
 * pSync.
 * @return The index (0 to SHCOLL_N_PSYNC_PER_TEAM-1 or special value for sync)
 * of the acquired pSync slot, or -1 on failure.
 */
int shcoll_psync_alloc(
    shcoll_psync_team_state_t *team_state, /* Renamed param type */
    shcoll_psync_op_t op,                  /* Renamed param type */
    long **chosen_psync_ptr);              /* Renamed */

/**
 * @brief Releases a specific pSync array slot back to the pool.
 * Uses atomic operations on pSync[0] for unlocking.
 * @param team_state Pointer to the team's pSync state structure (PE-local).
 * @param op The type of collective operation that finished.
 * @param used_slot_index The index of the slot that was used.
 */
void shcoll_psync_free(
    shcoll_psync_team_state_t *team_state, /* Renamed param type */
    shcoll_psync_op_t op,                  /* Renamed param type */
    int used_slot_index);                  /* Renamed */

/**
 * @brief Queries the status of a pSync array by checking its first element.
 * @param psync Pointer to the pSync array (must be valid and symmetric).
 * @return 0 if pSync[0] == SHCOLL_SYNC_VALUE (free), 1 otherwise (busy).
 *         Returns -1 on error (e.g., NULL psync).
 */
int shcoll_psync_query(long *psync);

/* --- Deprecated/Removed definitions from original psync_pool.h --- */
/* Removed: #define MAX_NUM_PSYNC_POOL_SZ 16 (now SHCOLL_MAX_TEAMS) */
/* Removed: #define MAX_RECURSION_DEPTH 16 (belongs elsewhere?) */
/* Removed: extern long *shcoll_psync_pool[MAX_NUM_PSYNC_POOL_SZ]; (replaced by
 * shcoll_psync_pool_buffer) */
/* Removed: old definitions replaced by renamed/updated ones */

#endif /* SHCOLL_PSYNC_POOL_H */