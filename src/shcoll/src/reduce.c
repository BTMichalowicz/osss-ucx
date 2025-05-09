/*
 * @file reduction.c
 * @brief Implementation of collective reduction operations
 *
 * This file contains implementations of various reduction algorithms including:
 * - Linear reduction
 * - Binomial tree reduction
 * - Recursive doubling reduction
 * - Rabenseifner's algorithm
 *
 * Each algorithm is implemented as a macro that generates type-specific
 * implementations for different reduction operations (AND, OR, XOR, MIN, MAX,
 * SUM, PROD) and data types (int, long, float, double etc).
 */

#include "shcoll.h"
#include "util/bithacks.h"
#include "../tests/util/debug.h"

#include "shmem.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

/*
 * @brief Helper macro to define local reduction operations
 *
 * @param _name Name of the reduction operation (e.g. sum, prod)
 * @param _type Data type to operate on
 * @param _op Binary operator to apply
 */
#define REDUCE_HELPER_LOCAL(_name, _type, _op)                                 \
  inline static void local_##_name##_reduce(                                   \
      _type *dest, const _type *src1, const _type *src2, size_t nreduce) {     \
    size_t i;                                                                  \
                                                                               \
    for (i = 0; i < nreduce; i++) {                                            \
      dest[i] = _op(src1[i], src2[i]);                                         \
    }                                                                          \
  }

/*
 * @brief Helper macro to define linear reduction operations
 *
 * Implements a linear reduction algorithm where PE 0 sequentially reduces
 * values from all other PEs.
 *
 * @param _name Name of the reduction operation
 * @param _type Data type to operate on
 * @param _op Binary operator to apply
 */
#define REDUCE_HELPER_LINEAR(_name, _type, _op)                                \
  void reduce_helper_##_name##_linear(                                         \
      _type *dest, const _type *source, int nreduce, int PE_start,             \
      int logPE_stride, int PE_size, _type *pWrk, long *pSync) {               \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    const int me_as = (me - PE_start) / stride;                                \
    const size_t nbytes = sizeof(_type) * nreduce;                             \
                                                                               \
    _type *tmp_array;                                                          \
    int i;                                                                     \
                                                                               \
    shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);             \
                                                                               \
    if (me_as == 0) {                                                          \
      tmp_array = malloc(nbytes);                                              \
      if (!tmp_array) {                                                        \
        /* TODO: raise error */                                                \
        exit(-1);                                                              \
      }                                                                        \
                                                                               \
      memcpy(tmp_array, source, nbytes);                                       \
                                                                               \
      for (i = 1; i < PE_size; i++) {                                          \
        shmem_getmem(dest, source, nbytes, PE_start + i * stride);             \
        local_##_name##_reduce(tmp_array, tmp_array, dest, nreduce);           \
      }                                                                        \
                                                                               \
      memcpy(dest, tmp_array, nbytes);                                         \
      free(tmp_array);                                                         \
    }                                                                          \
                                                                               \
    shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync);             \
                                                                               \
    shcoll_broadcast8_linear(dest, dest, nreduce * sizeof(_type), PE_start,    \
                             PE_start, logPE_stride, PE_size, pSync + 1);      \
  }

/*
 * @brief Helper macro to define binomial tree reduction operations
 *
 * Implements a binomial tree reduction algorithm for better scalability.
 *
 * @param _name Name of the reduction operation
 * @param _type Data type to operate on
 * @param _op Binary operator to apply
 */
#define REDUCE_HELPER_BINOMIAL(_name, _type, _op)                              \
  void reduce_helper_##_name##_binomial(                                       \
      _type *dest, const _type *source, int nreduce, int PE_start,             \
      int logPE_stride, int PE_size, _type *pWrk, long *pSync) {               \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    int me_as = (me - PE_start) / stride;                                      \
    int target_as;                                                             \
    size_t nbytes = sizeof(_type) * nreduce;                                   \
    _type *tmp_array = NULL;                                                   \
    unsigned mask = 0x1;                                                       \
    long old_pSync = SHCOLL_SYNC_VALUE;                                        \
    long to_receive = 0;                                                       \
    long recv_mask;                                                            \
                                                                               \
    tmp_array = malloc(nbytes);                                                \
    if (!tmp_array) {                                                          \
      /* TODO: raise error */                                                  \
      fprintf(stderr, "PE %d: Cannot allocate memory!\n", me);                 \
      exit(-1);                                                                \
    }                                                                          \
                                                                               \
    if (source != dest) {                                                      \
      memcpy(dest, source, nbytes);                                            \
    }                                                                          \
                                                                               \
    /* Stop if all messages are received or if there are no more PE on right   \
     */                                                                        \
    for (mask = 0x1; !(me_as & mask) && ((me_as | mask) < PE_size);            \
         mask <<= 1) {                                                         \
      to_receive |= mask;                                                      \
    }                                                                          \
                                                                               \
    /* TODO: fix if SHCOLL_SYNC_VALUE not 0 */                                 \
    /* Wait until all messages are received */                                 \
    while (to_receive != 0) {                                                  \
      memcpy(tmp_array, dest, nbytes);                                         \
      shmem_long_wait_until(pSync, SHMEM_CMP_NE, old_pSync);                   \
      recv_mask = shmem_long_atomic_fetch(pSync, me);                          \
                                                                               \
      recv_mask &= to_receive;                                                 \
      recv_mask ^= (recv_mask - 1) & recv_mask;                                \
                                                                               \
      /* Get array and reduce */                                               \
      target_as = (int)(me_as | recv_mask);                                    \
      shmem_getmem(dest, dest, nbytes, PE_start + target_as * stride);         \
                                                                               \
      local_##_name##_reduce(dest, dest, tmp_array, nreduce);                  \
                                                                               \
      /* Mark as received */                                                   \
      to_receive &= ~recv_mask;                                                \
      old_pSync |= recv_mask;                                                  \
    }                                                                          \
                                                                               \
    /* Notify parent */                                                        \
    if (me_as != 0) {                                                          \
      target_as = me_as & (me_as - 1);                                         \
      shmem_long_atomic_add(pSync, me_as ^ target_as,                          \
                            PE_start + target_as * stride);                    \
    }                                                                          \
                                                                               \
    shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);                                \
    shcoll_barrier_linear(PE_start, logPE_stride, PE_size, pSync + 1);         \
                                                                               \
    shcoll_broadcast8_binomial_tree(dest, dest, nreduce * sizeof(_type),       \
                                    PE_start, PE_start, logPE_stride, PE_size, \
                                    pSync + 2);                                \
                                                                               \
    free(tmp_array);                                                           \
  }

/*
 * @brief Helper macro to define recursive doubling reduction operations
 *
 * Implements a recursive doubling algorithm for better scalability.
 *
 * @param _name Name of the reduction operation
 * @param _type Data type to operate on
 * @param _op Binary operator to apply
 */
