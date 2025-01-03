/**
 * @file broadcast.c
 * @brief Implementation of broadcast collective operations
 *
 * This file contains implementations of broadcast collective operations using
 * different algorithms:
 * - Linear broadcast
 * - Complete tree broadcast
 * - Binomial tree broadcast
 * - K-nomial tree broadcast
 * - K-nomial tree with signal broadcast
 * - Scatter-collect broadcast
 */

/* For license: see LICENSE file at top-level */

#include "shcoll.h"
#include "shcoll/compat.h"
#include "util/trees.h"

#include <stdio.h>

/** Default tree degree for tree-based broadcasts */
static int tree_degree_broadcast = 2;

/** Default radix for k-nomial tree barrier */
static int knomial_tree_radix_barrier = 2;

/**
 * @brief Set the tree degree used in tree-based broadcast algorithms
 * @param tree_degree The tree degree to use
 */
void shcoll_set_broadcast_tree_degree(int tree_degree) {
  tree_degree_broadcast = tree_degree;
}

/**
 * @brief Set the radix used in k-nomial tree barrier broadcast
 * @param tree_radix The tree radix to use
 */
void shcoll_set_broadcast_knomial_tree_radix_barrier(int tree_radix) {
  knomial_tree_radix_barrier = tree_radix;
}

/**
 * @brief Helper function implementing linear broadcast algorithm
 *
 * @param team Team of PEs participating in broadcast
 * @param target Address of target on local PE
 * @param source Address of source on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE number
 * @return 0 on success, -1 on error
 */
inline static int broadcast_helper_linear(shmem_team_t team, void *target,
                                          const void *source, size_t nbytes,
                                          int PE_root) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);

  /* Validate parameters */
  if (team == SHMEM_TEAM_INVALID || !target || !source || PE_root < 0 ||
      PE_root >= npes) {
    return -1;
  }

  shmem_team_sync(team);
  if (me != PE_root) {
    shmem_getmem(target, source, nbytes, PE_root);
  }
  shmem_team_sync(team);

  return 0;
}

/**
 * @brief Helper function implementing complete tree broadcast algorithm
 *
 * @param team Team of PEs participating in broadcast
 * @param target Address of target on local PE
 * @param source Address of source on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE number
 * @return 0 on success, -1 on error
 */
inline static int broadcast_helper_complete_tree(shmem_team_t team,
                                                 void *target,
                                                 const void *source,
                                                 size_t nbytes, int PE_root) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);
  node_info_complete_t node;
  int child;
  int dst;

  /* Validate parameters */
  if (team == SHMEM_TEAM_INVALID || !target || !source || PE_root < 0 ||
      PE_root >= npes) {
    return -1;
  }

  /* Get my index in the active set */
  int me_as = me;

  /* Get information about children */
  get_node_info_complete(npes, tree_degree_broadcast, me_as, &node);

  /* Wait for the data from the parent */
  if (PE_root != me) {
    source = target;
  }

  /* Send data to children */
  if (node.children_num != 0) {
    for (child = node.children_begin; child != node.children_end;
         child = (child + 1) % npes) {
      dst = child;
      shmem_putmem_nbi(target, source, nbytes, dst);
    }
    shmem_fence();
  }

  shmem_team_sync(team);
  return 0;
}

/**
 * @brief Helper function implementing binomial tree broadcast algorithm
 *
 * @param team Team of PEs participating in broadcast
 * @param target Address of target on local PE
 * @param source Address of source on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE number
 * @return 0 on success, -1 on error
 */
inline static int broadcast_helper_binomial_tree(shmem_team_t team,
                                                 void *target,
                                                 const void *source,
                                                 size_t nbytes, int PE_root) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);
  node_info_binomial_t node;
  int i;
  int dst;

  /* Validate parameters */
  if (team == SHMEM_TEAM_INVALID || !target || !source || PE_root < 0 ||
      PE_root >= npes) {
    return -1;
  }

  /* Get my index in the active set */
  int me_as = me;

  /* Get information about children */
  get_node_info_binomial_root(npes, PE_root, me_as, &node);

  /* Wait for the data from the parent */
  if (me_as != PE_root) {
    source = target;
  }

  /* Send data to children */
  if (node.children_num != 0) {
    for (i = 0; i < node.children_num; i++) {
      dst = node.children[i];
      shmem_putmem_nbi(target, source, nbytes, dst);
    }
    shmem_fence();
  }

  shmem_team_sync(team);
  return 0;
}

