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
#include "shmemc.h" /* for shmemc_team_h */
#include "shcoll/compat.h"
#include "shcoll/common.h"
#include "util/trees.h"
#include <shmem/api_types.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#if ENABLE_SHMEM_ENCRYPTION
#include "shmemx.h"
#include "shmem_enc.h"

/*
 * -- helpers ----------------------------------------------------------------
 */

/*
 * shortcut to look up the UCP endpoint of a context
 */
inline static ucp_ep_h lookup_ucp_ep(shmemc_context_h ch, int pe) {
  return ch->eps[pe];
}

/*
 * find rkey for memory "region" on PE "pe"
 */
inline static ucp_rkey_h lookup_rkey(shmemc_context_h ch, size_t region,
                                     int pe) {
  return ch->racc[region].rinfo[pe].rkey;
}

/*
 * -- translation helpers ---------------------------------------------------
 */

/*
 * is the given address in this memory region?  Non-zero if yes, 0 if
 * not.
 */
inline static int in_region(uint64_t addr, size_t region) {
  const mem_info_t *mip = &proc.comms.regions[region].minfo[proc.li.rank];

  return (mip->base <= addr) && (addr < mip->end);
}

/*
 * find memory region that ADDR is in, or -1 if none
 */
inline static long lookup_region(uint64_t addr) {
  long r;

  /*
   * Let's search down from top heap to globals (#0) under
   * assumption most data in heaps and newest one is most likely
   * (may need to revisit)
   */
  for (r = proc.comms.nregions - 1; r >= 0; --r) {
    if (in_region(addr, (size_t)r)) {
      return r;
      /* NOT REACHED */
    }
  }

  return -1L;
}

/*
 * where the heap lives on PE "pe"
 */

inline static size_t get_base(size_t region, int pe) {
  return proc.comms.regions[region].minfo[pe].base;
}

inline static uint64_t translate_region_address(uint64_t local_addr,
                                                size_t region, int pe) {
  if (region == 0) {
    return local_addr;
  } else {
    const long my_offset = local_addr - get_base(region, proc.li.rank);

    if (my_offset < 0) {
      return 0;
    }

    return my_offset + get_base(region, pe);
  }
}

inline static uint64_t translate_address(uint64_t local_addr, int pe) {
  long r = lookup_region(local_addr);

  if (r < 0) {
    return 0;
  }

  return translate_region_address(local_addr, r, pe);
}

/*
 * All ops here need to find remote keys and addresses
 */
inline static void get_remote_key_and_addr(shmemc_context_h ch,
                                           uint64_t local_addr, int pe,
                                           ucp_rkey_h *rkey_p,
                                           uint64_t *raddr_p) {
  const long r = lookup_region(local_addr);

  shmemu_assert(r >= 0, "shmem_enc/dec, get_rkey/addr: can't find memory region for %p",
                (void *)local_addr);

  *rkey_p = lookup_rkey(ch, r, pe);
  *raddr_p = translate_region_address(local_addr, r, pe);
}



#endif /* ENABLE_SHMEM_ENCRYPTION */

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

#if ENABLE_SHMEM_ENCRYPTION
  size_t encrypt_size = 0;
  uint64_t enc_src = 0;
  ucp_rkey_h r_key;
  int segment_count = 0;
  size_t temp_size = 0;
  if (proc.env.shmem_encryption && me == root){
     get_remote_key_and_addr(defcp, (uint64_t)source, me, &r_key, &enc_src);
     shmemu_assert(enc_src, "linear_broadcast: Buffer is NULL!\n");
     segment_count = shmemx_encrypt_single_buffer_omp((unsigned char*)(enc_src), 0,
           source, 0, nbytes, &encrypt_size);
     temp_size = encrypt_size;
  }
#endif /* ENABLE_SHMEM_ENCRYPTION */

  if (me != root) {
#if ENABLE_SHMEM_ENCRYPTION
     if (proc.env.shmem_encryption){
        encrypt_size = nbytes + AES_RAND_BYTES;
        shmemc_ctx_get(SHMEM_CTX_DEFAULT, target, source, nbytes + AES_TAG_LEN + AES_RAND_BYTES, root);
        shmemx_decrypt_single_buffer_omp((unsigned char *)(target), 0, target, 0, nbytes+AES_RAND_BYTES, encrypt_size);
     }else
#endif /* ENABLE_SHMEM_ENCRYPTION */
        shmem_getmem(target, source, nbytes, root);
  }