#define REDUCE_HELPER_REC_DBL(_name, _type, _op)                               \
  void reduce_helper_##_name##_rec_dbl(                                        \
      _type *dest, const _type *source, int nreduce, int PE_start,             \
      int logPE_stride, int PE_size, _type *pWrk, long *pSync) {               \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
    int peer;                                                                  \
                                                                               \
    size_t nbytes = nreduce * sizeof(_type);                                   \
                                                                               \
    /* Get my index in the active set */                                       \
    int me_as = (me - PE_start) / stride;                                      \
    int mask;                                                                  \
                                                                               \
    int xchg_peer_p2s;                                                         \
    int xchg_peer_as;                                                          \
    int xchg_peer_pe;                                                          \
                                                                               \
    /* Power 2 set */                                                          \
    int me_p2s;                                                                \
    int p2s_size;                                                              \
                                                                               \
    _type *tmp_array = NULL;                                                   \
                                                                               \
    /* Find the greatest power of 2 lower than PE_size */                      \
    for (p2s_size = 1; p2s_size * 2 <= PE_size; p2s_size *= 2)                 \
      ;                                                                        \
                                                                               \
    /* Check if the current PE belongs to the power 2 set */                   \
    me_p2s = me_as * p2s_size / PE_size;                                       \
    if ((me_p2s * PE_size + p2s_size - 1) / p2s_size != me_as) {               \
      me_p2s = -1;                                                             \
    }                                                                          \
                                                                               \
    /* If current PE belongs to the power 2 set, it will need temporary buffer \
     */                                                                        \
    if (me_p2s != -1) {                                                        \
      tmp_array = malloc(nreduce * sizeof(_type));                             \
      if (tmp_array == NULL) {                                                 \
        /* TODO: raise error */                                                \
        fprintf(stderr, "PE %d: Cannot allocate memory!\n", me);               \
        exit(-1);                                                              \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* Check if the current PE should wait/send data to the peer */            \
    if (me_p2s == -1) {                                                        \
      /* Notify peer that the data is ready */                                 \
      peer = PE_start + (me_as - 1) * stride;                                  \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE + 1, peer);                        \
    } else if ((me_as + 1) * p2s_size / PE_size == me_p2s) {                   \
      /* We should wait for the data to be ready */                            \
      peer = PE_start + (me_as + 1) * stride;                                  \
                                                                               \
      shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);           \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);                              \
                                                                               \
      /* Get the array and reduce */                                           \
      shmem_getmem(dest, source, nbytes, peer);                                \
      local_##_name##_reduce(tmp_array, dest, source, nreduce);                \
    } else {                                                                   \
      memcpy(tmp_array, source, nbytes);                                       \
    }                                                                          \
                                                                               \
    /* If the current PE belongs to the power 2 set, do recursive doubling */  \
    if (me_p2s != -1) {                                                        \
      int i;                                                                   \
                                                                               \
      for (mask = 0x1, i = 1; mask < p2s_size; mask <<= 1, i++) {              \
        xchg_peer_p2s = me_p2s ^ mask;                                         \
        xchg_peer_as = (xchg_peer_p2s * PE_size + p2s_size - 1) / p2s_size;    \
        xchg_peer_pe = PE_start + xchg_peer_as * stride;                       \
                                                                               \
        /* Notify the peer PE that current PE is ready to accept the data */   \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE + 1, xchg_peer_pe);          \
                                                                               \
        /* Wait until the peer PE is ready to accept the data */               \
        shmem_long_wait_until(pSync + i, SHMEM_CMP_GT, SHCOLL_SYNC_VALUE);     \
                                                                               \
        /* Send the data to the peer */                                        \
        shmem_putmem(dest, tmp_array, nbytes, xchg_peer_pe);                   \
        shmem_fence();                                                         \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE + 2, xchg_peer_pe);          \
                                                                               \
        /* Wait until the data is received and do local reduce */              \
        shmem_long_wait_until(pSync + i, SHMEM_CMP_GT, SHCOLL_SYNC_VALUE + 1); \
        local_##_name##_reduce(tmp_array, tmp_array, dest, nreduce);           \
                                                                               \
        /* Reset the pSync for the current round */                            \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE, me);                        \
      }                                                                        \
                                                                               \
      memcpy(dest, tmp_array, nbytes);                                         \
    }                                                                          \
                                                                               \
    if (me_p2s == -1) {                                                        \
      /* Wait to get the data from a PE that is in the power 2 set */          \
      shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);           \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);                              \
    } else if ((me_as + 1) * p2s_size / PE_size == me_p2s) {                   \
      /* Send data to peer PE that is outside the power 2 set */               \
      peer = PE_start + (me_as + 1) * stride;                                  \
                                                                               \
      shmem_putmem(dest, dest, nbytes, peer);                                  \
      shmem_fence();                                                           \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE + 1, peer);                        \
    }                                                                          \
                                                                               \
    if (tmp_array != NULL) {                                                   \
      free(tmp_array);                                                         \
    }                                                                          \
  }

/*
 * @brief Helper macro to define Rabenseifner reduction operations
 *
 * Implements Rabenseifner's reduction algorithm which combines recursive
 * halving with recursive doubling for better scalability and communication
 * efficiency.
 *
 * @param _name Name of the reduction operation
 * @param _type Data type to operate on
 * @param _op Binary operator to apply
 */
#define REDUCE_HELPER_RABENSEIFNER(_name, _type, _op)                          \
  void reduce_helper_##_name##_rabenseifner(                                   \
      _type *dest, const _type *source, int nreduce, int PE_start,             \
      int logPE_stride, int PE_size, _type *pWrk, long *pSync) {               \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    int me_as = (me - PE_start) / stride;                                      \
    int peer;                                                                  \
    size_t i;                                                                  \
    const size_t nelems = (const size_t)nreduce;                               \
                                                                               \
    int block_idx_begin;                                                       \
    int block_idx_end;                                                         \
                                                                               \
    ptrdiff_t block_offset;                                                    \
    ptrdiff_t next_block_offset;                                               \
    size_t block_nelems;                                                       \
                                                                               \
    int xchg_peer_p2s;                                                         \
    int xchg_peer_as;                                                          \
    int xchg_peer_pe;                                                          \
                                                                               \
    /* Power 2 set */                                                          \
    int me_p2s;                                                                \
    int p2s_size;                                                              \
    int log_p2s_size;                                                          \
                                                                               \
    int distance;                                                              \
    _type *tmp_array = NULL;                                                   \
                                                                               \
    /* Find the greatest power of 2 lower than PE_size */                      \
    for (p2s_size = 1, log_p2s_size = 0; p2s_size * 2 <= PE_size;              \
         p2s_size *= 2, log_p2s_size++)                                        \
      ;                                                                        \
                                                                               \
    /* Check if the current PE belongs to the power 2 set */                   \
    me_p2s = me_as * p2s_size / PE_size;                                       \
    if ((me_p2s * PE_size + p2s_size - 1) / p2s_size != me_as) {               \
      me_p2s = -1;                                                             \
    }                                                                          \
                                                                               \
    /* If current PE belongs to the power 2 set, it will need temporary buffer \
     */                                                                        \
    if (me_p2s != -1) {                                                        \
      tmp_array = malloc((nelems / 2 + 1) * sizeof(_type));                    \
      if (tmp_array == NULL) {                                                 \
        /* TODO: raise error */                                                \
        fprintf(stderr, "PE %d: Cannot allocate memory!\n", me);               \
        exit(-1);                                                              \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* Check if the current PE should wait/send data to the peer */            \
    if (me_p2s == -1) {                                                        \
      /* Notify peer that the data is ready */                                 \
      peer = PE_start + (me_as - 1) * stride;                                  \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE + 1, peer);                        \
                                                                               \
      /* Wait until the data on peer node is ready and get the data (upper     \
       * half of the array) */                                                 \
      block_offset = nelems / 2;                                               \
      block_nelems = (size_t)(nelems - block_offset);                          \
                                                                               \
      shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);           \
      shmem_getmem(dest + block_offset, source + block_offset,                 \
                   block_nelems * sizeof(_type), peer);                        \
                                                                               \
      /* Reduce the upper half of the array */                                 \
      local_##_name##_reduce(dest + block_offset, dest + block_offset,         \
                             source + block_offset, block_nelems);             \
                                                                               \
      /* Send the upper half of the array to peer */                           \
      shmem_putmem(dest + block_offset, dest + block_offset,                   \
                   block_nelems * sizeof(_type), peer);                        \
      shmem_fence();                                                           \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE + 2, peer);                        \
    } else if ((me_as + 1) * p2s_size / PE_size == me_p2s) {                   \
      /* Notify peer that the data is ready */                                 \
      peer = PE_start + (me_as + 1) * stride;                                  \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE + 1, peer);                        \
                                                                               \
      /* Wait until the data on peer node is ready and get the data (lower     \
       * half of the array) */                                                 \
      block_offset = 0;                                                        \
      block_nelems = (size_t)(nelems / 2 - block_offset);                      \
                                                                               \
      shmem_long_wait_until(pSync, SHMEM_CMP_GT, SHCOLL_SYNC_VALUE);           \
      shmem_getmem(dest, source, block_nelems * sizeof(_type), peer);          \
                                                                               \
      /* Do local reduce */                                                    \
      local_##_name##_reduce(dest, dest, source, block_nelems);                \
                                                                               \
      /* Wait until the upper half is received from peer */                    \
      shmem_long_wait_until(pSync, SHMEM_CMP_GT, SHCOLL_SYNC_VALUE + 1);       \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);                              \
    } else {                                                                   \
      memcpy(dest, source, nelems * sizeof(_type));                            \
    }                                                                          \
                                                                               \
    /* For nodes in the power 2 set, dest contains data that should be reduced \
     */                                                                        \
                                                                               \
    /* Do reduce scatter with the nodes in power 2 set */                      \
    if (me_p2s != -1) {                                                        \
      block_idx_begin = 0;                                                     \
      block_idx_end = p2s_size;                                                \
                                                                               \
      for (distance = 1, i = 1; distance < p2s_size; distance <<= 1, i++) {    \
        xchg_peer_p2s = ((me_p2s & distance) == 0) ? me_p2s + distance         \
                                                   : me_p2s - distance;        \
        xchg_peer_as = (xchg_peer_p2s * PE_size + p2s_size - 1) / p2s_size;    \
        xchg_peer_pe = PE_start + xchg_peer_as * stride;                       \
                                                                               \
        /* Notify the peer PE that the data is ready to be read */             \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE + 1, xchg_peer_pe);          \
                                                                               \
        /* Check if the current PE is responsible for lower half of upper half \
         * of the vector */                                                    \
        if ((me_p2s & distance) == 0) {                                        \
          block_idx_end = (block_idx_begin + block_idx_end) / 2;               \
        } else {                                                               \
          block_idx_begin = (block_idx_begin + block_idx_end) / 2;             \
        }                                                                      \
                                                                               \
        /* TODO: possible overflow */                                          \
        block_offset = (block_idx_begin * nelems) / p2s_size;                  \
        next_block_offset = (block_idx_end * nelems) / p2s_size;               \
        block_nelems = (size_t)(next_block_offset - block_offset);             \
                                                                               \
        /* Wait until the data on peer PE is ready to be read and get the data \
         */                                                                    \
        shmem_long_wait_until(pSync + i, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + 1); \
        shmem_getmem(tmp_array, dest + block_offset,                           \
                     block_nelems * sizeof(_type), xchg_peer_pe);              \
                                                                               \
        /* Notify the peer PE that the data transfer has completed             \
         * successfully */                                                     \
        shmem_fence();                                                         \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE + 2, xchg_peer_pe);          \
                                                                               \
        /* Do local reduce */                                                  \
        local_##_name##_reduce(dest + block_offset, dest + block_offset,       \
                               tmp_array, block_nelems);                       \
                                                                               \
        /* Wait until the peer PE has read the data */                         \
        shmem_long_wait_until(pSync + i, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + 2); \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE, me);                        \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* For nodes in the power 2 set, dest contains data that should be reduced \
     */                                                                        \
                                                                               \
    /* Do collect with the nodes in power 2 set */                             \
    if (me_p2s != -1) {                                                        \
      block_offset = 0;                                                        \
      block_idx_begin = reverse_bits(me_p2s, log_p2s_size);                    \
      block_idx_end = block_idx_begin + 1;                                     \
                                                                               \
      for (distance = p2s_size / 2, i = sizeof(int) * CHAR_BIT + 1;            \
           distance > 0; distance >>= 1, i++) {                                \
        xchg_peer_p2s = ((me_p2s & distance) == 0) ? me_p2s + distance         \
                                                   : me_p2s - distance;        \
        xchg_peer_as = (xchg_peer_p2s * PE_size + p2s_size - 1) / p2s_size;    \
        xchg_peer_pe = PE_start + xchg_peer_as * stride;                       \
                                                                               \
        /* TODO: possible overflow */                                          \
        block_offset = (block_idx_begin * nelems) / p2s_size;                  \
        next_block_offset = (block_idx_end * nelems) / p2s_size;               \
        block_nelems = (size_t)(next_block_offset - block_offset);             \
                                                                               \
        shmem_putmem(dest + block_offset, dest + block_offset,                 \
                     block_nelems * sizeof(_type), xchg_peer_pe);              \
        shmem_fence();                                                         \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE + 1, xchg_peer_pe);          \
                                                                               \
        /* Wait until the data has arrived from exchange the peer PE */        \
        shmem_long_wait_until(pSync + i, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + 1); \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE, me);                        \
                                                                               \
        /* Updated the block range */                                          \
        if ((me_p2s & distance) == 0) {                                        \
          block_idx_end += (block_idx_end - block_idx_begin);                  \
        } else {                                                               \
          block_idx_begin -= (block_idx_end - block_idx_begin);                \
        }                                                                      \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* Check if the current PE should wait/send data to the peer */            \
    if (me_p2s == -1) {                                                        \
      /* Wait until the peer PE sends the data */                              \
      shmem_long_wait_until(pSync + 1, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + 1);   \
      shmem_long_p(pSync + 1, SHCOLL_SYNC_VALUE, me);                          \
    } else if ((me_as + 1) * p2s_size / PE_size == me_p2s) {                   \
      peer = PE_start + (me_as + 1) * stride;                                  \
      shmem_putmem(dest, dest, nelems * sizeof(_type), peer);                  \
      shmem_fence();                                                           \
      shmem_long_p(pSync + 1, SHCOLL_SYNC_VALUE + 1, peer);                    \
    }                                                                          \
                                                                               \
    if (tmp_array != NULL) {                                                   \
      free(tmp_array);                                                         \
    }                                                                          \
  }

