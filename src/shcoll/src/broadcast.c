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

  /* Wait for the data form the parent */
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

  /* Wait for the data form the parent */
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

  /* Wait for the data form the parent */
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

  /* Wait for the data form the parent */
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
  /* TODO: Optimize cases where data_start == data_end (block has size 0) */

  const int me = shmem_my_pe();
  const int stride = 1 << logPE_stride;
  int root_as = (PE_root - PE_start) / stride;
  /* Get my index in the active set */
  int me_as = (me - PE_start) / stride;

  /* Shift me_as so that me_as for PE_root is 0 */
  me_as = (me_as - root_as + PE_size) % PE_size;

  /* The number of received blocks (scatter + collect) */
  int total_received = me_as == 0 ? PE_size : 0;

  int target_pe;
  int next_as = (me_as + 1) % PE_size;
  int next_pe = PE_start + (root_as + next_as) % PE_size * stride;

  /* The index of the block that should be send to next_pe */
  int next_block = me_as;

  /* The number of blocks that next received */
  int next_pe_nblocks = next_as == 0 ? PE_size : 0;

  int left = 0;
  int right = PE_size;
  int mid;
  int dist;

  size_t data_start;
  size_t data_end;

  /* Used in the collect part to wait for new blocks */
  long ring_received = SHCOLL_SYNC_VALUE;

  if (me_as != 0) {
    source = target;
  }

  /* Scatter data to other PEs using binomial tree */
  while (right - left > 1) {
    /* dist = ceil((right - let) / 2) */
    dist = ((right - left) >> 1) + ((right - left) & 0x1);
    mid = left + dist;

    /* Send (right - mid) elements starting with mid to pe + dist */
    if (me_as == left && me_as + dist < right) {
      /* TODO: possible overflow */
      data_start = (mid * nbytes + PE_size - 1) / PE_size;
      data_end = (right * nbytes + PE_size - 1) / PE_size;
      target_pe = PE_start + (root_as + me_as + dist) % PE_size * stride;

      shmem_putmem_nbi((char *)target + data_start, (char *)source + data_start,
                       data_end - data_start, target_pe);
      shmem_fence();
      shmem_long_atomic_inc(pSync, target_pe);
    }

    /* Send (right - mid) elements starting with mid from (me_as - dist) */
    if (me_as - dist == left) {
      shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);
      total_received = right - mid;
    }

    if (next_as - dist == left) {
      next_pe_nblocks = right - mid;
    }

    if (me_as < mid) {
      right = mid;
    } else {
      left = mid;
    }
  }

  /* Do collect using (modified) ring algorithm */
  while (next_pe_nblocks != PE_size) {
    data_start = (next_block * nbytes + PE_size - 1) / PE_size;
    data_end = ((next_block + 1) * nbytes + PE_size - 1) / PE_size;

    shmem_putmem_nbi((char *)target + data_start, (char *)source + data_start,
                     data_end - data_start, next_pe);
    shmem_fence();
    shmem_long_atomic_inc(pSync + 1, next_pe);

    next_pe_nblocks++;
    next_block = (next_block - 1 + PE_size) % PE_size;

    /*
     * If we did not receive all blocks, we must wait for the next
     * block we want to send
     */
    if (total_received != PE_size) {
      shmem_long_wait_until(pSync + 1, SHMEM_CMP_GT, ring_received);
      ring_received++;
      total_received++;
    }
  }

  while (total_received != PE_size) {
    shmem_long_wait_until(pSync + 1, SHMEM_CMP_GT, ring_received);
    ring_received++;
    total_received++;
  }

  /* TODO: maybe only one pSync is enough */
  shmem_long_p(pSync + 0, SHCOLL_SYNC_VALUE, me);
  shmem_long_p(pSync + 1, SHCOLL_SYNC_VALUE, me);
}

/**
 * @brief Macro for sized broadcast implementations using legacy helpers
 */
