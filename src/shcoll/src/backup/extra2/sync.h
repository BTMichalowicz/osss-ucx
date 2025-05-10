#ifndef _SHCOLL_SYNC_H
#define _SHCOLL_SYNC_H 1

#include "shmem/teams.h"

/**
 * @brief Sets the degree (number of children) for tree-based synchronization
 * algorithms
 * @param tree_degree The degree of the tree (must be > 0)
 */
void shcoll_set_tree_degree(int tree_degree);

/**
 * @brief Sets the radix for k-nomial tree barrier synchronization
 * @param tree_radix The radix value for the k-nomial tree (must be > 0)
 */
void shcoll_set_knomial_tree_radix_barrier(int tree_radix);

/**
 * @brief Macro to declare synchronization functions for different algorithms
 * @param _name The algorithm name to create declarations for
 *
 * For each algorithm, declares:
 * - shcoll_sync_<name>: Team-based synchronization
 * - shcoll_sync_all_<name>: Global synchronization using pSync array
 */
#define SCHOLL_SYNC_DECLARATION(_name)                                         \
  int shcoll_sync_##_name(shmem_team_t team);                                  \
                                                                               \
  void shcoll_sync_all_##_name(long *pSync);

/** @name Synchronization Algorithm Declarations
 * Different synchronization algorithm implementations
 * @{
 */
SCHOLL_SYNC_DECLARATION(linear)        /**< Linear synchronization */
SCHOLL_SYNC_DECLARATION(complete_tree) /**< Complete tree synchronization */
SCHOLL_SYNC_DECLARATION(binomial_tree) /**< Binomial tree synchronization */
SCHOLL_SYNC_DECLARATION(knomial_tree)  /**< K-nomial tree synchronization */
SCHOLL_SYNC_DECLARATION(dissemination) /**< Dissemination synchronization */
/** @} */

#endif /* ! _SHCOLL_SYNC_H */
