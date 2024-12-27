/* For license: see LICENSE file at top-level */

/**
 * @file table.h
 * @brief Header file defining collective operation tables and registration
 * functions
 */

#ifndef _TABLE_H
#define _TABLE_H 1

/** Maximum length for collective operation names */
#define COLL_NAME_MAX 64

/******************************************************** */
/** Function pointer type for collective operations without type information */
typedef void (*coll_fn_t)();

/**
 * @brief Structure for sized collective operations that have 32-bit and 64-bit
 * variants
 */
typedef struct sized_op {
  const char op[COLL_NAME_MAX]; /**< Operation name */
  coll_fn_t f32;                /**< 32-bit implementation */
  coll_fn_t f64;                /**< 64-bit implementation */
} sized_op_t;

/**
 * @brief Structure for collective operations without size variants
 */
typedef struct unsized_op {
  const char op[COLL_NAME_MAX]; /**< Operation name */
  coll_fn_t f;                  /**< Implementation function */
} unsized_op_t;

/******************************************************** */
/** Function pointer type for typed collective operations */
typedef int (*typed_coll_fn_t)();

/**
 * @brief Structure for typed collective operations
 */
typedef struct typed_op {
  const char op[COLL_NAME_MAX]; /**< Operation name */
  typed_coll_fn_t f;            /**< Implementation function */
} typed_op_t;

/** Function pointer type for untyped collective operations */
typedef int (*untyped_coll_fn_t)();

/**
 * @brief Structure for untyped collective operations
 */
typedef struct untyped_op {
  const char op[COLL_NAME_MAX]; /**< Operation name */
  untyped_coll_fn_t f;          /**< Implementation function */
} untyped_op_t;

/******************************************************** */
/**
 * @brief Structure containing all collective operation implementations
 */
typedef struct coll_ops {
  typed_op_t alltoall;      /**< All-to-all operation */
  typed_op_t alltoalls;     /**< Strided all-to-all operation */
  typed_op_t collect;       /**< Collect operation */
  typed_op_t fcollect;      /**< Ordered collect operation */
  sized_op_t broadcast;     /**< Broadcast operation */
  unsized_op_t barrier;     /**< Team barrier operation */
  unsized_op_t barrier_all; /**< Global barrier operation */
  // unsized_op_t sync;        /**< Team synchronization operation */
  unsized_op_t sync;        /**< Team synchronization operation */
  unsized_op_t sync_all;    /**< Global synchronization operation */
} coll_ops_t;

/** Global collective operations table */
extern coll_ops_t colls;

/**
 * @brief Registration functions for collective operations
 * @param op Name of the operation to register
 * @return 0 on success, non-zero on failure
 */
int register_barrier_all(const char *op);
int register_sync_all(const char *op);
int register_barrier(const char *op);
int register_sync(const char *op);
int register_broadcast(const char *op);
int register_alltoall(const char *op);
int register_alltoalls(const char *op);
int register_collect(const char *op);
int register_fcollect(const char *op);

#endif
