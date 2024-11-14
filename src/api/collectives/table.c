/**
 * @file table.c
 * @brief Implementation of collective operation registration and lookup tables
 *
 * This file contains the implementation of registration tables and functions
 * for OpenSHMEM collective operations. It provides mechanisms to register
 * different algorithms for collectives like alltoall, broadcast, collect etc.
 * and look them up at runtime.
 *
 * The tables support three types of collective operations:
 * - Sized operations (32/64 bit variants)
 * - Unsized operations (size independent)
 * - Typed operations (type specific implementations)
 *
 * @note The registration macros (SIZED_REG, UNSIZED_REG, etc.) are used to
 * populate the tables with the appropriate collective algorithm
 * implementations.
 *
 * @see shcoll.h for the actual collective algorithm implementations
 * @see table.h for the table structure definitions
 *
 * @license See LICENSE file at top-level
 */

#include "shcoll.h"
#include "table.h"

#include <stdio.h>
#include <string.h>

/******************************************************** */
/**
 * @brief Macro to register a sized collective operation with 32/64-bit variants
 * @param _op The collective operation name
 * @param _algo The algorithm implementation name
 */
#define SIZED_REG(_op, _algo)                                                  \
  { #_algo, shcoll_##_op##32##_##_algo, shcoll_##_op##64##_##_algo }

/**
 * @brief Macro to terminate a sized operation table
 */
#define SIZED_LAST                                                             \
  { "", NULL, NULL }

/**
 * @brief Macro to register an unsized collective operation
 * @param _op The collective operation name
 * @param _algo The algorithm implementation name
 */
#define UNSIZED_REG(_op, _algo)                                                \
  { #_algo, shcoll_##_op##_##_algo }

/**
 * @brief Macro to terminate an unsized operation table
 */
#define UNSIZED_LAST                                                           \
  { "", NULL }

/******************************************************** */
/**
 * @brief Macro to register a typed collective operation for a specific type
 * @param _op The collective operation name
 * @param _algo The algorithm implementation name
 * @param _typename The data type name
 */
#define TYPED_REG(_op, _algo, _typename)                                       \
  { #_algo, shcoll_##_typename##_##_op##_##_algo }

/**
 * @brief Macro to terminate a typed operation table
 */
#define TYPED_LAST                                                             \
  { "", NULL }

/**
 * @brief Macro to register a typed collective operation for all supported types
 * @param _op The collective operation name
 * @param _algo The algorithm implementation name
 */
#define TYPED_REG_FOR_ALL_TYPES(_op, _algo)                                    \
  TYPED_REG(_op, _algo, float), TYPED_REG(_op, _algo, double),                 \
      TYPED_REG(_op, _algo, longdouble), TYPED_REG(_op, _algo, char),          \
      TYPED_REG(_op, _algo, schar), TYPED_REG(_op, _algo, short),              \
      TYPED_REG(_op, _algo, int), TYPED_REG(_op, _algo, long),                 \
      TYPED_REG(_op, _algo, longlong), TYPED_REG(_op, _algo, uchar),           \
      TYPED_REG(_op, _algo, ushort), TYPED_REG(_op, _algo, uint),              \
      TYPED_REG(_op, _algo, ulong), TYPED_REG(_op, _algo, ulonglong),          \
      TYPED_REG(_op, _algo, int8), TYPED_REG(_op, _algo, int16),               \
      TYPED_REG(_op, _algo, int32), TYPED_REG(_op, _algo, int64),              \
      TYPED_REG(_op, _algo, uint8), TYPED_REG(_op, _algo, uint16),             \
      TYPED_REG(_op, _algo, uint32), TYPED_REG(_op, _algo, uint64),            \
      TYPED_REG(_op, _algo, size), TYPED_REG(_op, _algo, ptrdiff)

/**
 * @brief Macro to register an untyped collective operation
 * @param _op The collective operation name
 * @param _algo The algorithm implementation name
 */
#define UNTYPED_REG(_op, _algo)                                                \
  { #_algo, shcoll_##_op##_##_algo }

/**
 * @brief Macro to terminate an untyped operation table
 */
#define UNTYPED_LAST                                                           \
  { "", NULL }

/******************************************************** */
/**
 * @brief Table of alltoall collective algorithms for all types
 */
static typed_op_t alltoall_tab[] = {
    TYPED_REG_FOR_ALL_TYPES(alltoall, shift_exchange_barrier),
    TYPED_REG_FOR_ALL_TYPES(alltoall, shift_exchange_counter),
    TYPED_REG_FOR_ALL_TYPES(alltoall, shift_exchange_signal),
    TYPED_REG_FOR_ALL_TYPES(alltoall, xor_pairwise_exchange_barrier),
    TYPED_REG_FOR_ALL_TYPES(alltoall, xor_pairwise_exchange_counter),
    TYPED_REG_FOR_ALL_TYPES(alltoall, xor_pairwise_exchange_signal),
    TYPED_REG_FOR_ALL_TYPES(alltoall, color_pairwise_exchange_barrier),
    TYPED_REG_FOR_ALL_TYPES(alltoall, color_pairwise_exchange_counter),
    TYPED_REG_FOR_ALL_TYPES(alltoall, color_pairwise_exchange_signal),
    TYPED_LAST};

/**
 * @brief Table of alltoalls collective algorithms for all types
 */
static typed_op_t alltoalls_tab[] = {
    TYPED_REG_FOR_ALL_TYPES(alltoalls, shift_exchange_barrier),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, shift_exchange_counter),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, shift_exchange_signal),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, xor_pairwise_exchange_barrier),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, xor_pairwise_exchange_counter),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, xor_pairwise_exchange_signal),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, color_pairwise_exchange_barrier),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, color_pairwise_exchange_counter),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, color_pairwise_exchange_signal),
    TYPED_LAST};