/**
 * @brief Helper macro to define Rabenseifner reduction operations
 *
 * This function implements the Rabenseifner's algorithm for reducing data
 * across all PEs in a team. It uses a parallel reduction approach to
 * efficiently compute the desired operation (e.g., sum, product, etc.)
 * on the input data.
 */
#define REDUCE_HELPER_RABENSEIFNER2(_name, _type, _op)                         \
  void reduce_helper_##_name##_rabenseifner2(                                  \
      _type *dest, const _type *source, int nreduce, int PE_start,             \
      int logPE_stride, int PE_size, _type *pWrk, long *pSync) {               \
    const int stride = 1 << logPE_stride;                                      \
    const int me = shmem_my_pe();                                              \
                                                                               \
    int me_as = (me - PE_start) / stride;                                      \
    int peer;                                                                  \
    size_t i;                                                                  \
                                                                               \
    const size_t nelems = (const size_t)nreduce;                               \
    int block_idx_begin;                                                       \
    int block_idx_end;                                                         \
                                                                               \
    ptrdiff_t block_offset;                                                    \
    ptrdiff_t next_block_offset;                                               \
    size_t block_nelems;                                                       \
                                                                               \
    int xchg_peer_p2s;                                                         \
    int xchg_peer_as;                                                          \
    int xchg_peer_pe;                                                          \
                                                                               \
    int ring_peer_p2s;                                                         \
    int ring_peer_as;                                                          \
    int ring_peer_pe;                                                          \
                                                                               \
    /* Power 2 set */                                                          \
    int me_p2s;                                                                \
    int p2s_size;                                                              \
    int log_p2s_size;                                                          \
                                                                               \
    int distance;                                                              \
    _type *tmp_array = NULL;                                                   \
                                                                               \
    long *collect_pSync = pSync + (1 + sizeof(int) * CHAR_BIT);                \
                                                                               \
    /* Find the greatest power of 2 lower than PE_size */                      \
    for (p2s_size = 1, log_p2s_size = 0; p2s_size * 2 <= PE_size;              \
         p2s_size *= 2, log_p2s_size++)                                        \
      ;                                                                        \
                                                                               \
    /* Check if the current PE belongs to the power 2 set */                   \
    me_p2s = me_as * p2s_size / PE_size;                                       \
    if ((me_p2s * PE_size + p2s_size - 1) / p2s_size != me_as) {               \
      me_p2s = -1;                                                             \
    }                                                                          \
                                                                               \
    /* If current PE belongs to the power 2 set, it will need temporary buffer \
     */                                                                        \
    if (me_p2s != -1) {                                                        \
      tmp_array = malloc((nelems / 2 + 1) * sizeof(_type));                    \
      if (tmp_array == NULL) {                                                 \
        /* TODO: raise error */                                                \
        fprintf(stderr, "PE %d: Cannot allocate memory!\n", me);               \
        exit(-1);                                                              \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* Check if the current PE should wait/send data to the peer */            \
    if (me_p2s == -1) {                                                        \
      /* Notify peer that the data is ready */                                 \
      peer = PE_start + (me_as - 1) * stride;                                  \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE + 1, peer);                        \
                                                                               \
      /* Wait until the data on peer node is ready and get the data (upper     \
       * half of the array) */                                                 \
      block_offset = nelems / 2;                                               \
      block_nelems = (size_t)(nelems - block_offset);                          \
                                                                               \
      shmem_long_wait_until(pSync, SHMEM_CMP_NE, SHCOLL_SYNC_VALUE);           \
      shmem_getmem(dest + block_offset, source + block_offset,                 \
                   block_nelems * sizeof(_type), peer);                        \
                                                                               \
      /* Reduce the upper half of the array */                                 \
      local_##_name##_reduce(dest + block_offset, dest + block_offset,         \
                             source + block_offset, block_nelems);             \
                                                                               \
      /* Send the upper half of the array to peer */                           \
      shmem_putmem(dest + block_offset, dest + block_offset,                   \
                   block_nelems * sizeof(_type), peer);                        \
      shmem_fence();                                                           \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE + 2, peer);                        \
    } else if ((me_as + 1) * p2s_size / PE_size == me_p2s) {                   \
      /* Notify peer that the data is ready */                                 \
      peer = PE_start + (me_as + 1) * stride;                                  \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE + 1, peer);                        \
                                                                               \
      /* Wait until the data on peer node is ready and get the data (lower     \
       * half of the array) */                                                 \
      block_offset = 0;                                                        \
      block_nelems = (size_t)(nelems / 2 - block_offset);                      \
                                                                               \
      shmem_long_wait_until(pSync, SHMEM_CMP_GT, SHCOLL_SYNC_VALUE);           \
      shmem_getmem(dest, source, block_nelems * sizeof(_type), peer);          \
                                                                               \
      /* Do local reduce */                                                    \
      local_##_name##_reduce(dest, dest, source, block_nelems);                \
                                                                               \
      /* Wait until the upper half is received from peer */                    \
      shmem_long_wait_until(pSync, SHMEM_CMP_GT, SHCOLL_SYNC_VALUE + 1);       \
      shmem_long_p(pSync, SHCOLL_SYNC_VALUE, me);                              \
    } else {                                                                   \
      memcpy(dest, source, nelems * sizeof(_type));                            \
    }                                                                          \
                                                                               \
    /* For nodes in the power 2 set, dest contains data that should be reduced \
     */                                                                        \
                                                                               \
    /* Do reduce scatter with the nodes in power 2 set */                      \
    if (me_p2s != -1) {                                                        \
      block_idx_begin = 0;                                                     \
      block_idx_end = p2s_size;                                                \
                                                                               \
      for (distance = 1, i = 1; distance < p2s_size; distance <<= 1, i++) {    \
        xchg_peer_p2s = ((me_p2s & distance) == 0) ? me_p2s + distance         \
                                                   : me_p2s - distance;        \
        xchg_peer_as = (xchg_peer_p2s * PE_size + p2s_size - 1) / p2s_size;    \
        xchg_peer_pe = PE_start + xchg_peer_as * stride;                       \
                                                                               \
        /* Notify the peer PE that the data is ready to be read */             \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE + 1, xchg_peer_pe);          \
                                                                               \
        /* Check if the current PE is responsible for lower half of upper half \
         * of the vector */                                                    \
        if ((me_p2s & distance) == 0) {                                        \
          block_idx_end = (block_idx_begin + block_idx_end) / 2;               \
        } else {                                                               \
          block_idx_begin = (block_idx_begin + block_idx_end) / 2;             \
        }                                                                      \
                                                                               \
        /* TODO: possible overflow */                                          \
        block_offset = (block_idx_begin * nelems) / p2s_size;                  \
        next_block_offset = (block_idx_end * nelems) / p2s_size;               \
        block_nelems = (size_t)(next_block_offset - block_offset);             \
                                                                               \
        /* Wait until the data on peer PE is ready to be read and get the data \
         */                                                                    \
        shmem_long_wait_until(pSync + i, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + 1); \
        shmem_getmem(tmp_array, dest + block_offset,                           \
                     block_nelems * sizeof(_type), xchg_peer_pe);              \
                                                                               \
        /* Notify the peer PE that the data transfer has completed             \
         * successfully */                                                     \
        shmem_fence();                                                         \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE + 2, xchg_peer_pe);          \
                                                                               \
        /* Do local reduce */                                                  \
        local_##_name##_reduce(dest + block_offset, dest + block_offset,       \
                               tmp_array, block_nelems);                       \
                                                                               \
        /* Wait until the peer PE has read the data */                         \
        shmem_long_wait_until(pSync + i, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + 2); \
        shmem_long_p(pSync + i, SHCOLL_SYNC_VALUE, me);                        \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* For nodes in the power 2 set, dest contains data that should be reduced \
     */                                                                        \
                                                                               \
    /* Do collect with the nodes in power 2 set */                             \
    if (me_p2s != -1) {                                                        \
      ring_peer_p2s = (me_p2s + 1) % p2s_size;                                 \
      ring_peer_as = (ring_peer_p2s * PE_size + p2s_size - 1) / p2s_size;      \
      ring_peer_pe = PE_start + ring_peer_as * stride;                         \
                                                                               \
      for (i = 0; i < p2s_size; i++) {                                         \
        block_idx_begin = reverse_bits(                                        \
            (int)((me_p2s - i + p2s_size) % p2s_size), log_p2s_size);          \
        block_idx_end = block_idx_begin + 1;                                   \
                                                                               \
        /* TODO: possible overflow */                                          \
        block_offset = (block_idx_begin * nelems) / p2s_size;                  \
        next_block_offset = (block_idx_end * nelems) / p2s_size;               \
        block_nelems = (size_t)(next_block_offset - block_offset);             \
                                                                               \
        shmem_putmem_nbi(dest + block_offset, dest + block_offset,             \
                         block_nelems * sizeof(_type), ring_peer_pe);          \
        shmem_fence();                                                         \
        shmem_long_p(collect_pSync, SHCOLL_SYNC_VALUE + i + 1, ring_peer_pe);  \
                                                                               \
        shmem_long_wait_until(collect_pSync, SHMEM_CMP_GT,                     \
                              SHCOLL_SYNC_VALUE + i);                          \
      }                                                                        \
                                                                               \
      shmem_long_p(collect_pSync, SHCOLL_SYNC_VALUE, me);                      \
    }                                                                          \
                                                                               \
    /* Check if the current PE should wait/send data to the peer */            \
    if (me_p2s == -1) {                                                        \
      /* Wait until the peer PE sends the data */                              \
      shmem_long_wait_until(pSync + 1, SHMEM_CMP_GE, SHCOLL_SYNC_VALUE + 1);   \
      shmem_long_p(pSync + 1, SHCOLL_SYNC_VALUE, me);                          \
    } else if ((me_as + 1) * p2s_size / PE_size == me_p2s) {                   \
      peer = PE_start + (me_as + 1) * stride;                                  \
      shmem_putmem(dest, dest, nelems * sizeof(_type), peer);                  \
      shmem_fence();                                                           \
      shmem_long_p(pSync + 1, SHCOLL_SYNC_VALUE + 1, peer);                    \
    }                                                                          \
                                                                               \
    if (tmp_array != NULL) {                                                   \
      free(tmp_array);                                                         \
    }                                                                          \
  }

