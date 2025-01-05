/**
 * @file broadcast.c
 * @brief Implementation of broadcast collective communication routines for
 * OpenSHMEM
 * @author Srdan Milakovic, Michael Beebe
 *
 * This file contains implementations of various broadcast algorithms for
 * OpenSHMEM, including linear, complete tree, binomial tree, k-nomial tree, and
 * scatter-collect variants.
 */

#include "shcoll.h"
#include "shcoll/compat.h"
#include "shcoll/common.h"
#include "util/trees.h"

#include <stdio.h>
#include <string.h>

/** Default tree degree for broadcast operations */
static int tree_degree_broadcast = 2;

/** Default k-nomial tree radix for barrier operations */
static int knomial_tree_radix_barrier = 2;

/**
 * @brief Sets the tree degree used in broadcast operations
 * @param tree_degree The tree degree to use
 */
void shcoll_set_broadcast_tree_degree(int tree_degree) {
  tree_degree_broadcast = tree_degree;
}

/**
 * @brief Sets the k-nomial tree radix used in barrier operations during
 * broadcast
 * @param tree_radix The tree radix to use
 */
void shcoll_set_broadcast_knomial_tree_radix_barrier(int tree_radix) {
  knomial_tree_radix_barrier = tree_radix;
}

/**
 * @brief Linear broadcast helper that uses PE_root as source
 *
 * @param target Symmetric destination buffer on all PEs
 * @param source Source buffer on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE that broadcasts data
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void broadcast_helper_linear(void *target, const void *source,
                                           size_t nbytes, int PE_root,
                                           int PE_start, int logPE_stride,
                                           int PE_size, long *pSync) {
  const int stride = 1 << logPE_stride;
  const int root = (PE_root * stride) + PE_start;
  const int me = shmem_my_pe();

  shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);
  if (me != root) {
    shmem_getmem(target, source, nbytes, root);
  }
  shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);
}

/**
 * @brief Complete tree broadcast helper
 *
 * @param target Symmetric destination buffer on all PEs
 * @param source Source buffer on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE that broadcasts data
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void
broadcast_helper_complete_tree(void *target, const void *source, size_t nbytes,
                               int PE_root, int PE_start, int logPE_stride,
                               int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;
  int child;
  int dst;
  node_info_complete_t node;

  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;

  /* Get information about children */
  get_node_info_complete_root(PE_size, PE_root, tree_degree_broadcast, me_as,
                              &node);

  /* Wait for the data from the parent */
  if (PE_root != me) {
    shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    source = target;

    /* Send ack */
    shmem_long_atomic_inc(pSync, PE_start + node.parent * stride);
  }

  /* Send data to children */
  if (node.children_num != 0) {
    for (child = node.children_begin; child != node.children_end;
         child = (child + 1) % PE_size) {
      dst = PE_start + child * stride;
      shmem_putmem_nbi(target, source, nbytes, dst);
    }

    shmem_fence();

    for (child = node.children_begin; child != node.children_end;
         child = (child + 1) % PE_size) {
      dst = PE_start + child * stride;
      shmem_long_atomic_inc(pSync, dst);
    }

    shmem_long_wait_until(pSync, SHMEM_CMP_EQ,
                          SHCOLL_SYNC_VALUE + node.children_num +
                              (PE_root == me ? 0 : 1));
  }

  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);
}

/**
 * @brief Binomial tree broadcast helper
 *
 * @param target Symmetric destination buffer on all PEs
 * @param source Source buffer on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE that broadcasts data
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
// inline static void
// broadcast_helper_binomial_tree(void *target, const void *source, size_t
// nbytes,
//                                int PE_root, int PE_start, int logPE_stride,
//                                int PE_size, long *pSync) {
//   const int me = shmem_my_pe();
//   const int stride = 1 << logPE_stride;
//   int i;
//   int parent;
//   int dst;
//   node_info_binomial_t node;
//   /* Get my index in the active set */
//   int me_as = (me - PE_start) / stride;

//   /* Get information about children */
//   get_node_info_binomial_root(PE_size, PE_root, me_as, &node);

//   /* Wait for the data from the parent */
//   if (me_as != PE_root) {
//     shmem_long_wait_until(&pSync[0], SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
//     source = target;

