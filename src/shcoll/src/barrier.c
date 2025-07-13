/**
 * @file barrier.c
 * @brief Implementation of OpenSHMEM barrier and sync collective operations
 *
 * This file implements various algorithms for barrier synchronization
 * including:
 * - Linear barrier
 * - Complete tree barrier
 * - Binomial tree barrier
 * - K-nomial tree barrier
 * - Dissemination barrier
 *
 * Each algorithm is implemented for both barrier and sync operations, and
 * includes variants for team-based and global (all PEs) synchronization.
 */

#include "shcoll.h"
#include "util/trees.h"

#include "shmem.h"
#include <math.h>

/** Default tree degree for tree-based barrier algorithms */
static int tree_degree_barrier = 2;

/** Default radix for k-nomial tree barrier algorithm */
static int knomial_tree_radix_barrier = 2;

/**
 * @brief Set the tree degree for tree-based barrier algorithms
 * @param tree_degree The tree degree to use
 */
void shcoll_set_tree_degree(int tree_degree) {
  tree_degree_barrier = tree_degree;
}

/**
 * @brief Set the radix for k-nomial tree barrier algorithm
 * @param tree_radix The tree radix to use
 */
void shcoll_set_knomial_tree_radix_barrier(int tree_radix) {
  knomial_tree_radix_barrier = tree_radix;
}

/**
 * @brief Helper function implementing linear barrier algorithm
 *
 * Uses a centralized approach where all PEs signal the root PE and wait for
 * acknowledgement.
 *
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void barrier_sync_helper_linear(int PE_start, int logPE_stride,
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

/**
 * @brief Helper function implementing complete tree barrier algorithm
 *
 * Uses a complete tree topology where each node has a fixed number of children.
 *
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void barrier_sync_helper_complete_tree(int PE_start,
                                                     int logPE_stride,
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

/**
 * @brief Helper function implementing binomial tree barrier algorithm
 *
 * Uses a binomial tree topology where nodes have varying numbers of children.
 *
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void barrier_sync_helper_binomial_tree(int PE_start,
                                                     int logPE_stride,
                                                     int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;

  /* Get my index in the active set */
  const int me_as = (me - PE_start) / stride;

  int i;
  long npokes;
  node_info_binomial_t node; /* TODO: try static */

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

/**
 * @brief Helper function implementing k-nomial tree barrier algorithm
 *
 * Uses a k-nomial tree topology with configurable radix.
 *
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void barrier_sync_helper_knomial_tree(int PE_start,
                                                    int logPE_stride,
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

/**
 * @brief Helper function implementing dissemination barrier algorithm
 *
 * Uses a dissemination pattern where each PE communicates with a sequence of
 * partners.
 *
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void barrier_sync_helper_dissemination(int PE_start,
                                                     int logPE_stride,
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

/**
 * @brief Macro to define barrier and sync functions for a given algorithm
 *
 * Defines four functions for each algorithm:
 * - barrier: Active set barrier with memory ordering
 * - barrier_all: Global barrier with memory ordering
 * - sync: Active set barrier without memory ordering
 * - sync_all: Global barrier without memory ordering
 *
 * @param _algo Algorithm name to generate functions for
 */
