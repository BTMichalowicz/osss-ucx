#include "shcoll.h"
#include "util/trees.h"
#include "shmem.h"

#include "shmem/teams.h"

static int tree_degree_barrier = 2;
static int knomial_tree_radix_barrier = 2;

void shcoll_set_tree_degree(int tree_degree) {
  tree_degree_barrier = tree_degree;
}

void shcoll_set_knomial_tree_radix_barrier(int tree_radix) {
  knomial_tree_radix_barrier = tree_radix;
}

//
// TODO: add error checking to the team-based helpers
//

////////////////////////////////////////////////////////////////////////////////////
/*
 * Linear sync implementation
 */
inline static void sync_helper_linear(int PE_start, int logPE_stride,
                                      int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;
  int i;
  int pe;

  if (PE_start == me) {
    /* wait for the rest of the AS to poke me */
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE + PE_size - 1);
    shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE);

    /* send acks out */
    pe = PE_start + stride;
    for (i = 1; i < PE_size; ++i) {
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE + 1, pe);
      pe += stride;
    }
  } else {
    /* poke root */
    shmem_long_atomic_inc(pSync, PE_start);

    /* get ack */
    shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE);
  }
}

/*
 * Linear sync implementation using team-based logic
 */
inline static int sync_team_helper_linear(shmem_team_t team) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);

  if (me == 0) {
    /* Root PE waits for all other PEs to signal completion */
    for (int i = 1; i < npes; ++i) {
      shmem_long_wait_until(&team->pSync[i], SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE);
    }
    /* Root PE signals completion */
    for (int i = 1; i < npes; ++i) {
      shmem_long_p(&team->pSync[i], SHCOLL_SYNC_VALUE, i);
    }
  } else {
    /* Non-root PEs signal completion to root */
    shmem_long_p(&team->pSync[me], SHCOLL_SYNC_VALUE, 0);
    /* Non-root PEs wait for root to signal completion */
    shmem_long_wait_until(&team->pSync[me], SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE);
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
/*
 * Complete tree sync implementation
 */
inline static void sync_helper_complete_tree(int PE_start, int logPE_stride,
                                             int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;

  int child;
  long npokes;
  node_info_complete_t node;

  /* Get my index in the active set */
  const int me_as = (me - PE_start) / stride;

  /* Get node info */
  get_node_info_complete(PE_size, tree_degree_barrier, me_as, &node);

  /* Wait for pokes from the children */
  npokes = node.children_num;
  if (npokes != 0) {
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE + npokes);
  }

  if (node.parent != -1) {
    /* Poke the parent exists */
    shmem_long_atomic_inc(pSync, PE_start + node.parent * stride);

    /* Wait for the poke from parent */
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE + npokes + 1);
  }

  /* Clear pSync and poke the children */
  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);

  for (child = node.children_begin; child != node.children_end; child++) {
    shmem_long_atomic_inc(pSync, PE_start + child * stride);
  }
}

/*
 * Complete tree sync implementation using team-based logic
 */
inline static int sync_team_helper_complete_tree(shmem_team_t team) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);

  node_info_complete_t node;
  long npokes;

  /* Get node info for the current PE */
  get_node_info_complete(npes, tree_degree_barrier, me, &node);

  /* Wait for pokes from the children */
  npokes = node.children_num;
  if (npokes != 0) {
    for (int i = 0; i < npokes; ++i) {
      shmem_long_wait_until(&team->pSync[node.children[i]], SHMEM_CMP_EQ,
                            SHCOLL_SYNC_VALUE);
    }
  }

  if (node.parent != -1) {
    /* Poke the parent */
    shmem_long_atomic_inc(&team->pSync[node.parent], me);

    /* Wait for the poke from parent */
    shmem_long_wait_until(&team->pSync[me], SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE);
  }

  /* Clear pSync and poke the children */
  for (int i = 0; i < npokes; ++i) {
    shmem_long_atomic_inc(&team->pSync[node.children[i]], me);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
/*
 * Binomial tree sync implementation
 */
inline static void sync_helper_binomial_tree(int PE_start, int logPE_stride,
                                             int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;

  /* Get my index in the active set */
  const int me_as = (me - PE_start) / stride;

  int i;
  long npokes;
  node_info_binomial_t node;

  /* Get node info */
  get_node_info_binomial(PE_size, me_as, &node);

  /* Wait for pokes from the children */
  npokes = node.children_num;
  if (npokes != 0) {
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE + npokes);
  }

  if (node.parent != -1) {
    /* Poke the parent */
    shmem_long_atomic_inc(pSync, PE_start + node.parent * stride);

    /* Wait for the poke from parent */
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE + npokes + 1);
  }

  /* Clear pSync and poke the children */
  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);

  for (i = 0; i < node.children_num; i++) {
    shmem_long_atomic_inc(pSync, PE_start + node.children[i] * stride);
  }
}

/*
 * Binomial tree sync implementation using team-based logic
 */