//     /* Send ack */
//     parent = node.parent;
//     shmem_long_atomic_inc(&pSync[1], PE_start + parent * stride);
//   }

//   /* Send data to children */
//   if (node.children_num != 0) {
//     for (i = 0; i < node.children_num; i++) {
//       dst = PE_start + node.children[i] * stride;
//       shmem_putmem_nbi(target, source, nbytes, dst);
//       shmem_fence();
//       shmem_long_atomic_inc(&pSync[0], dst);
//     }

//     shmem_long_wait_until(&pSync[1], SHMEM_CMP_EQ,
//                           SHCOLL_SYNC_VALUE + node.children_num);
//   }

//   /* Reset both pSync elements */
//   shmem_long_p(&pSync[0], SHCOLL_SYNC_VALUE, me);
//   shmem_long_p(&pSync[1], SHCOLL_SYNC_VALUE, me);
// }

inline static void
broadcast_helper_binomial_tree(void *target, const void *source, size_t nbytes,
                               int PE_root, int PE_start, int logPE_stride,
                               int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;
  int i;
  int parent;
  int dst;
  node_info_binomial_t node;
  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;

  /* Get information about children */
  get_node_info_binomial_root(PE_size, PE_root, me_as, &node);

  /* Wait for the data from the parent */
  if (me_as != PE_root) {
    shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    source = target;

    /* Send ack */
    parent = node.parent;
    shmem_long_atomic_inc(pSync, PE_start + parent * stride);
  }

  /* Send data to children */
  if (node.children_num != 0) {
    for (i = 0; i < node.children_num; i++) {
      dst = PE_start + node.children[i] * stride;
      shmem_putmem_nbi(target, source, nbytes, dst);
      shmem_fence();
      shmem_long_atomic_inc(pSync, dst);
    }

    shmem_long_wait_until(pSync, SHMEM_CMP_EQ,
                          SHCOLL_SYNC_VALUE + node.children_num +
                              (me_as == PE_root ? 0 : 1));
  }

  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);
}

/**
 * @brief K-nomial tree broadcast helper
 *
 * @param target Symmetric destination buffer on all PEs
 * @param source Source buffer on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE that broadcasts data
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void broadcast_helper_knomial_tree(void *target,
                                                 const void *source,
                                                 size_t nbytes, int PE_root,
                                                 int PE_start, int logPE_stride,
                                                 int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;
  int i, j;
  int parent;
  int child_offset;
  int dst_pe;
  node_info_knomial_t node;
  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;

  /* Get information about children */
  get_node_info_knomial_root(PE_size, PE_root, knomial_tree_radix_barrier,
                             me_as, &node);

  /* Wait for the data from the parent */
  if (me_as != PE_root) {
    shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    source = target;

    /* Send ack */
    parent = node.parent;
    shmem_long_atomic_inc(pSync, PE_start + parent * stride);
  }

  /* Send data to children */
  if (node.children_num != 0) {
    child_offset = 0;

    for (i = 0; i < node.groups_num; i++) {
      for (j = 0; j < node.groups_sizes[i]; j++) {
        dst_pe = PE_start + node.children[child_offset + j] * stride;
        shmem_putmem_nbi(target, source, nbytes, dst_pe);
      }

      shmem_fence();

      for (j = 0; j < node.groups_sizes[i]; j++) {
        dst_pe = PE_start + node.children[child_offset + j] * stride;
        shmem_long_atomic_inc(pSync, dst_pe);
      }

      child_offset += node.groups_sizes[i];
    }

    shmem_long_wait_until(pSync, SHMEM_CMP_EQ,
                          SHCOLL_SYNC_VALUE + node.children_num +
                              (me_as == PE_root ? 0 : 1));
  }

  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);
}

