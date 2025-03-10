/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"
#include "shmem/teams.h"
#include "ucx/api.h"
#include "allocator/memalloc.h"
#include "module.h"

#include <stdlib.h>

shmemc_team_t shmemc_team_world;
shmemc_team_t shmemc_team_shared;

static shmemc_team_h world = &shmemc_team_world;
static shmemc_team_h shared = &shmemc_team_shared;
static shmemc_team_h invalid = NULL;

/*
 * clear up the allocated SHMEM contexts in a team
 */

static void shmemc_team_contexts_destroy(shmemc_team_h th) {
  size_t c;

  for (c = 0; c < th->nctxts; ++c) {
    shmemc_ucx_teardown_context(th->ctxts[c]);
  }
  free(th->ctxts);
}

#if 0
static void
dump_team(shmemc_team_h th)
{
   int key, val;

   printf("==========================================\n");

   kh_foreach(th->fwd, key, val,
              {
                  printf("fwd: %d -> %d\n", key, val);
              }
              );
   kh_foreach(th->rev, key, val,
              {
                  printf("rev: %d -> %d\n", key, val);
              }
              );
   printf("\n");

   printf("Team = %p (%s)\n", (void *) th, th->name);

   printf("  global rank = %d, mype = %4d, npes = %4d\n",
          proc.li.rank,
          th->rank,
          th->nranks);
   printf("------------------------------------------\n");
}
#endif

static void initialize_psync_buffers(shmemc_team_h th) {
  unsigned nsync;

  for (nsync = 0; nsync < SHMEMC_NUM_PSYNCS; ++nsync) {
    unsigned i;

    const size_t nbytes = SHMEM_BARRIER_SYNC_SIZE * sizeof(*(th->pSyncs));
    th->pSyncs[nsync] = (long *)shmema_malloc(nbytes);

    shmemu_assert(th->pSyncs[nsync] != NULL,
                  MODULE ": can't allocate sync memory "
                         "#%u in %s team (%p)",
                  nsync, th->parent == NULL ? th->name : "created", th);

    for (i = 0; i < SHMEM_BARRIER_SYNC_SIZE; ++i) {
      th->pSyncs[nsync][i] = SHMEM_SYNC_VALUE;
    }
  }
}

static void finalize_psync_buffers(shmemc_team_h th) {
  unsigned nsync;

  for (nsync = 0; nsync < SHMEMC_NUM_PSYNCS; ++nsync) {
    shmema_free(th->pSyncs[nsync]);
  }
}

/*
 * common setup
 */
static void initialize_common_team(shmemc_team_h th, const char *name,
                                   int cfg_nctxts) {
  th->parent = NULL;
  th->name = name;

  /* nothing allocated yet */
  th->nctxts = 0;
  th->ctxts = NULL;

  th->cfg.num_contexts = cfg_nctxts;

  th->fwd = kh_init(map);
  th->rev = kh_init(map);

  initialize_psync_buffers(th);
}

/*
 * set up world/shared per PE
 *
 */

static void initialize_team_world(void) {
  int i;
  int absent;

  initialize_common_team(world, "world", proc.env.prealloc_contexts);

  /* populate from launch info */
  world->rank = proc.li.rank;
  world->nranks = proc.li.nranks;

  for (i = 0; i < proc.li.nranks; ++i) {
    khiter_t k;

    k = kh_put(map, world->fwd, i, &absent);
    kh_val(world->fwd, k) = i;
    k = kh_put(map, world->rev, i, &absent);
    kh_val(world->rev, k) = i;
  }
}

static void initialize_team_shared(void) {
  int i;
  int absent;

  initialize_common_team(shared, "shared",
                         proc.env.prealloc_contexts / proc.li.nnodes);

  shared->rank = -1;
  shared->nranks = proc.li.npeers;

  for (i = 0; i < proc.li.npeers; ++i) {
    khiter_t k;

    if (proc.li.rank == proc.li.peers[i]) {
      shared->rank = i;
    }

    k = kh_put(map, shared->fwd, i, &absent);
    kh_val(shared->fwd, k) = proc.li.peers[i];
    k = kh_put(map, shared->rev, proc.li.peers[i], &absent);
    kh_val(shared->rev, k) = i;
  }
}