#define SHCOLL_BROADCAST_SIZE_DEFINITION(_algo, _size)                         \
  void shcoll_broadcast##_size##_##_algo(                                      \
      void *dest, const void *source, size_t nelems, int PE_root,              \
      int PE_start, int logPE_stride, int PE_size, long *pSync) {              \
    /* Sanity checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_POSITIVE(PE_size, "PE_size");                                 \
    SHMEMU_CHECK_NON_NEGATIVE(PE_start, "PE_start");                           \
    SHMEMU_CHECK_NON_NEGATIVE(logPE_stride, "logPE_stride");                   \
    SHMEMU_CHECK_ACTIVE_SET_RANGE(PE_start, logPE_stride, PE_size);            \
    SHMEMU_CHECK_SYMMETRIC(dest, (_size) / (CHAR_BIT) * nelems * PE_size);     \
    SHMEMU_CHECK_SYMMETRIC(source, (_size) / (CHAR_BIT) * nelems * PE_size);   \
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_ALLTOALL_SYNC_SIZE);   \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source,                                  \
                                (_size) / (CHAR_BIT) * nelems * PE_size,       \
                                (_size) / (CHAR_BIT) * nelems * PE_size);      \
    /* Perform broadcast */                                                    \
    broadcast_helper_##_algo(dest, source, ((_size) / CHAR_BIT) * nelems,      \
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
 */
#define SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, _type, _typename)              \
  int shcoll_##_typename##_broadcast_##_algo(shmem_team_t team, _type *dest,   \
                                             const _type *source,              \
                                             size_t nelems, int PE_root) {     \
    /* Check initialization */                                                 \
    SHMEMU_CHECK_INIT();                                                       \
                                                                               \
    /* Check team validity */                                                  \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
                                                                               \
    /* Get team parameters */                                                  \
    const int PE_size = shmem_team_n_pes(team);                                \
                                                                               \
    /* Check buffer symmetry */                                                \
    SHMEMU_CHECK_SYMMETRIC(dest, sizeof(_type) * nelems * PE_size);            \
    SHMEMU_CHECK_SYMMETRIC(source, sizeof(_type) * nelems * PE_size);          \
                                                                               \
    /* Check for overlap between source and destination */                     \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source,                                  \
                                sizeof(_type) * nelems * PE_size,              \
                                sizeof(_type) * nelems * PE_size);             \
                                                                               \
    /* Get team parameters */                                                  \
    /* TODO: use internal psync pool and team translate PE to team's PE 0 */   \
    const int PE_start = 0;                                                    \
    const int logPE_stride = 0;                                                \
    const int me = shmem_my_pe();                                              \
    const int world_root =                                                     \
        shmem_team_translate_pe(team, PE_root, SHMEM_TEAM_WORLD);              \
                                                                               \
    /* Allocate pSync from symmetric heap */                                   \
    long *pSync = shmem_malloc(SHCOLL_BCAST_SYNC_SIZE * sizeof(long));         \
    if (!pSync)                                                                \
      return -1;                                                               \
                                                                               \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_BCAST_SYNC_SIZE; i++) {                         \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
                                                                               \
    /* Ensure all PEs have initialized pSync */                                \
    shmem_team_sync(team);                                                     \
                                                                               \
    /* Initialize destination buffer */                                        \
    if (me != world_root) {                                                    \
      /* Non-root PEs: zero out destination buffer */                          \
      memset(dest, 0, nelems * sizeof(_type));                                 \
    } else {                                                                   \
      /* Root PE: copy from source to destination */                           \
      memcpy(dest, source, nelems * sizeof(_type));                            \
    }                                                                          \
                                                                               \
    /* Perform broadcast - Note we now always use dest as target */            \
    broadcast_helper_##_algo(dest, dest, nelems * sizeof(_type), PE_root,      \
                             PE_start, logPE_stride, PE_size, pSync);          \
                                                                               \
    /* Ensure broadcast completion */                                          \
    shmem_team_sync(team);                                                     \
                                                                               \
    /* Reset pSync before freeing */                                           \
    for (int i = 0; i < SHCOLL_BCAST_SYNC_SIZE; i++) {                         \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    shmem_team_sync(team);                                                     \
                                                                               \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

/**
 * @brief Macro to define broadcast implementations for all types
 */
#define DEFINE_SHCOLL_BROADCAST_TYPES(_algo)                                   \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, float, float)                        \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, double, double)                      \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, long double, longdouble)             \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, char, char)                          \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, signed char, schar)                  \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, short, short)                        \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, int, int)                            \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, long, long)                          \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, long long, longlong)                 \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, unsigned char, uchar)                \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, unsigned short, ushort)              \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, unsigned int, uint)                  \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, unsigned long, ulong)                \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, unsigned long long, ulonglong)       \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, int8_t, int8)                        \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, int16_t, int16)                      \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, int32_t, int32)                      \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, int64_t, int64)                      \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, uint8_t, uint8)                      \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, uint16_t, uint16)                    \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, uint32_t, uint32)                    \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, uint64_t, uint64)                    \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, size_t, size)                        \
  SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, ptrdiff_t, ptrdiff)

