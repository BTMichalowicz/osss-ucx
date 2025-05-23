/**
 * @file barrier.c
 * @brief Implementation of various barrier algorithms for OpenSHMEM
 *
 * This file contains implementations of different barrier algorithms including:
 * - Linear barrier
 * - Complete tree barrier
 * - Binomial tree barrier
 * - K-nomial tree barrier
 * - Dissemination barrier
 */

#include "shcoll.h"
#include "util/trees.h"
#include "shmem.h"

static int tree_degree_barrier = 2;
static int knomial_tree_radix_barrier = 2;

/**
 * @brief Sets the degree (number of children) for tree-based barrier algorithms
 * @param tree_degree The degree of the tree (must be > 0)
 */
void shcoll_set_tree_degree(int tree_degree) {
  tree_degree_barrier = tree_degree;
}

/**
 * @brief Sets the radix for k-nomial tree barrier algorithms
 * @param tree_radix The radix value for the k-nomial tree (must be > 0)
 */
void shcoll_set_knomial_tree_radix_barrier(int tree_radix) {
  knomial_tree_radix_barrier = tree_radix;
}

/**
 * @brief Linear barrier implementation
 *
 * Uses a centralized approach where all PEs synchronize through a root PE.
 *
 * @param PE_start Starting PE number
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs participating in barrier
 * @param pSync Symmetric work array
 */
inline static void barrier_helper_linear(int PE_start, int logPE_stride,
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
 * @brief Complete tree barrier implementation
 *
 * Uses a complete k-ary tree topology where k is specified by
 * tree_degree_barrier.
 *
 * @param PE_start Starting PE number
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs participating in barrier
 * @param pSync Symmetric work array
 */
inline static void barrier_helper_complete_tree(int PE_start, int logPE_stride,
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
 * @brief Binomial tree barrier implementation
 *
 * Uses a binomial tree topology for synchronization.
 *
 * @param PE_start Starting PE number
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs participating in barrier
 * @param pSync Symmetric work array
 */
inline static void barrier_helper_binomial_tree(int PE_start, int logPE_stride,
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
 * @brief K-nomial tree barrier implementation
 *
 * Uses a k-nomial tree topology where k is specified by
 * knomial_tree_radix_barrier.
 *
 * @param PE_start Starting PE number
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs participating in barrier
 * @param pSync Symmetric work array
 */
inline static void barrier_helper_knomial_tree(int PE_start, int logPE_stride,
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
 * @brief Dissemination barrier implementation
 *
 * Uses a dissemination pattern where each PE communicates with increasingly
 * distant partners.
 *
 * @param PE_start Starting PE number
 * @param logPE_stride Log (base 2) of stride between consecutive PEs
 * @param PE_size Number of PEs participating in barrier
 * @param pSync Symmetric work array
 */
inline static void barrier_helper_dissemination(int PE_start, int logPE_stride,
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
 * @brief Macro to define barrier functions for different algorithms
 *
 * For each algorithm, defines:
 * - shcoll_barrier_<name>: Barrier using PE_start/logPE_stride/PE_size
 * - shcoll_barrier_all_<name>: Global barrier using pSync array
 *
 * @param _name The algorithm name to create definitions for
 */
#define SHCOLL_BARRIER_DEFINITION(_name)                                       \
  void shcoll_barrier_##_name(int PE_start, int logPE_stride, int PE_size,     \
                              long *pSync) {                                   \
    shmem_quiet();                                                             \
    barrier_helper_##_name(PE_start, logPE_stride, PE_size, pSync);            \
  }                                                                            \
                                                                               \
  void shcoll_barrier_all_##_name(long *pSync) {                               \
    shmem_quiet();                                                             \
    barrier_helper_##_name(0, 0, shmem_n_pes(), pSync);                        \
  }

/* @formatter:off */

SHCOLL_BARRIER_DEFINITION(linear)
SHCOLL_BARRIER_DEFINITION(complete_tree)
SHCOLL_BARRIER_DEFINITION(knomial_tree)
SHCOLL_BARRIER_DEFINITION(binomial_tree)
SHCOLL_BARRIER_DEFINITION(dissemination)

/* @formatter:on */