/*
 * clean up team resources at end
 */

static void finalize_team(shmemc_team_h th) {
  finalize_psync_buffers(th);

  shmemc_team_contexts_destroy(th);
}

void shmemc_teams_init(void) {
  initialize_team_world();
  initialize_team_shared();
}

void shmemc_teams_finalize(void) {
  finalize_team(shared);
  finalize_team(world);
}

/*
 * ----------------------------------------------------------------
 */

/*
 * per-team rank queries
 */

int shmemc_team_my_pe(shmemc_team_h th) { return th->rank; }

int shmemc_team_n_pes(shmemc_team_h th) { return th->nranks; }

/*
 * retrieve the team's configuration
 */

int shmemc_team_get_config(shmemc_team_h th, long config_mask,
                           shmem_team_config_t *config) {
  /* Initialize config structure to zero */
  memset(config, 0, sizeof(shmem_team_config_t));

  /* Apply the configuration mask to retrieve requested parameters */
  if (config_mask & SHMEM_TEAM_NUM_CONTEXTS) {
    config->num_contexts = th->cfg.num_contexts;
  }

  /* Add handling for other configuration parameters as they are added */

  return 0;
}

/*
 * what's the SOURCEHANDLE team SRC_PE in the DESTHANDLE team?
 */

int shmemc_team_translate_pe(shmemc_team_h sh, int src_pe, shmemc_team_h dh) {
  khiter_t k;
  int wpe;

  /* can we find the source PE? */
  k = kh_get(map, sh->fwd, src_pe);
  if (k == kh_end(sh->fwd)) {
    return -1;
    /* NOT REACHED */
  }

  /* world equiv PE */
  wpe = kh_val(sh->fwd, k);

  /* map to world equiv in destination team */
  k = kh_get(map, dh->rev, wpe);
  if (k == kh_end(dh->rev)) {
    return -1;
    /* NOT REACHED */
  }

  /* world equiv is this in destination team */
  return kh_val(dh->rev, k);
}

static bool is_member(int parent_pe, int start, int stride) {
  return ((parent_pe - start) % stride) == 0;
}

int shmemc_team_split_strided(shmemc_team_h parh, int start, int stride,
                              int size, const shmem_team_config_t *config,
                              long config_mask, shmemc_team_h *newh) {
  int i;    /* new team PE # */
  int walk; /* iterate over parent PEs */
  shmemc_team_h newt;
  int absent;
  int nc;

  newt = (shmemc_team_h)malloc(sizeof(*newt));
  if (newt == NULL) {
    *newh = SHMEM_TEAM_INVALID;
    return -1;
  }

  nc = (config_mask & SHMEM_TEAM_NUM_CONTEXTS) ? config->num_contexts : 0;

  initialize_common_team(newt, NULL, nc);

  newt->parent = parh;
  newt->nranks = size;

  /* Initialize rank to -1 (invalid) */
  newt->rank = -1;

  walk = start;
  for (i = 0; i < size; ++i) {
    khint_t k;

    k = kh_get(map, parh->fwd, walk);
    const int up = kh_val(parh->fwd, k);

    /* Check if this PE is part of the team */
    if (is_member(up, start, size)) {
      k = kh_put(map, newt->fwd, i, &absent);
      kh_val(newt->fwd, k) = up;

      k = kh_put(map, newt->rev, up, &absent);
      kh_val(newt->rev, k) = i;

      /* If this is the calling PE, set the team rank */
      if (up == proc.li.rank) {
        newt->rank = i;
      }
    }

    walk += stride;
  }

  /* Verify that the calling PE is part of the team */
  // if (newt->rank == -1) {
  //   shmemu_warn("Calling PE %d is not part of the new team", proc.li.rank);
  // }

  *newh = newt;

  return 0;
}