/**
 * @brief Helper function implementing k-nomial tree broadcast algorithm
 *
 * @param team Team of PEs participating in broadcast
 * @param target Address of target on local PE
 * @param source Address of source on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE number
 * @return 0 on success, -1 on error
 */
inline static int broadcast_helper_knomial_tree(shmem_team_t team, void *target,
                                                const void *source,
                                                size_t nbytes, int PE_root) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);
  node_info_knomial_t node;
  int i, j;
  int child_offset;
  int dst_pe;

  /* Validate parameters */
  if (team == SHMEM_TEAM_INVALID || !target || !source || PE_root < 0 ||
      PE_root >= npes) {
    return -1;
  }

  /* Get my index in the active set */
  int me_as = me;

  /* Get information about children */
  get_node_info_knomial_root(npes, PE_root, knomial_tree_radix_barrier, me_as,
                             &node);

  /* Wait for the data from the parent */
  if (me_as != PE_root) {
    source = target;
  }

  /* Send data to children */
  if (node.children_num != 0) {
    child_offset = 0;

    for (i = 0; i < node.groups_num; i++) {
      for (j = 0; j < node.groups_sizes[i]; j++) {
        dst_pe = node.children[child_offset + j];
        shmem_putmem_nbi(target, source, nbytes, dst_pe);
      }
      shmem_fence();
      child_offset += node.groups_sizes[i];
    }
  }

  shmem_team_sync(team);
  return 0;
}

/**
 * @brief Helper function implementing k-nomial tree with signal broadcast
 * algorithm
 *
 * @param team Team of PEs participating in broadcast
 * @param target Address of target on local PE
 * @param source Address of source on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE number
 * @return 0 on success, -1 on error
 */
inline static int broadcast_helper_knomial_tree_signal(shmem_team_t team,
                                                       void *target,
                                                       const void *source,
                                                       size_t nbytes,
                                                       int PE_root) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);
  node_info_knomial_t node;
  int i, j;
  int child_offset;
  int dst_pe;

  /* Validate parameters */
  if (team == SHMEM_TEAM_INVALID || !target || !source || PE_root < 0 ||
      PE_root >= npes) {
    return -1;
  }

  /* Get my index in the active set */
  int me_as = me;

  /* Get information about children */
  get_node_info_knomial_root(npes, PE_root, knomial_tree_radix_barrier, me_as,
                             &node);

  /* Wait for the data from the parent */
  if (me_as != PE_root) {
    source = target;
  }

  /* Send data to children */
  if (node.children_num != 0) {
    child_offset = 0;

    for (i = 0; i < node.groups_num; i++) {
      for (j = 0; j < node.groups_sizes[i]; j++) {
        dst_pe = node.children[child_offset + j];
        shmem_putmem_nbi(target, source, nbytes, dst_pe);
      }
      shmem_fence();
      child_offset += node.groups_sizes[i];
    }
  }

  shmem_team_sync(team);
  return 0;
}

/**
 * @brief Helper function implementing scatter-collect broadcast algorithm
 *
 * @param team Team of PEs participating in broadcast
 * @param target Address of target on local PE
 * @param source Address of source on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE number
 * @return 0 on success, -1 on error
 */
inline static int broadcast_helper_scatter_collect(shmem_team_t team,
                                                   void *target,
                                                   const void *source,
                                                   size_t nbytes, int PE_root) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);
  int i;
  size_t block_size;
  size_t block_start;
  size_t block_end;

  /* Validate parameters */
  if (team == SHMEM_TEAM_INVALID || !target || !source || PE_root < 0 ||
      PE_root >= npes) {
    return -1;
  }

  /* Get my index in the active set */
  int me_as = me;

  /* Calculate block size and boundaries */
  block_size = (nbytes + npes - 1) / npes;
  block_start = me_as * block_size;
  block_end = (me_as == npes - 1) ? nbytes : (me_as + 1) * block_size;

  /* Scatter phase */
  if (me == PE_root) {
    for (i = 0; i < npes; i++) {
      size_t peer_start = i * block_size;
      size_t peer_end = (i == npes - 1) ? nbytes : (i + 1) * block_size;
      if (i != me) {
        shmem_putmem_nbi((char *)target + peer_start,
                         (const char *)source + peer_start,
                         peer_end - peer_start, i);
      }
    }
    shmem_fence();
  }

  shmem_team_sync(team);

  /* Collect phase */
  for (i = 0; i < npes; i++) {
    if (i != me) {
      size_t peer_start = me_as * block_size;
      size_t peer_end = (me_as == npes - 1) ? nbytes : (me_as + 1) * block_size;
      shmem_putmem_nbi((char *)target + peer_start,
                       (const char *)target + peer_start, peer_end - peer_start,
                       i);
    }
  }
  shmem_fence();

  shmem_team_sync(team);
  return 0;
}