#if ENABLE_SHMEM_ENCRYPTION
  else{
     if (proc.env.shmem_encryption){
        shmemx_decrypt_single_buffer_omp((unsigned char *)(enc_src), 0,(void *) source, 0, nbytes+AES_RAND_BYTES, temp_size);

     }
  }
#endif /* ENABLE_SHMEM_ENCRYPTION */
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

#if ENABLE_SHMEM_ENCRYPTION
  size_t encrypt_size = 0, temp_size = 0;
  uint64_t enc_src = 0;
  uint64_t dec_src = 0;
  ucp_rkey_h r_key = 0;
  int segment_count = 0;
#endif /* ENABLE_SHMEM_ENCRYPTION */

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
#if ENABLE_SHMEM_ENCRYPTION
      if(proc.env.shmem_encryption){
          get_remote_key_and_addr(defcp, (uint64_t) source, me_as, &r_key, &enc_src);
          shmemu_assert(enc_src, "Binomial tree bscast: Buffer is NULL\n");
          segment_count = shmemx_encrypt_single_buffer_omp((unsigned char*)enc_src,
                  0, source, 0, nbytes, &encrypt_size);
          temp_size = encrypt_size;
      }
#endif /* ENABLE_SHMEM_ENCRYPTION */

    for (i = 0; i < node.children_num; i++) {
      dst = PE_start + node.children[i] * stride;
#if ENABLE_SHMEM_ENCRYPTION
         if (proc.env.shmem_encryption)
            shmemc_ctx_put_nbi(SHMEM_CTX_DEFAULT, target, (void *)(enc_src), nbytes + AES_TAG_LEN + AES_RAND_BYTES, dst);
         else
#endif /* ENABLE_SHMEM_ENCRYPTION */
            shmem_putmem_nbi(target, source, nbytes, dst);
         shmem_fence();
         shmem_long_atomic_inc(pSync, dst);
    }

#if ENABLE_SHMEM_ENCRYPTION
    if (proc.env.shmem_encryption)
        shmem_quiet();
#endif /* ENABLE_SHMEM_ENCRYPTION */


    shmem_long_wait_until(pSync, SHMEM_CMP_EQ,
                          SHCOLL_SYNC_VALUE + node.children_num +
                              (me_as == PE_root ? 0 : 1));
  }

#if ENABLE_SHMEM_ENCRYPTION
  if (proc.env.shmem_encryption){
      if (node.children_num != 0){
          shmemx_decrypt_single_buffer_omp((unsigned char*)(enc_src), 0, (void *) source, 0,
                  nbytes+AES_RAND_BYTES, temp_size);
      }
      //else{
  //        get_remote_key_and_addr(defcp, (uint64_t) target, me_as, &r_key, &dec_src);
  //        shmemx_decrypt_single_buffer_omp((unsigned char*)(dec_src), 0, (void *) source, 0,
  //                nbytes + AES_RAND_BYTES, temp_size);
  //    }
  }
#endif /* ENABLE_SHMEM_ENCRYPTION */

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
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_BCAST_SYNC_SIZE);      \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, (_size) / (CHAR_BIT) * nelems,   \
                                (_size) / (CHAR_BIT) * nelems);                \
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
 * @brief Macro for typed broadcast implementations using the team's pSync
 */