/*
 * Supported reduction operations
 */

#define AND_OP(A, B) ((A) & (B))
#define MAX_OP(A, B) ((A) > (B) ? (A) : (B))
#define MIN_OP(A, B) ((A) < (B) ? (A) : (B))
#define SUM_OP(A, B) ((A) + (B))
#define PROD_OP(A, B) ((A) * (B))
#define OR_OP(A, B) ((A) | (B))
#define XOR_OP(A, B) ((A) ^ (B))

/*
 * Definitions for all reductions
 */

/**

  XXX: should i just add the additional types used in the team-based reductions
  or write separate subroutines?

 */
/**
 * @file reduce.c
 * @brief Collective reduction operations for OpenSHMEM
 *
 * This file implements various reduction operations (AND, OR, XOR, MAX, MIN,
 * SUM, PROD) across multiple processing elements (PEs) using different
 * algorithms.
 */

/**
 * @brief Macro to define reduction operations for all supported types
 *
 * This macro expands to define reduction operations for all combinations of:
 * - Data types (char, int, float, etc.)
 * - Operations (AND, OR, XOR, MAX, MIN, SUM, PROD)
 * - Algorithms (linear, binomial, recursive doubling, Rabenseifner)
 *
 * @param _name The name of the reduction helper function to define
 */