/* Generate typed implementations for all algorithms */
DEFINE_SHCOLL_BROADCAST_TYPES(linear)
DEFINE_SHCOLL_BROADCAST_TYPES(complete_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(binomial_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(knomial_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(knomial_tree_signal)
DEFINE_SHCOLL_BROADCAST_TYPES(scatter_collect)

/**
 * @brief Macro to define broadcast routines for untyped memory
 */
#define SHCOLL_BROADCASTMEM_DEFINITION(_algo)                                  \
  int shcoll_broadcastmem_##_algo(shmem_team_t team, void *dest,               \
                                  const void *source, size_t nelems,           \
                                  int PE_root) {                               \
    /* Check initialization */                                                 \
    SHMEMU_CHECK_INIT();                                                       \
                                                                               \
    /* Check team validity */                                                  \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
                                                                               \
    /* Check for NULL pointers */                                              \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
                                                                               \
    /* Get team parameters */                                                  \
    const int PE_size = shmem_team_n_pes(team);                                \
                                                                               \
    /* Check buffer symmetry */                                                \
    SHMEMU_CHECK_SYMMETRIC(dest, nelems *PE_size);                             \
    SHMEMU_CHECK_SYMMETRIC(source, nelems *PE_size);                           \
                                                                               \
    /* Check for overlap between source and destination */                     \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, nelems *PE_size,                 \
                                nelems *PE_size);                              \
                                                                               \
    /* Get team parameters */                                                  \
    /* TODO: use internal psync pool and team translate PE to team's PE 0 */   \
    const int PE_start = 0;                                                    \
    const int logPE_stride = 0;                                                \
    const int world_root =                                                     \
        shmem_team_translate_pe(team, PE_root, SHMEM_TEAM_WORLD);              \
                                                                               \
    /* Allocate pSync from symmetric heap */                                   \
    long *pSync = shmem_malloc(SHCOLL_BCAST_SYNC_SIZE * sizeof(long));         \
    if (!pSync)                                                                \
      return -1;                                                               \
                                                                               \
    /* Initialize pSync */                                                     \
    for (int i = 0; i < SHCOLL_BCAST_SYNC_SIZE; i++) {                         \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
                                                                               \
    /* Ensure all PEs have initialized pSync */                                \
    shmem_team_sync(team);                                                     \
                                                                               \
    /* For team-based broadcasts, root PE should also update its dest */       \
    if (shmem_my_pe() == world_root) {                                         \
      memcpy(dest, source, nelems);                                            \
    }                                                                          \
                                                                               \
    /* Perform broadcast */                                                    \
    broadcast_helper_##_algo(dest, source, nelems, PE_root, PE_start,          \
                             logPE_stride, PE_size, pSync);                    \
                                                                               \
    /* Ensure broadcast completion */                                          \
    shmem_team_sync(team);                                                     \
                                                                               \
    /* Reset pSync before freeing */                                           \
    for (int i = 0; i < SHCOLL_BCAST_SYNC_SIZE; i++) {                         \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    }                                                                          \
    shmem_team_sync(team);                                                     \
                                                                               \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

/* Define broadcast routines using different algorithms */
SHCOLL_BROADCASTMEM_DEFINITION(linear)
SHCOLL_BROADCASTMEM_DEFINITION(complete_tree)
SHCOLL_BROADCASTMEM_DEFINITION(binomial_tree)
SHCOLL_BROADCASTMEM_DEFINITION(knomial_tree)
SHCOLL_BROADCASTMEM_DEFINITION(knomial_tree_signal)
SHCOLL_BROADCASTMEM_DEFINITION(scatter_collect)