/**
 * @brief K-nomial tree broadcast helper using signal operations
 *
 * @param target Symmetric destination buffer on all PEs
 * @param source Source buffer on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE that broadcasts data
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void broadcast_helper_knomial_tree_signal(
    void *target, const void *source, size_t nbytes, int PE_root, int PE_start,
    int logPE_stride, int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;
  int i, j;
  int parent;
  int child_offset;
  int dest_pe;
  node_info_knomial_t node;
  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;

  /* Get information about children */
  get_node_info_knomial_root(PE_size, PE_root, knomial_tree_radix_barrier,
                             me_as, &node);

  /* Wait for the data from the parent */
  if (me_as != PE_root) {
    shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
    source = target;

    /* Send ack */
    parent = node.parent;
    shmem_long_atomic_inc(pSync, PE_start + parent * stride);
  }

  /* Send data to children */
  if (node.children_num != 0) {
    child_offset = 0;

    for (i = 0; i < node.groups_num; i++) {
      for (j = 0; j < node.groups_sizes[i]; j++) {
        dest_pe = PE_start + node.children[child_offset + j] * stride;

        shmem_putmem_signal_nb(target, source, nbytes, (uint64_t *)pSync,
                               SHCOLL_SYNC_VALUE + 1, dest_pe, NULL);
      }

      child_offset += node.groups_sizes[i];
    }

    shmem_long_wait_until(pSync, SHMEM_CMP_EQ,
                          SHCOLL_SYNC_VALUE + node.children_num +
                              (me_as == PE_root ? 0 : 1));
  }

  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);
}

/**
 * @brief Scatter-collect broadcast helper
 *
 * @param target Symmetric destination buffer on all PEs
 * @param source Source buffer on root PE
 * @param nbytes Number of bytes to broadcast
 * @param PE_root Root PE that broadcasts data
 * @param PE_start First PE in the active set
 * @param logPE_stride Log2 of stride between consecutive PEs
 * @param PE_size Number of PEs in the active set
 * @param pSync Symmetric work array
 */
inline static void
broadcast_helper_scatter_collect(void *target, const void *source,
                                 size_t nbytes, int PE_root, int PE_start,
                                 int logPE_stride, int PE_size, long *pSync) {
  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;
  const int root = PE_start + PE_root * stride;
  const size_t block_size = nbytes / PE_size;
  const size_t last_block_size = nbytes - (PE_size - 1) * block_size;
  int i;

  /* Scatter */
  if (me == root) {
    for (i = 0; i < PE_size; i++) {
      const int dst = PE_start + i * stride;
      const size_t curr_block_size =
          (i == PE_size - 1) ? last_block_size : block_size;
      if (dst != me) {
        shmem_putmem_nbi((char *)target + i * block_size,
                         (const char *)source + i * block_size, curr_block_size,
                         dst);
      } else {
        memcpy((char *)target + i * block_size,
               (const char *)source + i * block_size, curr_block_size);
      }
    }
    shmem_fence();
    shmem_long_atomic_inc(pSync, root);
  }

  /* Wait for scatter to complete */
  shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
  shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);

  /* Collect */
  for (i = 0; i < PE_size; i++) {
    const int src = PE_start + i * stride;
    const size_t curr_block_size =
        (i == PE_size - 1) ? last_block_size : block_size;
    if (src != me) {
      shmem_getmem_nbi((char *)target + i * block_size,
                       (const char *)target + i * block_size, curr_block_size,
                       src);
    }
  }
}

/**
 * @brief Macro for sized broadcast implementations using legacy helpers
 */
#define SHCOLL_BROADCAST_SIZE_DEFINITION(_algo, _size)                         \
  void shcoll_broadcast##_size##_##_algo(                                      \
      void *dest, const void *source, size_t nelems, int PE_root,              \
      int PE_start, int logPE_stride, int PE_size, long *pSync) {              \
    broadcast_helper_##_algo(dest, source, (_size / CHAR_BIT) * nelems,        \
                             PE_root, PE_start, logPE_stride, PE_size, pSync); \
  }

/* Generate sized implementations for all algorithms */
/* Linear */
SHCOLL_BROADCAST_SIZE_DEFINITION(linear, 8)
SHCOLL_BROADCAST_SIZE_DEFINITION(linear, 16)
SHCOLL_BROADCAST_SIZE_DEFINITION(linear, 32)
SHCOLL_BROADCAST_SIZE_DEFINITION(linear, 64)

/* Complete tree */
SHCOLL_BROADCAST_SIZE_DEFINITION(complete_tree, 8)
SHCOLL_BROADCAST_SIZE_DEFINITION(complete_tree, 16)
SHCOLL_BROADCAST_SIZE_DEFINITION(complete_tree, 32)
SHCOLL_BROADCAST_SIZE_DEFINITION(complete_tree, 64)