/* @formatter: off */
// clang-format off
#define SHCOLL_TO_ALL_DEFINE(_name)                                           \
  /* AND operation */                                                         \
  _name(char_and, char, AND_OP)                                               \
  _name(schar_and, signed char, AND_OP)                                       \
  _name(short_and, short, AND_OP)                                             \
  _name(int_and, int, AND_OP)                                                 \
  _name(long_and, long, AND_OP)                                               \
  _name(longlong_and, long long, AND_OP)                                      \
  _name(ptrdiff_and, ptrdiff_t, AND_OP)                                       \
  _name(uchar_and, unsigned char, AND_OP)                                     \
  _name(ushort_and, unsigned short, AND_OP)                                   \
  _name(uint_and, unsigned int, AND_OP)                                       \
  _name(ulong_and, unsigned long, AND_OP)                                     \
  _name(ulonglong_and, unsigned long long, AND_OP)                            \
  _name(int8_and, int8_t, AND_OP)                                             \
  _name(int16_and, int16_t, AND_OP)                                           \
  _name(int32_and, int32_t, AND_OP)                                           \
  _name(int64_and, int64_t, AND_OP)                                           \
  _name(uint8_and, uint8_t, AND_OP)                                           \
  _name(uint16_and, uint16_t, AND_OP)                                         \
  _name(uint32_and, uint32_t, AND_OP)                                         \
  _name(uint64_and, uint64_t, AND_OP)                                         \
  _name(size_and, size_t, AND_OP)                                             \
                                                                              \
  /* OR operation */                                                          \
  _name(char_or, char, OR_OP)                                                 \
  _name(schar_or, signed char, OR_OP)                                         \
  _name(short_or, short, OR_OP)                                               \
  _name(int_or, int, OR_OP)                                                   \
  _name(long_or, long, OR_OP)                                                 \
  _name(longlong_or, long long, OR_OP)                                        \
  _name(ptrdiff_or, ptrdiff_t, OR_OP)                                         \
  _name(uchar_or, unsigned char, OR_OP)                                       \
  _name(ushort_or, unsigned short, OR_OP)                                     \
  _name(uint_or, unsigned int, OR_OP)                                         \
  _name(ulong_or, unsigned long, OR_OP)                                       \
  _name(ulonglong_or, unsigned long long, OR_OP)                              \
  _name(int8_or, int8_t, OR_OP)                                               \
  _name(int16_or, int16_t, OR_OP)                                             \
  _name(int32_or, int32_t, OR_OP)                                             \
  _name(int64_or, int64_t, OR_OP)                                             \
  _name(uint8_or, uint8_t, OR_OP)                                             \
  _name(uint16_or, uint16_t, OR_OP)                                           \
  _name(uint32_or, uint32_t, OR_OP)                                           \
  _name(uint64_or, uint64_t, OR_OP)                                           \
  _name(size_or, size_t, OR_OP)                                               \
                                                                              \
  /* XOR operation */                                                         \
  _name(char_xor, char, XOR_OP)                                               \
  _name(schar_xor, signed char, XOR_OP)                                       \
  _name(short_xor, short, XOR_OP)                                             \
  _name(int_xor, int, XOR_OP)                                                 \
  _name(long_xor, long, XOR_OP)                                               \
  _name(longlong_xor, long long, XOR_OP)                                      \
  _name(ptrdiff_xor, ptrdiff_t, XOR_OP)                                       \
  _name(uchar_xor, unsigned char, XOR_OP)                                     \
  _name(ushort_xor, unsigned short, XOR_OP)                                   \
  _name(uint_xor, unsigned int, XOR_OP)                                       \
  _name(ulong_xor, unsigned long, XOR_OP)                                     \
  _name(ulonglong_xor, unsigned long long, XOR_OP)                            \
  _name(int8_xor, int8_t, XOR_OP)                                             \
  _name(int16_xor, int16_t, XOR_OP)                                           \
  _name(int32_xor, int32_t, XOR_OP)                                           \
  _name(int64_xor, int64_t, XOR_OP)                                           \
  _name(uint8_xor, uint8_t, XOR_OP)                                           \
  _name(uint16_xor, uint16_t, XOR_OP)                                         \
  _name(uint32_xor, uint32_t, XOR_OP)                                         \
  _name(uint64_xor, uint64_t, XOR_OP)                                         \
  _name(size_xor, size_t, XOR_OP)                                             \
                                                                              \
  /* MAX operation */                                                         \
  _name(char_max, char, MAX_OP)                                               \
  _name(schar_max, signed char, MAX_OP)                                       \
  _name(short_max, short, MAX_OP)                                             \
  _name(int_max, int, MAX_OP)                                                 \
  _name(long_max, long, MAX_OP)                                               \
  _name(longlong_max, long long, MAX_OP)                                      \
  _name(ptrdiff_max, ptrdiff_t, MAX_OP)                                       \
  _name(uchar_max, unsigned char, MAX_OP)                                     \
  _name(ushort_max, unsigned short, MAX_OP)                                   \
  _name(uint_max, unsigned int, MAX_OP)                                       \
  _name(ulong_max, unsigned long, MAX_OP)                                     \
  _name(ulonglong_max, unsigned long long, MAX_OP)                            \
  _name(int8_max, int8_t, MAX_OP)                                             \
  _name(int16_max, int16_t, MAX_OP)                                           \
  _name(int32_max, int32_t, MAX_OP)                                           \
  _name(int64_max, int64_t, MAX_OP)                                           \
  _name(uint8_max, uint8_t, MAX_OP)                                           \
  _name(uint16_max, uint16_t, MAX_OP)                                         \
  _name(uint32_max, uint32_t, MAX_OP)                                         \
  _name(uint64_max, uint64_t, MAX_OP)                                         \
  _name(size_max, size_t, MAX_OP)                                             \
  _name(float_max, float, MAX_OP)                                             \
  _name(double_max, double, MAX_OP)                                           \
  _name(longdouble_max, long double, MAX_OP)                                  \
                                                                              \
  /* MIN operation */                                                         \
  _name(char_min, char, MIN_OP)                                               \
  _name(schar_min, signed char, MIN_OP)                                       \
  _name(short_min, short, MIN_OP)                                             \
  _name(int_min, int, MIN_OP)                                                 \
  _name(long_min, long, MIN_OP)                                               \
  _name(longlong_min, long long, MIN_OP)                                      \
  _name(ptrdiff_min, ptrdiff_t, MIN_OP)                                       \
  _name(uchar_min, unsigned char, MIN_OP)                                     \
  _name(ushort_min, unsigned short, MIN_OP)                                   \
  _name(uint_min, unsigned int, MIN_OP)                                       \
  _name(ulong_min, unsigned long, MIN_OP)                                     \
  _name(ulonglong_min, unsigned long long, MIN_OP)                            \
  _name(int8_min, int8_t, MIN_OP)                                             \
  _name(int16_min, int16_t, MIN_OP)                                           \
  _name(int32_min, int32_t, MIN_OP)                                           \
  _name(int64_min, int64_t, MIN_OP)                                           \
  _name(uint8_min, uint8_t, MIN_OP)                                           \
  _name(uint16_min, uint16_t, MIN_OP)                                         \
  _name(uint32_min, uint32_t, MIN_OP)                                         \
  _name(uint64_min, uint64_t, MIN_OP)                                         \
  _name(size_min, size_t, MIN_OP)                                             \
  _name(float_min, float, MIN_OP)                                             \
  _name(double_min, double, MIN_OP)                                           \
  _name(longdouble_min, long double, MIN_OP)                                  \
                                                                              \
  /* SUM operation */                                                         \
  _name(char_sum, char, SUM_OP)                                               \
  _name(schar_sum, signed char, SUM_OP)                                       \
  _name(short_sum, short, SUM_OP)                                             \
  _name(int_sum, int, SUM_OP)                                                 \
  _name(long_sum, long, SUM_OP)                                               \
  _name(longlong_sum, long long, SUM_OP)                                      \
  _name(ptrdiff_sum, ptrdiff_t, SUM_OP)                                       \
  _name(uchar_sum, unsigned char, SUM_OP)                                     \
  _name(ushort_sum, unsigned short, SUM_OP)                                   \
  _name(uint_sum, unsigned int, SUM_OP)                                       \
  _name(ulong_sum, unsigned long, SUM_OP)                                     \
  _name(ulonglong_sum, unsigned long long, SUM_OP)                            \
  _name(int8_sum, int8_t, SUM_OP)                                             \
  _name(int16_sum, int16_t, SUM_OP)                                           \
  _name(int32_sum, int32_t, SUM_OP)                                           \
  _name(int64_sum, int64_t, SUM_OP)                                           \
  _name(uint8_sum, uint8_t, SUM_OP)                                           \
  _name(uint16_sum, uint16_t, SUM_OP)                                         \
  _name(uint32_sum, uint32_t, SUM_OP)                                         \
  _name(uint64_sum, uint64_t, SUM_OP)                                         \
  _name(size_sum, size_t, SUM_OP)                                             \
  _name(float_sum, float, SUM_OP)                                             \
  _name(double_sum, double, SUM_OP)                                           \
  _name(longdouble_sum, long double, SUM_OP)                                  \
  _name(complexf_sum, float _Complex, SUM_OP)                                 \
  _name(complexd_sum, double _Complex, SUM_OP)                                \
  /* PROD operation */                                                      \
  _name(char_prod, char, PROD_OP)                                           \
  _name(schar_prod, signed char, PROD_OP)                                   \
  _name(short_prod, short, PROD_OP)                                         \
  _name(int_prod, int, PROD_OP)                                             \
  _name(long_prod, long, PROD_OP)                                           \
  _name(longlong_prod, long long, PROD_OP)                                  \
  _name(ptrdiff_prod, ptrdiff_t, PROD_OP)                                   \
  _name(uchar_prod, unsigned char, PROD_OP)                                 \
  _name(ushort_prod, unsigned short, PROD_OP)                               \
  _name(uint_prod, unsigned int, PROD_OP)                                   \
  _name(ulong_prod, unsigned long, PROD_OP)                                 \
  _name(ulonglong_prod, unsigned long long, PROD_OP)                        \
  _name(int8_prod, int8_t, PROD_OP)                                         \
  _name(int16_prod, int16_t, PROD_OP)                                       \
  _name(int32_prod, int32_t, PROD_OP)                                       \
  _name(int64_prod, int64_t, PROD_OP)                                       \
  _name(uint8_prod, uint8_t, PROD_OP)                                       \
  _name(uint16_prod, uint16_t, PROD_OP)                                     \
  _name(uint32_prod, uint32_t, PROD_OP)                                     \
  _name(uint64_prod, uint64_t, PROD_OP)                                     \
  _name(size_prod, size_t, PROD_OP)                                         \
  _name(float_prod, float, PROD_OP)                                         \
  _name(double_prod, double, PROD_OP)                                       \
  _name(longdouble_prod, long double, PROD_OP)                              \
  _name(complexf_prod, float _Complex, PROD_OP)                             \
  _name(complexd_prod, double _Complex, PROD_OP)
// clang-format on

/* Generate all implementations for all types and operations */
SHCOLL_TO_ALL_DEFINE(REDUCE_HELPER_LOCAL)
SHCOLL_TO_ALL_DEFINE(REDUCE_HELPER_LINEAR)
SHCOLL_TO_ALL_DEFINE(REDUCE_HELPER_BINOMIAL)
SHCOLL_TO_ALL_DEFINE(REDUCE_HELPER_REC_DBL)
SHCOLL_TO_ALL_DEFINE(REDUCE_HELPER_RABENSEIFNER)
SHCOLL_TO_ALL_DEFINE(REDUCE_HELPER_RABENSEIFNER2)

/* @formatter:on */
// clang-format on

/*
 * @brief Macro to define the reduction operation wrapper function
 *
 * @param _typename_op Combined type and operation name (e.g. char_and)
 * @param _type       Actual C type (e.g. char)
 * @param _algo  Algorithm suffix (e.g. linear)
 */