/**
 * @brief Table of collect collective algorithms
 */
static typed_op_t collect_tab[] = {
    TYPED_REG_FOR_ALL_TYPES(collect, linear),
    TYPED_REG_FOR_ALL_TYPES(collect, all_linear),
    TYPED_REG_FOR_ALL_TYPES(collect, all_linear1),
    TYPED_REG_FOR_ALL_TYPES(collect, rec_dbl),
    TYPED_REG_FOR_ALL_TYPES(collect, rec_dbl_signal),
    TYPED_REG_FOR_ALL_TYPES(collect, ring),
    TYPED_REG_FOR_ALL_TYPES(collect, bruck),
    TYPED_REG_FOR_ALL_TYPES(collect, bruck_no_rotate),
    TYPED_LAST};

/**
 * @brief Table of fcollect collective algorithms
 */
static sized_op_t fcollect_tab[] = {SIZED_REG(fcollect, linear),
                                    SIZED_REG(fcollect, all_linear),
                                    SIZED_REG(fcollect, all_linear1),
                                    SIZED_REG(fcollect, rec_dbl),
                                    SIZED_REG(fcollect, ring),
                                    SIZED_REG(fcollect, bruck),
                                    SIZED_REG(fcollect, bruck_no_rotate),
                                    SIZED_REG(fcollect, bruck_signal),
                                    SIZED_REG(fcollect, bruck_inplace),
                                    SIZED_REG(fcollect, neighbor_exchange),
                                    SIZED_LAST};

/**
 * @brief Table of barrier_all collective algorithms
 */
static unsized_op_t barrier_all_tab[] = {
    UNSIZED_REG(barrier_all, linear),
    UNSIZED_REG(barrier_all, complete_tree),
    UNSIZED_REG(barrier_all, binomial_tree),
    UNSIZED_REG(barrier_all, knomial_tree),
    UNSIZED_REG(barrier_all, dissemination),
    UNSIZED_LAST};

/**
 * @brief Table of sync_all collective algorithms
 */
static unsized_op_t sync_all_tab[] = {
    UNSIZED_REG(sync_all, linear),        UNSIZED_REG(sync_all, complete_tree),
    UNSIZED_REG(sync_all, binomial_tree), UNSIZED_REG(sync_all, knomial_tree),
    UNSIZED_REG(sync_all, dissemination), UNSIZED_LAST};

/**
 * @brief Table of barrier collective algorithms
 */
static unsized_op_t barrier_tab[] = {
    UNSIZED_REG(barrier, linear),        UNSIZED_REG(barrier, complete_tree),
    UNSIZED_REG(barrier, binomial_tree), UNSIZED_REG(barrier, knomial_tree),
    UNSIZED_REG(barrier, dissemination), UNSIZED_LAST};

