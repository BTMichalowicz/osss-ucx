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
 * - Sized operations
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
  { #_algo, #_typename, shcoll_##_typename##_##_op##_##_algo }

/**
 * @brief Macro to terminate a typed operation table
 */
#define TYPED_LAST                                                             \
  { "", "", NULL }

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

/**
  TODO: Add separate macros for reduction registration
 */

/******************************************************** */
/**
 * @brief Table of alltoall collective algorithms for all types
 */
static typed_op_t alltoall_type_tab[] = {
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
 * @brief Table of generic alltoallmem collective algorithms
 */
static untyped_op_t alltoall_mem_tab[] = {
    UNTYPED_REG(alltoallmem, shift_exchange_barrier),
    UNTYPED_REG(alltoallmem, shift_exchange_counter),
    UNTYPED_REG(alltoallmem, shift_exchange_signal),
    UNTYPED_REG(alltoallmem, xor_pairwise_exchange_barrier),
    UNTYPED_REG(alltoallmem, xor_pairwise_exchange_counter),
    UNTYPED_REG(alltoallmem, xor_pairwise_exchange_signal),
    UNTYPED_REG(alltoallmem, color_pairwise_exchange_barrier),
    UNTYPED_REG(alltoallmem, color_pairwise_exchange_counter),
    UNTYPED_REG(alltoallmem, color_pairwise_exchange_signal),
    UNTYPED_LAST};

/**
 * @brief Table of sized alltoall (deprecated)
 */
static sized_op_t alltoall_size_tab[] = {
    SIZED_REG(alltoall, shift_exchange_barrier),
    SIZED_REG(alltoall, shift_exchange_counter),
    SIZED_REG(alltoall, shift_exchange_signal),
    SIZED_REG(alltoall, xor_pairwise_exchange_barrier),
    SIZED_REG(alltoall, xor_pairwise_exchange_counter),
    SIZED_REG(alltoall, xor_pairwise_exchange_signal),
    SIZED_REG(alltoall, color_pairwise_exchange_barrier),
    SIZED_REG(alltoall, color_pairwise_exchange_counter),
    SIZED_REG(alltoall, color_pairwise_exchange_signal),
    SIZED_LAST};

/**
 * @brief Table of alltoalls collective algorithms for all types
 */
static typed_op_t alltoalls_type_tab[] = {
    TYPED_REG_FOR_ALL_TYPES(alltoalls, shift_exchange_barrier),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, shift_exchange_counter),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, xor_pairwise_exchange_barrier),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, xor_pairwise_exchange_counter),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, color_pairwise_exchange_barrier),
    TYPED_REG_FOR_ALL_TYPES(alltoalls, color_pairwise_exchange_counter),
    TYPED_LAST};

/**
 * @brief Table of generic alltoalls (deprecated)
 */
static untyped_op_t alltoalls_mem_tab[] = {
    UNTYPED_REG(alltoallsmem, shift_exchange_barrier),
    UNTYPED_REG(alltoallsmem, shift_exchange_counter),
    UNTYPED_REG(alltoallsmem, xor_pairwise_exchange_barrier),
    UNTYPED_REG(alltoallsmem, xor_pairwise_exchange_counter),
    UNTYPED_REG(alltoallsmem, color_pairwise_exchange_barrier),
    UNTYPED_REG(alltoallsmem, color_pairwise_exchange_counter),
    UNTYPED_LAST};

/**
 * @brief Table of sized alltoalls (deprecated)
 */
static sized_op_t alltoalls_size_tab[] = {
    SIZED_REG(alltoalls, shift_exchange_barrier),
    SIZED_REG(alltoalls, shift_exchange_counter),
    SIZED_REG(alltoalls, xor_pairwise_exchange_barrier),
    SIZED_REG(alltoalls, xor_pairwise_exchange_counter),
    SIZED_REG(alltoalls, color_pairwise_exchange_barrier),
    SIZED_REG(alltoalls, color_pairwise_exchange_counter),
    SIZED_LAST};

/**
 * @brief Table of collect collective algorithms
 */