#define SHCOLL_BROADCAST_TYPE_DEFINITION(_algo, _type, _typename)              \
  int shcoll_##_typename##_broadcast_##_algo(shmem_team_t team, _type *dest,   \
                                             const _type *source,              \
                                             size_t nelems, int PE_root) {     \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
                                                                               \
    shmemc_team_h team_h = (shmemc_team_h)team;                                \
    SHMEMU_CHECK_TEAM_STRIDE(team_h->stride, __func__);                        \
    SHMEMU_CHECK_SYMMETRIC(dest, sizeof(_type) * nelems * team_h->nranks);     \
    SHMEMU_CHECK_SYMMETRIC(source, sizeof(_type) * nelems * team_h->nranks);   \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, sizeof(_type) * nelems,          \
                                sizeof(_type) * nelems);                       \
                                                                               \
    SHMEMU_CHECK_NULL(shmemc_team_get_psync(team_h, SHMEMC_PSYNC_BROADCAST),   \
                      "team_h->pSyncs[BROADCAST]");                            \
                                                                               \
    if (team_h->rank != PE_root)                                               \
      memset(dest, 0, nelems * sizeof(_type));                                 \
    else                                                                       \
      memcpy(dest, source, nelems * sizeof(_type));                            \
                                                                               \
    shmem_team_sync(team_h);                                                   \
                                                                               \
    broadcast_helper_##_algo(                                                  \
        dest, source, nelems * sizeof(_type), PE_root, team_h->start,          \
        (team_h->stride > 0) ? (int)log2((double)team_h->stride) : 0,          \
        team_h->nranks,                                                        \
        shmemc_team_get_psync(team_h, SHMEMC_PSYNC_BROADCAST));                \
                                                                               \
    shmemc_team_reset_psync(team_h, SHMEMC_PSYNC_BROADCAST);                   \
                                                                               \
    return 0;                                                                  \
  }

#define DEFINE_BROADCAST_TYPES(_type, _typename)                               \
  SHCOLL_BROADCAST_TYPE_DEFINITION(linear, _type, _typename)                   \
  SHCOLL_BROADCAST_TYPE_DEFINITION(complete_tree, _type, _typename)            \
  SHCOLL_BROADCAST_TYPE_DEFINITION(binomial_tree, _type, _typename)            \
  SHCOLL_BROADCAST_TYPE_DEFINITION(knomial_tree, _type, _typename)             \
  SHCOLL_BROADCAST_TYPE_DEFINITION(knomial_tree_signal, _type, _typename)      \
  SHCOLL_BROADCAST_TYPE_DEFINITION(scatter_collect, _type, _typename)

SHMEM_STANDARD_RMA_TYPE_TABLE(DEFINE_BROADCAST_TYPES)
#undef DEFINE_BROADCAST_TYPES

/**
 * @brief Macro for memory broadcast implementations using legacy helpers
 */
#define SHCOLL_BROADCASTMEM_DEFINITION(_algo)                                  \
  int shcoll_broadcastmem_##_algo(shmem_team_t team, void *dest,               \
                                  const void *source, size_t nelems,           \
                                  int PE_root) {                               \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
    shmemc_team_h team_h = (shmemc_team_h)team;                                \
    SHMEMU_CHECK_TEAM_STRIDE(team_h->stride, __func__);                        \
    SHMEMU_CHECK_SYMMETRIC(dest, nelems * team_h->nranks);                     \
    SHMEMU_CHECK_SYMMETRIC(source, nelems * team_h->nranks);                   \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, nelems, nelems);                 \
    SHMEMU_CHECK_NULL(shmemc_team_get_psync(team_h, SHMEMC_PSYNC_BROADCAST),   \
                      "team_h->pSyncs[BROADCAST]");                            \
                                                                               \
    shmem_team_sync(team_h);                                                   \
                                                                               \
    if (team_h->rank == PE_root)                                               \
      memcpy(dest, source, nelems);                                            \
                                                                               \
    broadcast_helper_##_algo(                                                  \
        dest, source, nelems, PE_root, team_h->start,                          \
        (team_h->stride > 0) ? (int)log2((double)team_h->stride) : 0,          \
        team_h->nranks,                                                        \
        shmemc_team_get_psync(team_h, SHMEMC_PSYNC_BROADCAST));                \
                                                                               \
    shmemc_team_reset_psync(team_h, SHMEMC_PSYNC_BROADCAST);                   \
    return 0;                                                                  \
  }

/* Define broadcast routines using different algorithms */
SHCOLL_BROADCASTMEM_DEFINITION(linear)
SHCOLL_BROADCASTMEM_DEFINITION(complete_tree)
SHCOLL_BROADCASTMEM_DEFINITION(binomial_tree)
SHCOLL_BROADCASTMEM_DEFINITION(knomial_tree)
SHCOLL_BROADCASTMEM_DEFINITION(knomial_tree_signal)
SHCOLL_BROADCASTMEM_DEFINITION(scatter_collect)
