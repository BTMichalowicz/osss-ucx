#include "shcoll.h"
#include "shcoll/compat.h"
#include "shcoll/common.h"
#include "util/trees.h"

#include <stdio.h>
#include <string.h>

static int tree_degree_broadcast = 2;
static int knomial_tree_radix_barrier = 2;

void shcoll_set_broadcast_tree_degree(int tree_degree) {
  tree_degree_broadcast = tree_degree;
}

void shcoll_set_broadcast_knomial_tree_radix_barrier(int tree_radix) {
  knomial_tree_radix_barrier = tree_radix;
}

/* Helper function definitions */
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
 * @brief Macro for typed broadcast implementations using legacy helpers
 * FIXME: is SHCOLL_BCAST_SYNC_SIZE correct?
 */
#define SHCOLL_BROADCAST_TYPED_DEFINITION(_name, _type, _typename)             \
  int shcoll_##_typename##_broadcast_##_name(shmem_team_t team, _type *dest,   \
                                             const _type *source,              \
                                             size_t nelems, int PE_root) {     \
    /* Convert team to legacy parameters */                                    \
    int PE_start = shmem_team_translate_pe(team, PE_root, SHMEM_TEAM_WORLD);   \
    int PE_size = shmem_team_n_pes(team);                                      \
    static long pSync[SHCOLL_BCAST_SYNC_SIZE];                                 \
                                                                               \
    broadcast_helper_##_name(dest, source, sizeof(_type) * nelems, PE_root,    \
                             PE_start, 0, PE_size, pSync);                     \
    return 0;                                                                  \
  }

/**
 * @brief Macro for sized broadcast implementations using legacy helpers
 */
#define SHCOLL_BROADCAST_SIZED_DEFINITION(_name, _size)                        \
  void shcoll_broadcast##_size##_##_name(                                      \
      void *dest, const void *source, size_t nelems, int PE_root,              \
      int PE_start, int logPE_stride, int PE_size, long *pSync) {              \
    broadcast_helper_##_name(dest, source, (_size / CHAR_BIT) * nelems,        \
                             PE_root, PE_start, logPE_stride, PE_size, pSync); \
  }

/**
 * @brief Macro to define broadcast implementations for all types
 */
#define DEFINE_SHCOLL_BROADCAST_TYPES(_name)                                   \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, float, float)                         \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, double, double)                       \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, long double, longdouble)              \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, char, char)                           \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, signed char, schar)                   \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, short, short)                         \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, int, int)                             \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, long, long)                           \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, long long, longlong)                  \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, unsigned char, uchar)                 \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, unsigned short, ushort)               \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, unsigned int, uint)                   \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, unsigned long, ulong)                 \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, unsigned long long, ulonglong)        \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, int8_t, int8)                         \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, int16_t, int16)                       \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, int32_t, int32)                       \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, int64_t, int64)                       \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, uint8_t, uint8)                       \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, uint16_t, uint16)                     \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, uint32_t, uint32)                     \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, uint64_t, uint64)                     \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, size_t, size)                         \
  SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, ptrdiff_t, ptrdiff)

#define SHCOLL_BROADCAST_TYPED_FOR_TYPE(_name, _type, _typename)               \
  SHCOLL_BROADCAST_TYPED_DEFINITION(_name, _type, _typename)

/* Generate typed implementations for all algorithms */
DEFINE_SHCOLL_BROADCAST_TYPES(linear)
DEFINE_SHCOLL_BROADCAST_TYPES(complete_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(binomial_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(knomial_tree)
DEFINE_SHCOLL_BROADCAST_TYPES(knomial_tree_signal)
DEFINE_SHCOLL_BROADCAST_TYPES(scatter_collect)

/* Generate sized implementations for all algorithms */
/* Linear */
SHCOLL_BROADCAST_SIZED_DEFINITION(linear, 8)
SHCOLL_BROADCAST_SIZED_DEFINITION(linear, 16)
SHCOLL_BROADCAST_SIZED_DEFINITION(linear, 32)
SHCOLL_BROADCAST_SIZED_DEFINITION(linear, 64)

/* Complete tree */
SHCOLL_BROADCAST_SIZED_DEFINITION(complete_tree, 8)
SHCOLL_BROADCAST_SIZED_DEFINITION(complete_tree, 16)
SHCOLL_BROADCAST_SIZED_DEFINITION(complete_tree, 32)
SHCOLL_BROADCAST_SIZED_DEFINITION(complete_tree, 64)

/* Binomial tree */
SHCOLL_BROADCAST_SIZED_DEFINITION(binomial_tree, 8)
SHCOLL_BROADCAST_SIZED_DEFINITION(binomial_tree, 16)
SHCOLL_BROADCAST_SIZED_DEFINITION(binomial_tree, 32)
SHCOLL_BROADCAST_SIZED_DEFINITION(binomial_tree, 64)

/* K-nomial tree */
SHCOLL_BROADCAST_SIZED_DEFINITION(knomial_tree, 8)
SHCOLL_BROADCAST_SIZED_DEFINITION(knomial_tree, 16)
SHCOLL_BROADCAST_SIZED_DEFINITION(knomial_tree, 32)
SHCOLL_BROADCAST_SIZED_DEFINITION(knomial_tree, 64)

/* K-nomial tree with signal */
SHCOLL_BROADCAST_SIZED_DEFINITION(knomial_tree_signal, 8)
SHCOLL_BROADCAST_SIZED_DEFINITION(knomial_tree_signal, 16)
SHCOLL_BROADCAST_SIZED_DEFINITION(knomial_tree_signal, 32)
SHCOLL_BROADCAST_SIZED_DEFINITION(knomial_tree_signal, 64)

/* Scatter-collect */
SHCOLL_BROADCAST_SIZED_DEFINITION(scatter_collect, 8)
SHCOLL_BROADCAST_SIZED_DEFINITION(scatter_collect, 16)
SHCOLL_BROADCAST_SIZED_DEFINITION(scatter_collect, 32)
SHCOLL_BROADCAST_SIZED_DEFINITION(scatter_collect, 64)
