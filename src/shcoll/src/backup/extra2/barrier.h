#ifndef _SHCOLL_BARRIER_H
#define _SHCOLL_BARRIER_H 1

/**
 * @brief Sets the degree (number of children) for tree-based barrier algorithms
 * @param tree_degree The degree of the tree (must be > 0)
 */
void shcoll_set_tree_degree(int tree_degree);

/**
 * @brief Sets the radix for k-nomial tree barrier algorithms
 * @param tree_radix The radix value for the k-nomial tree (must be > 0)
 */
void shcoll_set_knomial_tree_radix_barrier(int tree_radix);

/**
 * @brief Macro to declare barrier functions for different algorithms
 * @param _name The algorithm name to create declarations for
 *
 * For each algorithm, declares:
 * - shcoll_barrier_<name>: Barrier using PE_start/logPE_stride/PE_size
 * - shcoll_barrier_all_<name>: Global barrier using pSync array
 */
#define SHCOLL_BARRIER_DECLARATION(_name)                                      \
  void shcoll_barrier_##_name(int PE_start, int logPE_stride, int PE_size,     \
                              long *pSync);                                    \
                                                                               \
  void shcoll_barrier_all_##_name(long *pSync);

/** @name Barrier Algorithm Declarations
 * Different barrier algorithm implementations
 * @{
 */
SHCOLL_BARRIER_DECLARATION(linear)        /**< Linear barrier */
SHCOLL_BARRIER_DECLARATION(complete_tree) /**< Complete tree barrier */
SHCOLL_BARRIER_DECLARATION(binomial_tree) /**< Binomial tree barrier */
SHCOLL_BARRIER_DECLARATION(knomial_tree)  /**< K-nomial tree barrier */
SHCOLL_BARRIER_DECLARATION(dissemination) /**< Dissemination barrier */
/** @} */

#endif /* ! _SHCOLL_BARRIER_H */