/**
 * @brief Table of sync collective algorithms
 */
static unsized_op_t sync_tab[] = {
    UNSIZED_REG(sync, linear),        UNSIZED_REG(sync, complete_tree),
    UNSIZED_REG(sync, binomial_tree), UNSIZED_REG(sync, knomial_tree),
    UNSIZED_REG(sync, dissemination), UNSIZED_LAST};

/**
 * @brief Table of broadcast collective algorithms
 */
static sized_op_t broadcast_tab[] = {SIZED_REG(broadcast, linear),
                                     SIZED_REG(broadcast, complete_tree),
                                     SIZED_REG(broadcast, binomial_tree),
                                     SIZED_REG(broadcast, knomial_tree),
                                     SIZED_REG(broadcast, knomial_tree_signal),
                                     SIZED_REG(broadcast, scatter_collect),
                                     SIZED_LAST};

/******************************************************** */
/**
 * @brief Register a sized collective operation
 * @param tabp Pointer to the operation table
 * @param op Operation name to register
 * @param fn32 Pointer to store 32-bit function pointer
 * @param fn64 Pointer to store 64-bit function pointer
 * @return 0 on success, -1 if operation not found
 */
static int register_sized(sized_op_t *tabp, const char *op, coll_fn_t *fn32,
                          coll_fn_t *fn64) {
  sized_op_t *p;

  for (p = tabp; p->f32 != NULL; ++p) {
    if (strncmp(op, p->op, COLL_NAME_MAX) == 0) {
      *fn32 = p->f32;
      *fn64 = p->f64;
      return 0;
      /* NOT REACHED */
    }
  }
  return -1;
}

/**
 * @brief Register an unsized collective operation
 * @param tabp Pointer to the operation table
 * @param op Operation name to register
 * @param fn Pointer to store function pointer
 * @return 0 on success, -1 if operation not found
 */
static int register_unsized(unsized_op_t *tabp, const char *op, coll_fn_t *fn) {
  unsized_op_t *p;

  for (p = tabp; p->f != NULL; ++p) {
    if (strncmp(op, p->op, COLL_NAME_MAX) == 0) {
      *fn = p->f;
      return 0;
      /* NOT REACHED */
    }
  }
  return -1;
}

/**
 * @brief Register a typed collective operation
 * @param tabp Pointer to the operation table
 * @param op Operation name to register
 * @param fn Pointer to store function pointer
 * @return 0 on success, -1 if operation not found
 */
static int register_typed(typed_op_t *tabp, const char *op,
                          typed_coll_fn_t *fn) {
  typed_op_t *p;

  for (p = tabp; p->f != NULL; ++p) {
    if (strncmp(op, p->op, COLL_NAME_MAX) == 0) {
      *fn = p->f;
      return 0;
      /* NOT REACHED */
    }
  }
  return -1;
}

/******************************************************** */
/**
 * @brief Global structure holding all collective operation function pointers
 */
coll_ops_t colls;

/**
 * @brief Macro to generate registration function for sized collectives
 * @param _coll Collective operation name
 */
#define REGISTER_SIZED(_coll)                                                  \
  int register_##_coll(const char *op) {                                       \
    return register_sized(_coll##_tab, op, &colls._coll.f32,                   \
                          &colls._coll.f64);                                   \
  }

/**
 * @brief Macro to generate registration function for unsized collectives
 * @param _coll Collective operation name
 */
#define REGISTER_UNSIZED(_coll)                                                \
  int register_##_coll(const char *op) {                                       \
    return register_unsized(_coll##_tab, op, &colls._coll.f);                  \
  }

/**
 * @brief Macro to generate registration function for typed collectives
 * @param _coll Collective operation name
 */
#define REGISTER_TYPED(_coll)                                                  \
  int register_##_coll(const char *op) {                                       \
    return register_typed(_coll##_tab, op, &colls._coll.f);                    \
  }

REGISTER_TYPED(alltoall)
REGISTER_TYPED(alltoalls)
REGISTER_TYPED(collect)

REGISTER_SIZED(fcollect)
REGISTER_SIZED(broadcast)

REGISTER_UNSIZED(barrier)
REGISTER_UNSIZED(barrier_all)
REGISTER_UNSIZED(sync)
REGISTER_UNSIZED(sync_all)

/*
 * TODO: reductions
 */