#define SHCOLL_TO_ALL_DEFINITION(_typename_op, _type, _algo)                   \
  void shcoll_##_typename_op##_to_all_##_algo(                                 \
      _type *dest, const _type *source, int nreduce, int PE_start,             \
      int logPE_stride, int PE_size, _type *pWrk, long *pSync) {               \
    /* Sanity Checks */                                                        \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_POSITIVE(PE_size, "PE_size");                                 \
    SHMEMU_CHECK_NON_NEGATIVE(PE_start, "PE_start");                           \
    SHMEMU_CHECK_NON_NEGATIVE(logPE_stride, "logPE_stride");                   \
    SHMEMU_CHECK_ACTIVE_SET_RANGE(PE_start, logPE_stride, PE_size);            \
    SHMEMU_CHECK_NULL(dest, "dest");                                           \
    SHMEMU_CHECK_NULL(source, "source");                                       \
    SHMEMU_CHECK_NULL(pWrk, "pWrk");                                           \
    SHMEMU_CHECK_NULL(pSync, "pSync");                                         \
    SHMEMU_CHECK_SYMMETRIC(dest, sizeof(_type) * nreduce);                     \
    SHMEMU_CHECK_SYMMETRIC(source, sizeof(_type) * nreduce);                   \
    SHMEMU_CHECK_SYMMETRIC(pWrk,                                               \
                           sizeof(_type) * SHCOLL_REDUCE_MIN_WRKDATA_SIZE);    \
    SHMEMU_CHECK_SYMMETRIC(pSync, sizeof(long) * SHCOLL_REDUCE_SYNC_SIZE);     \
    SHMEMU_CHECK_BUFFER_OVERLAP(dest, source, sizeof(_type) * nreduce,         \
                                sizeof(_type) * nreduce);                      \
    /* dispatch into the helper routine */                                     \
    reduce_helper_##_typename_op##_##_algo(                                    \
        dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);  \
  }

/*
 * @brief Improved wrapper macros for each reduction algorithm
 */
#define TO_ALL_WRAPPER(_name, _type, _op, _algo)                               \
  SHCOLL_TO_ALL_DEFINITION(_name, _type, _algo)

/* Group by operation type, similar to SHIM_REDUCE approach */
#define TO_ALL_WRAPPER_BITWISE(_algo)                                          \
  /* AND operation */                                                          \
  TO_ALL_WRAPPER(char_and, char, AND_OP, _algo)                                \
  TO_ALL_WRAPPER(schar_and, signed char, AND_OP, _algo)                        \
  TO_ALL_WRAPPER(short_and, short, AND_OP, _algo)                              \
  TO_ALL_WRAPPER(int_and, int, AND_OP, _algo)                                  \
  TO_ALL_WRAPPER(long_and, long, AND_OP, _algo)                                \
  TO_ALL_WRAPPER(longlong_and, long long, AND_OP, _algo)                       \
  TO_ALL_WRAPPER(ptrdiff_and, ptrdiff_t, AND_OP, _algo)                        \
  TO_ALL_WRAPPER(uchar_and, unsigned char, AND_OP, _algo)                      \
  TO_ALL_WRAPPER(ushort_and, unsigned short, AND_OP, _algo)                    \
  TO_ALL_WRAPPER(uint_and, unsigned int, AND_OP, _algo)                        \
  TO_ALL_WRAPPER(ulong_and, unsigned long, AND_OP, _algo)                      \
  TO_ALL_WRAPPER(ulonglong_and, unsigned long long, AND_OP, _algo)             \
  TO_ALL_WRAPPER(int8_and, int8_t, AND_OP, _algo)                              \
  TO_ALL_WRAPPER(int16_and, int16_t, AND_OP, _algo)                            \
  TO_ALL_WRAPPER(int32_and, int32_t, AND_OP, _algo)                            \
  TO_ALL_WRAPPER(int64_and, int64_t, AND_OP, _algo)                            \
  TO_ALL_WRAPPER(uint8_and, uint8_t, AND_OP, _algo)                            \
  TO_ALL_WRAPPER(uint16_and, uint16_t, AND_OP, _algo)                          \
  TO_ALL_WRAPPER(uint32_and, uint32_t, AND_OP, _algo)                          \
  TO_ALL_WRAPPER(uint64_and, uint64_t, AND_OP, _algo)                          \
  TO_ALL_WRAPPER(size_and, size_t, AND_OP, _algo)                              \
  /* OR operation */                                                           \
  TO_ALL_WRAPPER(char_or, char, OR_OP, _algo)                                  \
  TO_ALL_WRAPPER(schar_or, signed char, OR_OP, _algo)                          \
  TO_ALL_WRAPPER(short_or, short, OR_OP, _algo)                                \
  TO_ALL_WRAPPER(int_or, int, OR_OP, _algo)                                    \
  TO_ALL_WRAPPER(long_or, long, OR_OP, _algo)                                  \
  TO_ALL_WRAPPER(longlong_or, long long, OR_OP, _algo)                         \
  TO_ALL_WRAPPER(ptrdiff_or, ptrdiff_t, OR_OP, _algo)                          \
  TO_ALL_WRAPPER(uchar_or, unsigned char, OR_OP, _algo)                        \
  TO_ALL_WRAPPER(ushort_or, unsigned short, OR_OP, _algo)                      \
  TO_ALL_WRAPPER(uint_or, unsigned int, OR_OP, _algo)                          \
  TO_ALL_WRAPPER(ulong_or, unsigned long, OR_OP, _algo)                        \
  TO_ALL_WRAPPER(ulonglong_or, unsigned long long, OR_OP, _algo)               \
  TO_ALL_WRAPPER(int8_or, int8_t, OR_OP, _algo)                                \
  TO_ALL_WRAPPER(int16_or, int16_t, OR_OP, _algo)                              \
  TO_ALL_WRAPPER(int32_or, int32_t, OR_OP, _algo)                              \
  TO_ALL_WRAPPER(int64_or, int64_t, OR_OP, _algo)                              \
  TO_ALL_WRAPPER(uint8_or, uint8_t, OR_OP, _algo)                              \
  TO_ALL_WRAPPER(uint16_or, uint16_t, OR_OP, _algo)                            \
  TO_ALL_WRAPPER(uint32_or, uint32_t, OR_OP, _algo)                            \
  TO_ALL_WRAPPER(uint64_or, uint64_t, OR_OP, _algo)                            \
  TO_ALL_WRAPPER(size_or, size_t, OR_OP, _algo)                                \
  /* XOR operation */                                                          \
  TO_ALL_WRAPPER(char_xor, char, XOR_OP, _algo)                                \
  TO_ALL_WRAPPER(schar_xor, signed char, XOR_OP, _algo)                        \
  TO_ALL_WRAPPER(short_xor, short, XOR_OP, _algo)                              \
  TO_ALL_WRAPPER(int_xor, int, XOR_OP, _algo)                                  \
  TO_ALL_WRAPPER(long_xor, long, XOR_OP, _algo)                                \
  TO_ALL_WRAPPER(longlong_xor, long long, XOR_OP, _algo)                       \
  TO_ALL_WRAPPER(ptrdiff_xor, ptrdiff_t, XOR_OP, _algo)                        \
  TO_ALL_WRAPPER(uchar_xor, unsigned char, XOR_OP, _algo)                      \
  TO_ALL_WRAPPER(ushort_xor, unsigned short, XOR_OP, _algo)                    \
  TO_ALL_WRAPPER(uint_xor, unsigned int, XOR_OP, _algo)                        \
  TO_ALL_WRAPPER(ulong_xor, unsigned long, XOR_OP, _algo)                      \
  TO_ALL_WRAPPER(ulonglong_xor, unsigned long long, XOR_OP, _algo)             \
  TO_ALL_WRAPPER(int8_xor, int8_t, XOR_OP, _algo)                              \
  TO_ALL_WRAPPER(int16_xor, int16_t, XOR_OP, _algo)                            \
  TO_ALL_WRAPPER(int32_xor, int32_t, XOR_OP, _algo)                            \
  TO_ALL_WRAPPER(int64_xor, int64_t, XOR_OP, _algo)                            \
  TO_ALL_WRAPPER(uint8_xor, uint8_t, XOR_OP, _algo)                            \
  TO_ALL_WRAPPER(uint16_xor, uint16_t, XOR_OP, _algo)                          \
  TO_ALL_WRAPPER(uint32_xor, uint32_t, XOR_OP, _algo)                          \
  TO_ALL_WRAPPER(uint64_xor, uint64_t, XOR_OP, _algo)                          \
  TO_ALL_WRAPPER(size_xor, size_t, XOR_OP, _algo)

