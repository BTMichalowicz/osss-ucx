/* For license: see LICENSE file at top-level */

#include "shcoll.h"
#include "table.h"

#include <stdio.h>
#include <string.h>

/*
 * construct table of known algorithms
 */

/* 32-bit and 64-bit versions */
#define SIZED_REG(_op, _algo)                                                  \
  { #_algo, shcoll_##_op##32##_##_algo, shcoll_##_op##64##_##_algo }
#define SIZED_LAST                                                             \
  { "", NULL, NULL }

/* Unsized versions */
#define UNSIZED_REG(_op, _algo)                                                \
  { #_algo, shcoll_##_op##_##_algo }
#define UNSIZED_LAST                                                           \
  { "", NULL }

/* Typed versions */
#define TYPED_REG(_op, _algo, _typename)                                       \
  { #_algo, shcoll_##_typename##_##_op##_##_algo }
#define TYPED_LAST                                                             \
  { "", NULL }

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

#define UNTYPED_REG(_op, _algo)                                                \
  { #_algo, shcoll_##_op##_##_algo }
#define UNTYPED_LAST                                                           \
  { "", NULL }


//////////////////////////////////////////////////////////////////////
/* Collective Tables */
//////////////////////////////////////////////////////////////////////
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

static sized_op_t collect_tab[] = {SIZED_REG(collect, linear),
                                   SIZED_REG(collect, all_linear),
                                   SIZED_REG(collect, all_linear1),
                                   SIZED_REG(collect, rec_dbl),
                                   SIZED_REG(collect, rec_dbl_signal),
                                   SIZED_REG(collect, ring),
                                   SIZED_REG(collect, bruck),
                                   SIZED_REG(collect, bruck_no_rotate),
                                   SIZED_LAST};

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

static unsized_op_t barrier_all_tab[] = {
    UNSIZED_REG(barrier_all, linear),
    UNSIZED_REG(barrier_all, complete_tree),
    UNSIZED_REG(barrier_all, binomial_tree),
    UNSIZED_REG(barrier_all, knomial_tree),
    UNSIZED_REG(barrier_all, dissemination),
    UNSIZED_LAST};

static unsized_op_t sync_all_tab[] = {
    UNSIZED_REG(sync_all, linear),        UNSIZED_REG(sync_all, complete_tree),
    UNSIZED_REG(sync_all, binomial_tree), UNSIZED_REG(sync_all, knomial_tree),
    UNSIZED_REG(sync_all, dissemination), UNSIZED_LAST};

static unsized_op_t barrier_tab[] = {
    UNSIZED_REG(barrier, linear),        UNSIZED_REG(barrier, complete_tree),
    UNSIZED_REG(barrier, binomial_tree), UNSIZED_REG(barrier, knomial_tree),
    UNSIZED_REG(barrier, dissemination), UNSIZED_LAST};

static unsized_op_t sync_tab[] = {
    UNSIZED_REG(sync, linear),        UNSIZED_REG(sync, complete_tree),
    UNSIZED_REG(sync, binomial_tree), UNSIZED_REG(sync, knomial_tree),
    UNSIZED_REG(sync, dissemination), UNSIZED_LAST};

static sized_op_t broadcast_tab[] = {SIZED_REG(broadcast, linear),
                                     SIZED_REG(broadcast, complete_tree),
                                     SIZED_REG(broadcast, binomial_tree),
                                     SIZED_REG(broadcast, knomial_tree),
                                     SIZED_REG(broadcast, knomial_tree_signal),
                                     SIZED_REG(broadcast, scatter_collect),
                                     SIZED_LAST};
//////////////////////////////////////////////////////////////////////









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

/*
 * global registry
 */
coll_ops_t colls;

#define REGISTER_SIZED(_coll)                                                  \
  int register_##_coll(const char *op) {                                       \
    return register_sized(_coll##_tab, op, &colls._coll.f32,                   \
                          &colls._coll.f64);                                   \
  }

#define REGISTER_UNSIZED(_coll)                                                \
  int register_##_coll(const char *op) {                                       \
    return register_unsized(_coll##_tab, op, &colls._coll.f);                  \
  }

#define REGISTER_TYPED(_coll)                                                  \
  int register_##_coll(const char *op) {                                       \
    return register_typed(_coll##_tab, op, &colls._coll.f);                    \
  }

// REGISTER_SIZED(alltoall)
// REGISTER_SIZED(alltoalls)

REGISTER_TYPED(alltoall)
REGISTER_TYPED(alltoalls)
REGISTER_SIZED(broadcast)
REGISTER_SIZED(collect)
REGISTER_SIZED(fcollect)

REGISTER_UNSIZED(barrier)
REGISTER_UNSIZED(barrier_all)
REGISTER_UNSIZED(sync)
REGISTER_UNSIZED(sync_all)

/*
 * TODO: reductions
 */