/**
 * @brief Macro to define broadcast functions for different data types
 *
 * @param _algo Algorithm name
 * @param _type Data type
 * @param _typename Type name string
 */
#define SHCOLL_BROADCAST_DEFINITION(_algo, _type, _typename)                   \
  int shcoll_##_typename##_broadcast_##_algo(                                  \
      shmem_team_t team, _type *dest, const _type *source, size_t nelems) {   \
    return broadcast_helper_##_algo(team, dest, source,                        \
                                  sizeof(_type) * nelems, 0);                  \
  }

/* @formatter:off */
/**
 * @brief Macro to define broadcast functions for all supported data types
 *
 * @param _algo Algorithm name
 */
#define DEFINE_SHCOLL_BROADCAST_TYPES(_algo)                                   \
  SHCOLL_BROADCAST_DEFINITION(_algo, float, float)                             \
  SHCOLL_BROADCAST_DEFINITION(_algo, double, double)                           \
  SHCOLL_BROADCAST_DEFINITION(_algo, long double, longdouble)                  \
  SHCOLL_BROADCAST_DEFINITION(_algo, char, char)                               \
  SHCOLL_BROADCAST_DEFINITION(_algo, signed char, schar)                       \
  SHCOLL_BROADCAST_DEFINITION(_algo, short, short)                             \
  SHCOLL_BROADCAST_DEFINITION(_algo, int, int)                                 \
  SHCOLL_BROADCAST_DEFINITION(_algo, long, long)                               \
  SHCOLL_BROADCAST_DEFINITION(_algo, long long, longlong)                      \
  SHCOLL_BROADCAST_DEFINITION(_algo, unsigned char, uchar)                     \
  SHCOLL_BROADCAST_DEFINITION(_algo, unsigned short, ushort)                   \
  SHCOLL_BROADCAST_DEFINITION(_algo, unsigned int, uint)                       \
  SHCOLL_BROADCAST_DEFINITION(_algo, unsigned long, ulong)                     \
  SHCOLL_BROADCAST_DEFINITION(_algo, unsigned long long, ulonglong)            \
  SHCOLL_BROADCAST_DEFINITION(_algo, int8_t, int8)                             \
  SHCOLL_BROADCAST_DEFINITION(_algo, int16_t, int16)                           \
  SHCOLL_BROADCAST_DEFINITION(_algo, int32_t, int32)                           \
  SHCOLL_BROADCAST_DEFINITION(_algo, int64_t, int64)                           \
  SHCOLL_BROADCAST_DEFINITION(_algo, uint8_t, uint8)                           \
  SHCOLL_BROADCAST_DEFINITION(_algo, uint16_t, uint16)                         \
  SHCOLL_BROADCAST_DEFINITION(_algo, uint32_t, uint32)                         \
  SHCOLL_BROADCAST_DEFINITION(_algo, uint64_t, uint64)                         \
  SHCOLL_BROADCAST_DEFINITION(_algo, size_t, size)                             \
  SHCOLL_BROADCAST_DEFINITION(_algo, ptrdiff_t, ptrdiff)

/* Define implementations for all algorithms */
DEFINE_SHCOLL_BROADCAST_TYPES(linear)
DEFINE_SHCOLL_BROADCAST_TYPES(complete_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(binomial_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(knomial_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(knomial_tree_signal)
DEFINE_SHCOLL_BROADCAST_TYPES(scatter_collect)
/* @formatter:on */

/**
 * @brief Broadcast memory using simple linear algorithm
 *
 * @param team Team of PEs participating in broadcast
 * @param dest Address of destination on local PE
 * @param source Address of source on root PE
 * @param nelems Number of elements to broadcast
 * @param PE_root Root PE number
 * @return 0 on success, -1 on error
 */
int shcoll_broadcastmem(shmem_team_t team, void *dest, const void *source,
                        size_t nelems, int PE_root) {
  const int me = shmem_team_my_pe(team);
  const int npes = shmem_team_n_pes(team);

  if (me == PE_root) {
    for (int i = 0; i < npes; i++) {
      if (i != me) {
        shmem_putmem_nbi(dest, source, nelems, i);
      }
    }
    shmem_fence();
  }

  shmem_team_sync(team);
  return 0;
}