int shmemc_team_split_2d(shmemc_team_h parh, int xrange,
                         const shmem_team_config_t *xaxis_config,
                         long xaxis_mask, shmemc_team_h *xaxish,
                         const shmem_team_config_t *yaxis_config,
                         long yaxis_mask, shmemc_team_h *yaxish) {
  int parent_size, my_pe_in_parent;
  int yrange;
  int my_x, my_y;
  int i, ret;
  shmemc_team_h xaxis_team = NULL;
  shmemc_team_h yaxis_team = NULL;

  /* Get the parent team size and our PE in the parent team */
  parent_size = parh->nranks;
  my_pe_in_parent = parh->rank;

  /* Boundary check for xrange */
  if (xrange <= 0) {
    shmemu_warn("xrange must be positive");
    return -1;
  }

  /* If xrange is greater than parent team size, treat it as equal to parent
   * size */
  if (xrange > parent_size) {
    xrange = parent_size;
  }

  /* Calculate yrange: ceiling of (parent_size / xrange) */
  yrange = (parent_size + xrange - 1) / xrange;

  /* Calculate our coordinates in the 2D grid */
  my_x = my_pe_in_parent % xrange;
  my_y = my_pe_in_parent / xrange;

  /* Create the x-axis team (all PEs with the same y coordinate) */
  xaxis_team = (shmemc_team_h)malloc(sizeof(*xaxis_team));
  if (xaxis_team == NULL) {
    goto cleanup;
  }

  /* Initialize the x-axis team */
  int nc_x =
      (xaxis_mask & SHMEM_TEAM_NUM_CONTEXTS) ? xaxis_config->num_contexts : 0;
  initialize_common_team(xaxis_team, NULL, nc_x);
  xaxis_team->parent = parh;

  /* x-axis team size is minimum of xrange or remaining PEs in last row */
  if (my_y == yrange - 1 && parent_size % xrange != 0) {
    /* Last row might be incomplete */
    xaxis_team->nranks = parent_size % xrange;
  } else {
    xaxis_team->nranks = xrange;
  }

  /* Initialize rank to -1 (invalid) */
  xaxis_team->rank = -1;

  /* Map PEs to the x-axis team */
  int absent;
  int x_team_idx = 0;

  /* Populate the x-axis team with PEs that have the same y-coordinate */
  for (i = 0; i < parent_size; i++) {
    int pe_y = i / xrange;

    /* Only include PEs with the same y-coordinate as me */
    if (pe_y == my_y) {
      khint_t k;
      int global_pe;

      /* Get the global PE from the parent team */
      k = kh_get(map, parh->fwd, i);
      global_pe = kh_val(parh->fwd, k);

      /* Add to the x-axis team mapping */
      k = kh_put(map, xaxis_team->fwd, x_team_idx, &absent);
      kh_val(xaxis_team->fwd, k) = global_pe;

      k = kh_put(map, xaxis_team->rev, global_pe, &absent);
      kh_val(xaxis_team->rev, k) = x_team_idx;

      /* If this is me, set my rank in the x-axis team */
      if (i == my_pe_in_parent) {
        xaxis_team->rank = x_team_idx;
      }

      x_team_idx++;
    }
  }

  /* Create the y-axis team (all PEs with the same x coordinate) */
  yaxis_team = (shmemc_team_h)malloc(sizeof(*yaxis_team));
  if (yaxis_team == NULL) {
    goto cleanup;
  }

  /* Initialize the y-axis team */
  int nc_y =
      (yaxis_mask & SHMEM_TEAM_NUM_CONTEXTS) ? yaxis_config->num_contexts : 0;
  initialize_common_team(yaxis_team, NULL, nc_y);
  yaxis_team->parent = parh;

  /* y-axis team size is at most yrange */
  int actual_y_size = (my_x < parent_size % xrange && parent_size % xrange != 0)
                          ? yrange
                          : (parent_size - 1) / xrange + 1;
  yaxis_team->nranks = actual_y_size;

  /* Initialize rank to -1 (invalid) */
  yaxis_team->rank = -1;

  /* Map PEs to the y-axis team */
  int y_team_idx = 0;

  /* Populate the y-axis team with PEs that have the same x-coordinate */
  for (i = 0; i < parent_size; i++) {
    int pe_x = i % xrange;

    /* Only include PEs with the same x-coordinate as me */
    if (pe_x == my_x) {
      khint_t k;
      int global_pe;

      /* Get the global PE from the parent team */
      k = kh_get(map, parh->fwd, i);
      global_pe = kh_val(parh->fwd, k);

      /* Add to the y-axis team mapping */
      k = kh_put(map, yaxis_team->fwd, y_team_idx, &absent);
      kh_val(yaxis_team->fwd, k) = global_pe;

      k = kh_put(map, yaxis_team->rev, global_pe, &absent);
      kh_val(yaxis_team->rev, k) = y_team_idx;

      /* If this is me, set my rank in the y-axis team */
      if (i == my_pe_in_parent) {
        yaxis_team->rank = y_team_idx;
      }

      y_team_idx++;
    }
  }

  /* All good, assign the teams and return success */
  *xaxish = xaxis_team;
  *yaxish = yaxis_team;
  return 0;

cleanup:
  /* Clean up in case of error */
  if (xaxis_team != NULL) {
    free(xaxis_team);
  }
  if (yaxis_team != NULL) {
    free(yaxis_team);
  }

  *xaxish = SHMEM_TEAM_INVALID;
  *yaxish = SHMEM_TEAM_INVALID;
  return -1;
}