inline static int sync_team_helper_binomial_tree(shmem_team_t team) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);

  node_info_binomial_t node;
  long npokes;

  /* Get node info for the current PE */
  get_node_info_binomial(npes, me, &node);

  /* Wait for pokes from the children */
  npokes = node.children_num;
  if (npokes != 0) {
    for (int i = 0; i < npokes; ++i) {
      shmem_long_wait_until(&team->pSync[node.children[i]], SHMEM_CMP_EQ,
                            SHCOLL_SYNC_VALUE);
    }
  }

  if (node.parent != -1) {
    /* Poke the parent */
    shmem_long_atomic_inc(&team->pSync[node.parent], me);

    /* Wait for the poke from parent */
    shmem_long_wait_until(&team->pSync[me], SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE);
  }

  /* Clear pSync and poke the children */
  for (int i = 0; i < npokes; ++i) {
    shmem_long_atomic_inc(&team->pSync[node.children[i]], me);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
/*
 * Knomial tree sync implementation
 */
inline static void sync_helper_knomial_tree(int PE_start, int logPE_stride,
                                            int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;

  /* Get my index in the active set */
  const int me_as = (me - PE_start) / stride;

  int i;
  long npokes;
  node_info_knomial_t node;

  /* Get node info */
  get_node_info_knomial(PE_size, knomial_tree_radix_barrier, me_as, &node);

  /* Wait for pokes from the children */
  npokes = node.children_num;
  if (npokes != 0) {
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE + npokes);
  }

  if (node.parent != -1) {
    /* Poke the parent */
    shmem_long_atomic_inc(pSync, PE_start + node.parent * stride);

    /* Wait for the poke from parent */
    shmem_long_wait_until(pSync, SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE + npokes + 1);
  }

  /* Clear pSync and poke the children */
  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);

  for (i = 0; i < node.children_num; i++) {
    shmem_long_atomic_inc(pSync, PE_start + node.children[i] * stride);
  }
}

/*
 * Knomial tree sync implementation using team-based logic
 */
inline static int sync_team_helper_knomial_tree(shmem_team_t team) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);

  node_info_knomial_t node;
  long npokes;

  /* Get node info for the current PE */
  get_node_info_knomial(npes, knomial_tree_radix_barrier, me, &node);

  /* Wait for pokes from the children */
  npokes = node.children_num;
  if (npokes != 0) {
    for (int i = 0; i < npokes; ++i) {
      shmem_long_wait_until(&team->pSync[node.children[i]], SHMEM_CMP_EQ,
                            SHCOLL_SYNC_VALUE);
    }
  }

  if (node.parent != -1) {
    /* Poke the parent */
    shmem_long_atomic_inc(&team->pSync[node.parent], me);

    /* Wait for the poke from parent */
    shmem_long_wait_until(&team->pSync[me], SHMEM_CMP_EQ, SHCOLL_SYNC_VALUE);
  }

  /* Clear pSync and poke the children */
  for (int i = 0; i < npokes; ++i) {
    shmem_long_atomic_inc(&team->pSync[node.children[i]], me);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
/*
 * Dissemination sync implementation
 */
inline static void sync_helper_dissemination(int PE_start, int logPE_stride,
                                             int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;
  /* Calculate my index in the active set */
  const int me_as = (me - PE_start) / stride;
  int round;
  int distance;
  int target_as;
  long unused;

  for (round = 0, distance = 1; distance < PE_size; round++, distance <<= 1) {
    target_as = (me_as + distance) % PE_size;

    /* Poke the target for the current round */
    shmem_long_atomic_inc(&pSync[round], PE_start + target_as * stride);

    /* Wait until poked in this round */
    shmem_long_wait_until(&pSync[round], SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);

    /* Reset pSync element, fadd is used instead of add because we have to
       be sure that reset happens before next invocation of barrier */
    unused = shmem_long_atomic_fetch_add(&pSync[round], -1, me);
  }
}

/*
 * Dissemination sync implementation using team-based logic
 */
inline static int sync_team_helper_dissemination(shmem_team_t team) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);

  int round;
  int distance;
  int target;
  long unused;

  for (round = 0, distance = 1; distance < npes; round++, distance <<= 1) {
    target = (me + distance) % npes;

    /* Poke the target for the current round */
    shmem_long_atomic_inc(&team->pSync[round], target);

    /* Wait until poked in this round */
    shmem_long_wait_until(&team->pSync[round], SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);

    /* Reset pSync element */
    unused = shmem_long_atomic_fetch_add(&team->pSync[round], -1, me);
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
// FIXME: figure out how to deal with the psync array. between these two
//        helpers
#define SHCOLL_SYNC_DEFINITION(_name)                                          \
  int shcoll_sync_##_name(shmem_team_t team) {                                 \
    int me = shmem_team_my_pe(team);                                           \
    int npes = shmem_team_n_pes(team);                                         \
    if (me < 0 || npes <= 0) {                                                 \
      return 1; /* Invalid team */                                             \
    }                                                                          \
    return sync_team_helper_##_name(team);                                     \
  }                                                                            \
                                                                               \
  void shcoll_sync_all_##_name(long *pSync) {                                  \
    sync_helper_##_name(0, 0, shmem_n_pes(), pSync);                           \
  }

/* @formatter:off */

SHCOLL_SYNC_DEFINITION(linear)
SHCOLL_SYNC_DEFINITION(complete_tree)
SHCOLL_SYNC_DEFINITION(knomial_tree)
SHCOLL_SYNC_DEFINITION(binomial_tree)
SHCOLL_SYNC_DEFINITION(dissemination)

/* @formatter:on */