static typed_op_t collect_type_tab[] = {
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
 * @brief Table of generic collectmem (deprecated)
 */
static untyped_op_t collect_mem_tab[] = {
    UNTYPED_REG(collectmem, linear),
    UNTYPED_REG(collectmem, all_linear),
    UNTYPED_REG(collectmem, all_linear1),
    UNTYPED_REG(collectmem, rec_dbl),
    UNTYPED_REG(collectmem, rec_dbl_signal),
    UNTYPED_REG(collectmem, ring),
    UNTYPED_REG(collectmem, bruck),
    UNTYPED_REG(collectmem, bruck_no_rotate),
    UNTYPED_LAST};

/**
 * @brief Table of sized collect (deprecated)
 */
static sized_op_t collect_size_tab[] = {SIZED_REG(collect, linear),
                                        SIZED_REG(collect, all_linear),
                                        SIZED_REG(collect, all_linear1),
                                        SIZED_REG(collect, rec_dbl),
                                        SIZED_REG(collect, rec_dbl_signal),
                                        SIZED_REG(collect, ring),
                                        SIZED_REG(collect, bruck),
                                        SIZED_REG(collect, bruck_no_rotate),
                                        SIZED_LAST};

/**
 * @brief Table of fcollect collective algorithms
 */
static typed_op_t fcollect_type_tab[] = {
    TYPED_REG_FOR_ALL_TYPES(fcollect, linear),
    TYPED_REG_FOR_ALL_TYPES(fcollect, all_linear),
    TYPED_REG_FOR_ALL_TYPES(fcollect, all_linear1),
    TYPED_REG_FOR_ALL_TYPES(fcollect, rec_dbl),
    TYPED_REG_FOR_ALL_TYPES(fcollect, ring),
    TYPED_REG_FOR_ALL_TYPES(fcollect, bruck),
    TYPED_REG_FOR_ALL_TYPES(fcollect, bruck_no_rotate),
    TYPED_REG_FOR_ALL_TYPES(fcollect, bruck_signal),
    TYPED_REG_FOR_ALL_TYPES(fcollect, bruck_inplace),
    TYPED_REG_FOR_ALL_TYPES(fcollect, neighbor_exchange),
    TYPED_LAST};

/**
 * @brief Table of generic fcollectmem (deprecated)
 */
static untyped_op_t fcollect_mem_tab[] = {
    UNTYPED_REG(fcollectmem, linear),
    UNTYPED_REG(fcollectmem, all_linear),
    UNTYPED_REG(fcollectmem, all_linear1),
    UNTYPED_REG(fcollectmem, rec_dbl),
    UNTYPED_REG(fcollectmem, ring),
    UNTYPED_REG(fcollectmem, bruck),
    UNTYPED_REG(fcollectmem, bruck_no_rotate),
    UNTYPED_REG(fcollectmem, bruck_signal),
    UNTYPED_REG(fcollectmem, bruck_inplace),
    UNTYPED_REG(fcollectmem, neighbor_exchange),
    UNTYPED_LAST};

/**
 * @brief Table of sized fcollect (deprecated)
 */
static sized_op_t fcollect_size_tab[] = {SIZED_REG(fcollect, linear),
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
 * @brief Table of broadcast collective algorithms
 */
static typed_op_t broadcast_type_tab[] = {
    TYPED_REG_FOR_ALL_TYPES(broadcast, linear),
    TYPED_REG_FOR_ALL_TYPES(broadcast, complete_tree),
    TYPED_REG_FOR_ALL_TYPES(broadcast, binomial_tree),
    TYPED_REG_FOR_ALL_TYPES(broadcast, knomial_tree),
    TYPED_REG_FOR_ALL_TYPES(broadcast, knomial_tree_signal),
    TYPED_REG_FOR_ALL_TYPES(broadcast, scatter_collect),
    TYPED_LAST};

/**
 * @brief Table of generic broadcastmem (deprecated)
 */
static untyped_op_t broadcast_mem_tab[] = {
    UNTYPED_REG(broadcastmem, linear),
    UNTYPED_REG(broadcastmem, complete_tree),
    UNTYPED_REG(broadcastmem, binomial_tree),
    UNTYPED_REG(broadcastmem, knomial_tree),
    UNTYPED_REG(broadcastmem, knomial_tree_signal),
    UNTYPED_REG(broadcastmem, scatter_collect),
    UNTYPED_LAST};

/**
 * @brief Table of sized broadcast (deprecated)
 */
static sized_op_t broadcast_size_tab[] = {
    SIZED_REG(broadcast, linear),
    SIZED_REG(broadcast, complete_tree),
    SIZED_REG(broadcast, binomial_tree),
    SIZED_REG(broadcast, knomial_tree),
    SIZED_REG(broadcast, knomial_tree_signal),
    SIZED_REG(broadcast, scatter_collect),
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
 * @brief Table of team_sync collective algorithms
 */
static untyped_op_t team_sync_tab[] = {UNTYPED_REG(team_sync, linear),
                                       UNTYPED_REG(team_sync, complete_tree),
                                       UNTYPED_REG(team_sync, binomial_tree),
                                       UNTYPED_REG(team_sync, knomial_tree),
                                       UNTYPED_REG(team_sync, dissemination),
                                       UNTYPED_LAST};

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
  char base_op[COLL_NAME_MAX];

  /* Strip _size suffix if present */
  size_t len = strlen(op);
  if (len > 5 && strcmp(op + len - 5, "_size") == 0) {
    strncpy(base_op, op, len - 5);
    base_op[len - 5] = '\0';
  } else {
    strncpy(base_op, op, COLL_NAME_MAX - 1);
    base_op[COLL_NAME_MAX - 1] = '\0';
  }

  for (p = tabp; p->f32 != NULL; ++p) {
    if (strncmp(base_op, p->op, COLL_NAME_MAX) ==
        0) { // Compare with stripped name
      *fn32 = p->f32;
      *fn64 = p->f64;
      return 0;
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
 * @param op Algorithm or "algorithm:type" string
 * @param fn Pointer to store function pointer
 * @return 0 on success, -1 if operation not found
 */
static int register_typed(typed_op_t *tabp, const char *op,
                          typed_coll_fn_t *fn) {
  typed_op_t *p;
  char base_op[COLL_NAME_MAX] = {0};
  char req_type[COLL_NAME_MAX] = {0};

  /* Parse "algorithm:type" or just "algorithm" */
  const char *colon = strchr(op, ':');
  if (colon) {
    size_t len = colon - op;
    strncpy(base_op, op, len);
    base_op[len] = '\0';
    strncpy(req_type, colon + 1, COLL_NAME_MAX - 1);
  } else {
    strncpy(base_op, op, COLL_NAME_MAX - 1);
    base_op[COLL_NAME_MAX - 1] = '\0';
  }

  /* Look up matching entry: algorithm and (optional) type */
  for (p = tabp; p->f != NULL; ++p) {
    if (strncmp(base_op, p->op, COLL_NAME_MAX) != 0)
      continue;
    if (req_type[0] && strncmp(req_type, p->type, COLL_NAME_MAX) != 0)
      continue;
    *fn = p->f;
    return 0;
  }
  return -1;
}

/**
 * @brief Register an untyped collective operation
 * @param tabp Pointer to the operation table
 * @param op Operation name to register
 * @param fn Pointer to store function pointer
 * @return 0 on success, -1 if operation not found
 */
static int register_untyped(untyped_op_t *tabp, const char *op,
                            untyped_coll_fn_t *fn) {
  untyped_op_t *p;
  char op_name[COLL_NAME_MAX];
  const char *mem_suffix = "_mem";

  // Copy op name and handle _mem suffix
  strncpy(op_name, op, COLL_NAME_MAX);
  char *suffix_pos = strstr(op_name, mem_suffix);
  if (suffix_pos != NULL) {
    // Remove the underscore by shifting chars left
    memmove(suffix_pos, suffix_pos + 1, strlen(suffix_pos));
  }

  for (p = tabp; p->f != NULL; ++p) {
    if (strncmp(op_name, p->op, COLL_NAME_MAX) == 0) {
      *fn = p->f;
      return 0;
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

/**
 * @brief Macro to generate registration function for untyped collectives
 * @param _coll Collective operation name
 */
#define REGISTER_UNTYPED(_coll)                                                \
  int register_##_coll(const char *op) {                                       \
    return register_untyped(_coll##_tab, op, &colls._coll.f);                  \
  }

/* Register all collectives */
REGISTER_TYPED(alltoall_type)
REGISTER_UNTYPED(alltoall_mem)
REGISTER_SIZED(alltoall_size)

REGISTER_TYPED(alltoalls_type)
REGISTER_UNTYPED(alltoalls_mem)
REGISTER_SIZED(alltoalls_size)

REGISTER_TYPED(collect_type)
REGISTER_UNTYPED(collect_mem)
REGISTER_SIZED(collect_size)

REGISTER_TYPED(fcollect_type)
REGISTER_UNTYPED(fcollect_mem)
REGISTER_SIZED(fcollect_size)

REGISTER_TYPED(broadcast_type)
REGISTER_UNTYPED(broadcast_mem)
REGISTER_SIZED(broadcast_size)

REGISTER_UNSIZED(barrier_all)
REGISTER_UNSIZED(sync)
REGISTER_UNSIZED(sync_all)
REGISTER_UNSIZED(barrier)
REGISTER_UNTYPED(team_sync)

/*
 * TODO: reductions
 */