/* Binomial tree */
SHCOLL_BROADCAST_SIZE_DEFINITION(binomial_tree, 8)
SHCOLL_BROADCAST_SIZE_DEFINITION(binomial_tree, 16)
SHCOLL_BROADCAST_SIZE_DEFINITION(binomial_tree, 32)
SHCOLL_BROADCAST_SIZE_DEFINITION(binomial_tree, 64)

/* K-nomial tree */
SHCOLL_BROADCAST_SIZE_DEFINITION(knomial_tree, 8)
SHCOLL_BROADCAST_SIZE_DEFINITION(knomial_tree, 16)
SHCOLL_BROADCAST_SIZE_DEFINITION(knomial_tree, 32)
SHCOLL_BROADCAST_SIZE_DEFINITION(knomial_tree, 64)

/* K-nomial tree with signal */
SHCOLL_BROADCAST_SIZE_DEFINITION(knomial_tree_signal, 8)
SHCOLL_BROADCAST_SIZE_DEFINITION(knomial_tree_signal, 16)
SHCOLL_BROADCAST_SIZE_DEFINITION(knomial_tree_signal, 32)
SHCOLL_BROADCAST_SIZE_DEFINITION(knomial_tree_signal, 64)

/* Scatter-collect */
SHCOLL_BROADCAST_SIZE_DEFINITION(scatter_collect, 8)
SHCOLL_BROADCAST_SIZE_DEFINITION(scatter_collect, 16)
SHCOLL_BROADCAST_SIZE_DEFINITION(scatter_collect, 32)
SHCOLL_BROADCAST_SIZE_DEFINITION(scatter_collect, 64)

/**
 * @brief Macro for typed broadcast implementations using legacy helpers
 FIXME: this isn't working
 */
#define SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, _type, _typename)              \
  int shcoll_##_typename##_broadcast_##_algo(shmem_team_t team, _type *dest,   \
                                             const _type *source,              \
                                             size_t nelems, int PE_root) {     \
    int PE_start = shmem_team_translate_pe(team, 0, SHMEM_TEAM_WORLD);         \
    int logPE_stride = 0;                                                      \
    int PE_size = shmem_team_n_pes(team);                                      \
    /* Allocate pSync from symmetric heap */                                   \
    long *pSync = shmem_malloc(SHCOLL_BCAST_SYNC_SIZE * sizeof(long));         \
    if (!pSync)                                                                \
      return -1;                                                               \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_BCAST_SYNC_SIZE; i++) {                         \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    /* Ensure all PEs have initialized pSync */                                \
    shmem_team_sync(team);                                                     \
    /* Perform broadcast */                                                    \
    broadcast_helper_##_algo(dest, source, sizeof(_type) * nelems, PE_root,    \
                             PE_start, logPE_stride, PE_size, pSync);          \
    /* Cleanup */                                                              \
    shmem_team_sync(team);                                                     \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

/**
 * @brief Macro to define broadcast implementations for all types
 */
#define DEFINE_SHCOLL_BROADCAST_TYPES(_algo)                                   \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, float, float)                          \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, double, double)                        \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, long double, longdouble)               \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, char, char)                            \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, signed char, schar)                    \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, short, short)                          \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, int, int)                              \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, long, long)                            \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, long long, longlong)                   \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, unsigned char, uchar)                  \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, unsigned short, ushort)                \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, unsigned int, uint)                    \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, unsigned long, ulong)                  \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, unsigned long long, ulonglong)         \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, int8_t, int8)                          \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, int16_t, int16)                        \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, int32_t, int32)                        \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, int64_t, int64)                        \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, uint8_t, uint8)                        \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, uint16_t, uint16)                      \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, uint32_t, uint32)                      \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, uint64_t, uint64)                      \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, size_t, size)                          \
  SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, ptrdiff_t, ptrdiff)

#define SHCOLL_BROADCAST_TYPE_FOR_TYPE(_algo, _type, _typename)                \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, _type, _typename)

/* Generate typed implementations for all algorithms */
DEFINE_SHCOLL_BROADCAST_TYPES(linear)
DEFINE_SHCOLL_BROADCAST_TYPES(complete_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(binomial_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(knomial_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(knomial_tree_signal)
DEFINE_SHCOLL_BROADCAST_TYPES(scatter_collect)