/*
 * teams that the library defined in advance cannot be destroyed.
 */

void shmemc_team_destroy(shmemc_team_h th) {
  if (th->parent != NULL) {
    size_t c;

    for (c = 0; c < th->nctxts; ++c) {
      if (!th->ctxts[c]->attr.privat) {
        shmemc_context_destroy(th->ctxts[c]);
      }
    }

    free(th);

    th = invalid;
  } else {
    shmemu_fatal("cannot destroy predefined team \"%s\"", th->name);
    /* NOT REACHED */
  }
}

int shmemc_team_sync(shmemc_team_h th) {
  /* Validate the team handle */
  if (th == NULL) {
    shmemu_warn("shmemc_team_sync: Invalid team handle (NULL)");
    return -1;
  }

  /* Iterate through all contexts within the team */
  for (size_t i = 0; i < th->nctxts; ++i) {
    shmemc_context_h ch = th->ctxts[i];

    /* Validate the context handle */
    if (ch == NULL) {
      shmemu_warn("shmemc_team_sync: Context at index %zu is NULL", i);
      continue; /* Skip to the next context */
    }

    /* Perform a fence operation to synchronize */
    ucs_status_t status = ucp_worker_fence(ch->w);
    if (status != UCS_OK) {
      shmemu_warn("shmemc_team_sync: ucp_worker_fence failed on context %zu "
                  "with status %s",
                  i, ucs_status_string(status));
      return -1;
    }
  }

  /* Successful synchronization across all contexts */
  return 0;
}

// /**
//  * @brief Implementation of team-relative pointer access
//  *
//  * @param th The team handle
//  * @param dest The symmetric address of the remotely accessible data object
//  * @param pe Team-relative PE number
//  * @return Pointer to the remote object or NULL if not accessible
//  */
// void *shmemc_team_ptr(shmemc_team_h th, const void *dest, int pe) {
//   int global_pe;

//   /* Validate the PE is within team range */
//   if (pe < 0 || pe >= th->nranks) {
//     return NULL;
//   }

//   /* Convert team-relative PE to global PE */
//   global_pe = shmemc_team_translate_pe(th, pe, &shmemc_team_world);
//   if (global_pe < 0) {
//     return NULL;
//   }

//   /* Call the original shmemc_ptr function with the global PE */
//   return shmemc_ptr(dest, global_pe);
// }