#define TO_ALL_WRAPPER_MINMAX(_algo)                                           \
  /* MAX operation */                                                          \
  TO_ALL_WRAPPER(char_max, char, MAX_OP, _algo)                                \
  TO_ALL_WRAPPER(schar_max, signed char, MAX_OP, _algo)                        \
  TO_ALL_WRAPPER(short_max, short, MAX_OP, _algo)                              \
  TO_ALL_WRAPPER(int_max, int, MAX_OP, _algo)                                  \
  TO_ALL_WRAPPER(long_max, long, MAX_OP, _algo)                                \
  TO_ALL_WRAPPER(longlong_max, long long, MAX_OP, _algo)                       \
  TO_ALL_WRAPPER(ptrdiff_max, ptrdiff_t, MAX_OP, _algo)                        \
  TO_ALL_WRAPPER(uchar_max, unsigned char, MAX_OP, _algo)                      \
  TO_ALL_WRAPPER(ushort_max, unsigned short, MAX_OP, _algo)                    \
  TO_ALL_WRAPPER(uint_max, unsigned int, MAX_OP, _algo)                        \
  TO_ALL_WRAPPER(ulong_max, unsigned long, MAX_OP, _algo)                      \
  TO_ALL_WRAPPER(ulonglong_max, unsigned long long, MAX_OP, _algo)             \
  TO_ALL_WRAPPER(int8_max, int8_t, MAX_OP, _algo)                              \
  TO_ALL_WRAPPER(int16_max, int16_t, MAX_OP, _algo)                            \
  TO_ALL_WRAPPER(int32_max, int32_t, MAX_OP, _algo)                            \
  TO_ALL_WRAPPER(int64_max, int64_t, MAX_OP, _algo)                            \
  TO_ALL_WRAPPER(uint8_max, uint8_t, MAX_OP, _algo)                            \
  TO_ALL_WRAPPER(uint16_max, uint16_t, MAX_OP, _algo)                          \
  TO_ALL_WRAPPER(uint32_max, uint32_t, MAX_OP, _algo)                          \
  TO_ALL_WRAPPER(uint64_max, uint64_t, MAX_OP, _algo)                          \
  TO_ALL_WRAPPER(size_max, size_t, MAX_OP, _algo)                              \
  TO_ALL_WRAPPER(float_max, float, MAX_OP, _algo)                              \
  TO_ALL_WRAPPER(double_max, double, MAX_OP, _algo)                            \
  TO_ALL_WRAPPER(longdouble_max, long double, MAX_OP, _algo)                   \
  /* MIN operation */                                                          \
  TO_ALL_WRAPPER(char_min, char, MIN_OP, _algo)                                \
  TO_ALL_WRAPPER(schar_min, signed char, MIN_OP, _algo)                        \
  TO_ALL_WRAPPER(short_min, short, MIN_OP, _algo)                              \
  TO_ALL_WRAPPER(int_min, int, MIN_OP, _algo)                                  \
  TO_ALL_WRAPPER(long_min, long, MIN_OP, _algo)                                \
  TO_ALL_WRAPPER(longlong_min, long long, MIN_OP, _algo)                       \
  TO_ALL_WRAPPER(ptrdiff_min, ptrdiff_t, MIN_OP, _algo)                        \
  TO_ALL_WRAPPER(uchar_min, unsigned char, MIN_OP, _algo)                      \
  TO_ALL_WRAPPER(ushort_min, unsigned short, MIN_OP, _algo)                    \
  TO_ALL_WRAPPER(uint_min, unsigned int, MIN_OP, _algo)                        \
  TO_ALL_WRAPPER(ulong_min, unsigned long, MIN_OP, _algo)                      \
  TO_ALL_WRAPPER(ulonglong_min, unsigned long long, MIN_OP, _algo)             \
  TO_ALL_WRAPPER(int8_min, int8_t, MIN_OP, _algo)                              \
  TO_ALL_WRAPPER(int16_min, int16_t, MIN_OP, _algo)                            \
  TO_ALL_WRAPPER(int32_min, int32_t, MIN_OP, _algo)                            \
  TO_ALL_WRAPPER(int64_min, int64_t, MIN_OP, _algo)                            \
  TO_ALL_WRAPPER(uint8_min, uint8_t, MIN_OP, _algo)                            \
  TO_ALL_WRAPPER(uint16_min, uint16_t, MIN_OP, _algo)                          \
  TO_ALL_WRAPPER(uint32_min, uint32_t, MIN_OP, _algo)                          \
  TO_ALL_WRAPPER(uint64_min, uint64_t, MIN_OP, _algo)                          \
  TO_ALL_WRAPPER(size_min, size_t, MIN_OP, _algo)                              \
  TO_ALL_WRAPPER(float_min, float, MIN_OP, _algo)                              \
  TO_ALL_WRAPPER(double_min, double, MIN_OP, _algo)                            \
  TO_ALL_WRAPPER(longdouble_min, long double, MIN_OP, _algo)

#define TO_ALL_WRAPPER_ARITH(_algo)                                            \
  /* SUM operation */                                                          \
  TO_ALL_WRAPPER(char_sum, char, SUM_OP, _algo)                                \
  TO_ALL_WRAPPER(schar_sum, signed char, SUM_OP, _algo)                        \
  TO_ALL_WRAPPER(short_sum, short, SUM_OP, _algo)                              \
  TO_ALL_WRAPPER(int_sum, int, SUM_OP, _algo)                                  \
  TO_ALL_WRAPPER(long_sum, long, SUM_OP, _algo)                                \
  TO_ALL_WRAPPER(longlong_sum, long long, SUM_OP, _algo)                       \
  TO_ALL_WRAPPER(ptrdiff_sum, ptrdiff_t, SUM_OP, _algo)                        \
  TO_ALL_WRAPPER(uchar_sum, unsigned char, SUM_OP, _algo)                      \
  TO_ALL_WRAPPER(ushort_sum, unsigned short, SUM_OP, _algo)                    \
  TO_ALL_WRAPPER(uint_sum, unsigned int, SUM_OP, _algo)                        \
  TO_ALL_WRAPPER(ulong_sum, unsigned long, SUM_OP, _algo)                      \
  TO_ALL_WRAPPER(ulonglong_sum, unsigned long long, SUM_OP, _algo)             \
  TO_ALL_WRAPPER(int8_sum, int8_t, SUM_OP, _algo)                              \
  TO_ALL_WRAPPER(int16_sum, int16_t, SUM_OP, _algo)                            \
  TO_ALL_WRAPPER(int32_sum, int32_t, SUM_OP, _algo)                            \
  TO_ALL_WRAPPER(int64_sum, int64_t, SUM_OP, _algo)                            \
  TO_ALL_WRAPPER(uint8_sum, uint8_t, SUM_OP, _algo)                            \
  TO_ALL_WRAPPER(uint16_sum, uint16_t, SUM_OP, _algo)                          \
  TO_ALL_WRAPPER(uint32_sum, uint32_t, SUM_OP, _algo)                          \
  TO_ALL_WRAPPER(uint64_sum, uint64_t, SUM_OP, _algo)                          \
  TO_ALL_WRAPPER(size_sum, size_t, SUM_OP, _algo)                              \
  TO_ALL_WRAPPER(float_sum, float, SUM_OP, _algo)                              \
  TO_ALL_WRAPPER(double_sum, double, SUM_OP, _algo)                            \
  TO_ALL_WRAPPER(longdouble_sum, long double, SUM_OP, _algo)                   \
  TO_ALL_WRAPPER(complexf_sum, float _Complex, SUM_OP, _algo)                  \
  TO_ALL_WRAPPER(complexd_sum, double _Complex, SUM_OP, _algo)                 \
  /* PROD operation */                                                         \
  TO_ALL_WRAPPER(char_prod, char, PROD_OP, _algo)                              \
  TO_ALL_WRAPPER(schar_prod, signed char, PROD_OP, _algo)                      \
  TO_ALL_WRAPPER(short_prod, short, PROD_OP, _algo)                            \
  TO_ALL_WRAPPER(int_prod, int, PROD_OP, _algo)                                \
  TO_ALL_WRAPPER(long_prod, long, PROD_OP, _algo)                              \
  TO_ALL_WRAPPER(longlong_prod, long long, PROD_OP, _algo)                     \
  TO_ALL_WRAPPER(ptrdiff_prod, ptrdiff_t, PROD_OP, _algo)                      \
  TO_ALL_WRAPPER(uchar_prod, unsigned char, PROD_OP, _algo)                    \
  TO_ALL_WRAPPER(ushort_prod, unsigned short, PROD_OP, _algo)                  \
  TO_ALL_WRAPPER(uint_prod, unsigned int, PROD_OP, _algo)                      \
  TO_ALL_WRAPPER(ulong_prod, unsigned long, PROD_OP, _algo)                    \
  TO_ALL_WRAPPER(ulonglong_prod, unsigned long long, PROD_OP, _algo)           \
  TO_ALL_WRAPPER(int8_prod, int8_t, PROD_OP, _algo)                            \
  TO_ALL_WRAPPER(int16_prod, int16_t, PROD_OP, _algo)                          \
  TO_ALL_WRAPPER(int32_prod, int32_t, PROD_OP, _algo)                          \
  TO_ALL_WRAPPER(int64_prod, int64_t, PROD_OP, _algo)                          \
  TO_ALL_WRAPPER(uint8_prod, uint8_t, PROD_OP, _algo)                          \
  TO_ALL_WRAPPER(uint16_prod, uint16_t, PROD_OP, _algo)                        \
  TO_ALL_WRAPPER(uint32_prod, uint32_t, PROD_OP, _algo)                        \
  TO_ALL_WRAPPER(uint64_prod, uint64_t, PROD_OP, _algo)                        \
  TO_ALL_WRAPPER(size_prod, size_t, PROD_OP, _algo)                            \
  TO_ALL_WRAPPER(float_prod, float, PROD_OP, _algo)                            \
  TO_ALL_WRAPPER(double_prod, double, PROD_OP, _algo)                          \
  TO_ALL_WRAPPER(longdouble_prod, long double, PROD_OP, _algo)                 \
  TO_ALL_WRAPPER(complexf_prod, float _Complex, PROD_OP, _algo)                \
  TO_ALL_WRAPPER(complexd_prod, double _Complex, PROD_OP, _algo)