#define SHCOLL_BARRIER_SYNC_DEFINITION(_algo)                                  \
  void shcoll_barrier_##_algo(int PE_start, int logPE_stride, int PE_size,     \
                              long *pSync) {                                   \
    /* Sanity Checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_POSITIVE(PE_size, "PE_size");                                 \
    SHMEMU_CHECK_NON_NEGATIVE(PE_start, "PE_start");                           \
    SHMEMU_CHECK_NON_NEGATIVE(logPE_stride, "logPE_stride");                   \
    SHMEMU_CHECK_ACTIVE_SET_RANGE(PE_start, logPE_stride, PE_size);            \
    SHMEMU_CHECK_NULL(pSync, "pSync");                                         \
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_BARRIER_SYNC_SIZE);    \
    shmem_quiet();                                                             \
    barrier_sync_helper_##_algo(PE_start, logPE_stride, PE_size, pSync);       \
  }                                                                            \
                                                                               \
  void shcoll_barrier_all_##_algo(long *pSync) {                               \
    /* Sanity Checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_NULL(pSync, "pSync");                                         \
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_BARRIER_SYNC_SIZE);    \
    shmem_quiet();                                                             \
    barrier_sync_helper_##_algo(0, 0, shmem_n_pes(), pSync);                   \
  }                                                                            \
                                                                               \
  void shcoll_sync_##_algo(int PE_start, int logPE_stride, int PE_size,        \
                           long *pSync) {                                      \
    /* Sanity Checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_POSITIVE(PE_size, "PE_size");                                 \
    SHMEMU_CHECK_NON_NEGATIVE(PE_start, "PE_start");                           \
    SHMEMU_CHECK_NON_NEGATIVE(logPE_stride, "logPE_stride");                   \
    SHMEMU_CHECK_ACTIVE_SET_RANGE(PE_start, logPE_stride, PE_size);            \
    SHMEMU_CHECK_NULL(pSync, "pSync");                                         \
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_BARRIER_SYNC_SIZE);    \
    /* TODO: memory fence? */                                                   \
    barrier_sync_helper_##_algo(PE_start, logPE_stride, PE_size, pSync);       \
  }                                                                            \
                                                                               \
  void shcoll_sync_all_##_algo(long *pSync) {                                  \
    /* Sanity Checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_NULL(pSync, "pSync");                                         \
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_BARRIER_SYNC_SIZE);    \
    /* TODO: memory fence? */                                                   \
    barrier_sync_helper_##_algo(0, 0, shmem_n_pes(), pSync);                   \
  }

/* @formatter:off */

SHCOLL_BARRIER_SYNC_DEFINITION(linear)
SHCOLL_BARRIER_SYNC_DEFINITION(complete_tree)
SHCOLL_BARRIER_SYNC_DEFINITION(knomial_tree)
SHCOLL_BARRIER_SYNC_DEFINITION(binomial_tree)
SHCOLL_BARRIER_SYNC_DEFINITION(dissemination)

/* @formatter:on */

/**
 * @brief Macro to define team sync function for a given algorithm
 *
 * Defines a team-based synchronization function that allocates its own pSync
 * array.
 *
 * @param _algo Algorithm name to generate function for
 */
#define SHCOLL_TEAM_SYNC_DEFINITION(_algo)                                     \
  int shcoll_team_sync_##_algo(shmem_team_t team) {                            \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
    shmemc_team_h team_h = (shmemc_team_h)team;                                \
    SHMEMU_CHECK_TEAM_STRIDE(team_h->stride, __func__);                        \
    SHMEMU_CHECK_NULL(shmemc_team_get_psync(team_h, SHMEMC_PSYNC_BARRIER),     \
                      "team_h->pSyncs[BARRIER]");                              \
                                                                               \
    barrier_sync_helper_##_algo(                                               \
        team_h->start,                                                         \
        (team_h->stride > 0) ? (int)log2((double)team_h->stride) : 0,          \
        team_h->nranks, shmemc_team_get_psync(team_h, SHMEMC_PSYNC_BARRIER));  \
                                                                               \
    shmemc_team_reset_psync(team_h, SHMEMC_PSYNC_BARRIER);                     \
    return 0;                                                                  \
  }

SHCOLL_TEAM_SYNC_DEFINITION(linear)
SHCOLL_TEAM_SYNC_DEFINITION(complete_tree)
SHCOLL_TEAM_SYNC_DEFINITION(knomial_tree)
SHCOLL_TEAM_SYNC_DEFINITION(binomial_tree)
SHCOLL_TEAM_SYNC_DEFINITION(dissemination)
