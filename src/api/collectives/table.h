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
  /* Current routines */
  // typed_op_t alltoall_type;      /**< Typed all-to-all operation */
  untyped_op_t alltoall_mem; /**< Generic all-to-all memory operation */
  sized_op_t alltoall_size;  /**< Sized all-to-all operation */

  // typed_op_t alltoalls_type;      /**< Typed strided all-to-all operation */
  untyped_op_t
      alltoalls_mem;         /**< Generic strided all-to-all memory operation */
  sized_op_t alltoalls_size; /**< Sized strided all-to-all operation */

  // typed_op_t collect_type;      /**< Typed collect operation */
  untyped_op_t collect_mem; /**< Generic collect memory operation */
  sized_op_t collect_size;  /**< Sized collect operation */

  // typed_op_t fcollect_type;      /**< Typed ordered collect operation */
  untyped_op_t fcollect_mem; /**< Generic ordered collect memory operation */
  sized_op_t fcollect_size;  /**< Sized ordered collect operation */

  // typed_op_t broadcast_type;      /**< Typed broadcast operation */
  untyped_op_t broadcast_mem; /**< Generic broadcast memory operation */
  sized_op_t broadcast_size;  /**< Sized broadcast operation */

  unsized_op_t barrier_all; /**< Typed global barrier operation */
  unsized_op_t sync;        /**< Synchronization operation */
  unsized_op_t sync_all;    /**< Global synchronization operation */
  unsized_op_t barrier;     /**< Barrier operation */

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

// int register_alltoall_type(const char *op);
int register_alltoall_mem(const char *op);
int register_alltoall_size(const char *op);

// int register_alltoalls_type(const char *op);
int register_alltoalls_mem(const char *op);
int register_alltoalls_size(const char *op);

// int register_collect_type(const char *op);
int register_collect_mem(const char *op);
int register_collect_size(const char *op);

// int register_fcollect_type(const char *op);
int register_fcollect_mem(const char *op);
int register_fcollect_size(const char *op);

// int register_broadcast_type(const char *op);
int register_broadcast_mem(const char *op);
int register_broadcast_size(const char *op);

#endif