/* Combine all operation types into one macro */
#define TO_ALL_WRAPPER_ALL(_algo)                                              \
  TO_ALL_WRAPPER_BITWISE(_algo)                                                \
  TO_ALL_WRAPPER_MINMAX(_algo)                                                 \
  TO_ALL_WRAPPER_ARITH(_algo)

/* generate wrappers for all types and ops for each algorithm */
TO_ALL_WRAPPER_ALL(linear)
TO_ALL_WRAPPER_ALL(binomial)
TO_ALL_WRAPPER_ALL(rec_dbl)
TO_ALL_WRAPPER_ALL(rabenseifner)
TO_ALL_WRAPPER_ALL(rabenseifner2)

/*
 * @brief Macro to define team-based reduction operations
 *
 * @param _typename Type name (e.g. int_sum)
 * @param _type Actual type (e.g. int)
 * @param _op Operation (e.g. sum)
 * @param _algo Algorithm name (e.g. linear)
 *
 *
 */
#define SHCOLL_REDUCE_DEFINITION(_typename, _type, _op, _algo)                 \
  int shcoll_##_typename##_##_op##_reduce_##_algo(                             \
      shmem_team_t team, _type *dest, const _type *source, size_t nreduce) {   \
    SHMEMU_CHECK_INIT();                                                       \
    SHMEMU_CHECK_TEAM_VALID(team);                                             \
    SHMEMU_CHECK_SYMMETRIC(dest, "dest");                                      \
    SHMEMU_CHECK_SYMMETRIC(source, "source");                                  \
    shmemc_team_h team_h = (shmemc_team_h)team;                                \
    int PE_start = team_h->start;                                              \
    int PE_size = team_h->nranks;                                              \
    int stride = team_h->stride;                                               \
    SHMEMU_CHECK_TEAM_STRIDE(stride, __func__);                                \
    int logPE_stride = (stride > 0) ? (int)log2((double)stride) : 0;           \
                                                                               \
    long *pSync = shmem_malloc(SHCOLL_REDUCE_SYNC_SIZE * sizeof(long));        \
    if (!pSync)                                                                \
      return -1;                                                               \
    for (int i = 0; i < SHCOLL_REDUCE_SYNC_SIZE; i++)                          \
      pSync[i] = SHCOLL_SYNC_VALUE;                                            \
    _type *pWrk =                                                              \
        shmem_malloc(SHCOLL_REDUCE_MIN_WRKDATA_SIZE * sizeof(_type));          \
    if (!pWrk) {                                                               \
      shmem_free(pSync);                                                       \
      return -1;                                                               \
    }                                                                          \
                                                                               \
    shmem_team_sync(team);                                                     \
                                                                               \
    /* call the type-specific to_all implementation */                         \
    shcoll_##_typename##_##_op##_to_all_##_algo(                               \
        dest, source, nreduce, PE_start, logPE_stride, PE_size, pWrk, pSync);  \
                                                                               \
    shmem_team_sync(team);                                                     \
    shmem_free(pWrk);                                                          \
    shmem_free(pSync);                                                         \
    return 0;                                                                  \
  }

#define SHIM_REDUCE_DECLARE(_typename, _type, _op, _algo)                      \
  SHCOLL_REDUCE_DEFINITION(_typename, _type, _op, _algo)

/*
 * @brief Macros to define reduction operations for different data types
 */

/* Bitwise reduction operations (AND, OR, XOR) */
#define SHIM_REDUCE_BITWISE_TYPES(_op, _algo)                                  \
  SHIM_REDUCE_DECLARE(uchar, unsigned char, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ushort, unsigned short, _op, _algo)                      \
  SHIM_REDUCE_DECLARE(uint, unsigned int, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(ulong, unsigned long, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ulonglong, unsigned long long, _op, _algo)               \
  SHIM_REDUCE_DECLARE(int8, int8_t, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(int16, int16_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int32, int32_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int64, int64_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint8, uint8_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint16, uint16_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint32, uint32_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint64, uint64_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(size, size_t, _op, _algo)

/* Min/Max reduction operations */
#define SHIM_REDUCE_MINMAX_TYPES(_op, _algo)                                   \
  SHIM_REDUCE_DECLARE(char, char, _op, _algo)                                  \
  SHIM_REDUCE_DECLARE(schar, signed char, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(short, short, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(int, int, _op, _algo)                                    \
  SHIM_REDUCE_DECLARE(long, long, _op, _algo)                                  \
  SHIM_REDUCE_DECLARE(longlong, long long, _op, _algo)                         \
  SHIM_REDUCE_DECLARE(ptrdiff, ptrdiff_t, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(uchar, unsigned char, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ushort, unsigned short, _op, _algo)                      \
  SHIM_REDUCE_DECLARE(uint, unsigned int, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(ulong, unsigned long, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ulonglong, unsigned long long, _op, _algo)               \
  SHIM_REDUCE_DECLARE(int8, int8_t, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(int16, int16_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int32, int32_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int64, int64_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint8, uint8_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint16, uint16_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint32, uint32_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint64, uint64_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(size, size_t, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(float, float, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(double, double, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(longdouble, long double, _op, _algo)

/* Arithmetic reduction operations (SUM, PROD) */
#define SHIM_REDUCE_ARITH_TYPES(_op, _algo)                                    \
  SHIM_REDUCE_DECLARE(char, char, _op, _algo)                                  \
  SHIM_REDUCE_DECLARE(schar, signed char, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(short, short, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(int, int, _op, _algo)                                    \
  SHIM_REDUCE_DECLARE(long, long, _op, _algo)                                  \
  SHIM_REDUCE_DECLARE(longlong, long long, _op, _algo)                         \
  SHIM_REDUCE_DECLARE(ptrdiff, ptrdiff_t, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(uchar, unsigned char, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ushort, unsigned short, _op, _algo)                      \
  SHIM_REDUCE_DECLARE(uint, unsigned int, _op, _algo)                          \
  SHIM_REDUCE_DECLARE(ulong, unsigned long, _op, _algo)                        \
  SHIM_REDUCE_DECLARE(ulonglong, unsigned long long, _op, _algo)               \
  SHIM_REDUCE_DECLARE(int8, int8_t, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(int16, int16_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int32, int32_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(int64, int64_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint8, uint8_t, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(uint16, uint16_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint32, uint32_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(uint64, uint64_t, _op, _algo)                            \
  SHIM_REDUCE_DECLARE(size, size_t, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(float, float, _op, _algo)                                \
  SHIM_REDUCE_DECLARE(double, double, _op, _algo)                              \
  SHIM_REDUCE_DECLARE(longdouble, long double, _op, _algo)                     \
  SHIM_REDUCE_DECLARE(complexf, float _Complex, _op, _algo)                    \
  SHIM_REDUCE_DECLARE(complexd, double _Complex, _op, _algo)

/*
 * @brief Grouping macros for each algorithm
 */
#define SHIM_REDUCE_BITWISE_ALL(_algo)                                         \
  SHIM_REDUCE_BITWISE_TYPES(or, _algo)                                         \
  SHIM_REDUCE_BITWISE_TYPES(xor, _algo)                                        \
  SHIM_REDUCE_BITWISE_TYPES(and, _algo)

#define SHIM_REDUCE_MINMAX_ALL(_algo)                                          \
  SHIM_REDUCE_MINMAX_TYPES(min, _algo)                                         \
  SHIM_REDUCE_MINMAX_TYPES(max, _algo)

#define SHIM_REDUCE_ARITH_ALL(_algo)                                           \
  SHIM_REDUCE_ARITH_TYPES(sum, _algo)                                          \
  SHIM_REDUCE_ARITH_TYPES(prod, _algo)

#define SHIM_REDUCE_ALL(_algo)                                                 \
  SHIM_REDUCE_BITWISE_ALL(_algo)                                               \
  SHIM_REDUCE_MINMAX_ALL(_algo)                                                \
  SHIM_REDUCE_ARITH_ALL(_algo)

/* Instantiate all shmem_*_reduce wrappers */
SHIM_REDUCE_ALL(linear)
SHIM_REDUCE_ALL(binomial)
SHIM_REDUCE_ALL(rec_dbl)
SHIM_REDUCE_ALL(rabenseifner)
SHIM_REDUCE_ALL(rabenseifner2)
