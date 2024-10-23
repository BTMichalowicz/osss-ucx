/* For license: see LICENSE file at top-level */

#include "shcoll.h"
#include "table.h"

#include <stdio.h>
#include <string.h>

/*
 * construct table of known algorithms
 */

#define SIZED_REG(_op, _algo)                                                  \
  { #_algo, shcoll_##_op##32##_##_algo, shcoll_##_op##64##_##_algo }
#define SIZED_LAST                                                             \
  { "", NULL, NULL }

#define UNSIZED_REG(_op, _algo)                                                \
  { #_algo, shcoll_##_op##_##_algo }
#define UNSIZED_LAST                                                           \
  { "", NULL }

//////////////////////////////////////////////////////////////
#define TYPED_REG( _op, _algo, _typename )                                       \
  { #_algo, (typed_coll_fn_t)shcoll_##_typename##_##_op##_##_algo }
#define TYPED_LAST                                                             \
  { "", NULL }

#define UNTYPED_REG(_op, _algo)                                                \
  { #_algo, shcoll_##_op##_##_algo }
#define UNTYPED_LAST                                                           \
  { "", NULL }
//////////////////////////////////////////////////////////////

/*
 * known implementations from SHCOLL
 */

static sized_op_t broadcast_tab[] = {SIZED_REG(broadcast, linear),
                                     SIZED_REG(broadcast, complete_tree),
                                     SIZED_REG(broadcast, binomial_tree),
                                     SIZED_REG(broadcast, knomial_tree),
                                     SIZED_REG(broadcast, knomial_tree_signal),
                                     SIZED_REG(broadcast, scatter_collect),
                                     SIZED_LAST};

//////////////////////////////////////////////////////////////
static typed_op_t alltoall_tab[] = {
    TYPED_REG(alltoall, shift_exchange_barrier, float),
    TYPED_REG(alltoall, shift_exchange_barrier, double),
    TYPED_REG(alltoall, shift_exchange_barrier, longdouble),
    TYPED_REG(alltoall, shift_exchange_barrier, char),
    TYPED_REG(alltoall, shift_exchange_barrier, schar),
    TYPED_REG(alltoall, shift_exchange_barrier, short),
    TYPED_REG(alltoall, shift_exchange_barrier, int),
    TYPED_REG(alltoall, shift_exchange_barrier, long),
    TYPED_REG(alltoall, shift_exchange_barrier, longlong),
    TYPED_REG(alltoall, shift_exchange_barrier, uchar),
    TYPED_REG(alltoall, shift_exchange_barrier, ushort),
    TYPED_REG(alltoall, shift_exchange_barrier, uint),
    TYPED_REG(alltoall, shift_exchange_barrier, ulong),
    TYPED_REG(alltoall, shift_exchange_barrier, ulonglong),
    TYPED_REG(alltoall, shift_exchange_barrier, int8),
    TYPED_REG(alltoall, shift_exchange_barrier, int16),
    TYPED_REG(alltoall, shift_exchange_barrier, int32),
    TYPED_REG(alltoall, shift_exchange_barrier, int64),
    TYPED_REG(alltoall, shift_exchange_barrier, uint8),
    TYPED_REG(alltoall, shift_exchange_barrier, uint16),
    TYPED_REG(alltoall, shift_exchange_barrier, uint32),
    TYPED_REG(alltoall, shift_exchange_barrier, uint64),
    TYPED_REG(alltoall, shift_exchange_barrier, size),
    TYPED_REG(alltoall, shift_exchange_barrier, ptrdiff),

    TYPED_REG(alltoall, shift_exchange_counter, float),
    TYPED_REG(alltoall, shift_exchange_counter, double),
    TYPED_REG(alltoall, shift_exchange_counter, longdouble),
    TYPED_REG(alltoall, shift_exchange_counter, char),
    TYPED_REG(alltoall, shift_exchange_counter, schar),
    TYPED_REG(alltoall, shift_exchange_counter, short),
    TYPED_REG(alltoall, shift_exchange_counter, int),
    TYPED_REG(alltoall, shift_exchange_counter, long),
    TYPED_REG(alltoall, shift_exchange_counter, longlong),
    TYPED_REG(alltoall, shift_exchange_counter, uchar),
    TYPED_REG(alltoall, shift_exchange_counter, ushort),
    TYPED_REG(alltoall, shift_exchange_counter, uint),
    TYPED_REG(alltoall, shift_exchange_counter, ulong),
    TYPED_REG(alltoall, shift_exchange_counter, ulonglong),
    TYPED_REG(alltoall, shift_exchange_counter, int8),
    TYPED_REG(alltoall, shift_exchange_counter, int16),
    TYPED_REG(alltoall, shift_exchange_counter, int32),
    TYPED_REG(alltoall, shift_exchange_counter, int64),
    TYPED_REG(alltoall, shift_exchange_counter, uint8),
    TYPED_REG(alltoall, shift_exchange_counter, uint16),
    TYPED_REG(alltoall, shift_exchange_counter, uint32),
    TYPED_REG(alltoall, shift_exchange_counter, uint64),
    TYPED_REG(alltoall, shift_exchange_counter, size),
    TYPED_REG(alltoall, shift_exchange_counter, ptrdiff),

    TYPED_REG(alltoall, shift_exchange_signal, float),
    TYPED_REG(alltoall, shift_exchange_signal, double),
    TYPED_REG(alltoall, shift_exchange_signal, longdouble),
    TYPED_REG(alltoall, shift_exchange_signal, char),
    TYPED_REG(alltoall, shift_exchange_signal, schar),
    TYPED_REG(alltoall, shift_exchange_signal, short),
    TYPED_REG(alltoall, shift_exchange_signal, int),
    TYPED_REG(alltoall, shift_exchange_signal, long),
    TYPED_REG(alltoall, shift_exchange_signal, longlong),
    TYPED_REG(alltoall, shift_exchange_signal, uchar),
    TYPED_REG(alltoall, shift_exchange_signal, ushort),
    TYPED_REG(alltoall, shift_exchange_signal, uint),
    TYPED_REG(alltoall, shift_exchange_signal, ulong),
    TYPED_REG(alltoall, shift_exchange_signal, ulonglong),
    TYPED_REG(alltoall, shift_exchange_signal, int8),
    TYPED_REG(alltoall, shift_exchange_signal, int16),
    TYPED_REG(alltoall, shift_exchange_signal, int32),
    TYPED_REG(alltoall, shift_exchange_signal, int64),
    TYPED_REG(alltoall, shift_exchange_signal, uint8),
    TYPED_REG(alltoall, shift_exchange_signal, uint16),
    TYPED_REG(alltoall, shift_exchange_signal, uint32),
    TYPED_REG(alltoall, shift_exchange_signal, uint64),
    TYPED_REG(alltoall, shift_exchange_signal, size),
    TYPED_REG(alltoall, shift_exchange_signal, ptrdiff),
    
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, float),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, double),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, longdouble),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, char),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, schar),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, short),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, int),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, long),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, longlong),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, uchar),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, ushort),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, uint),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, ulong),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, ulonglong),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, int8),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, int16),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, int32),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, int64),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, uint8),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, uint16),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, uint32),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, uint64),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, size),
    TYPED_REG(alltoall, xor_pairwise_exchange_barrier, ptrdiff),

    TYPED_REG(alltoall, color_pairwise_exchange_signal, float),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, double),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, longdouble),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, char),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, schar),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, short),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, int),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, long),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, longlong),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, uchar),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, ushort),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, uint),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, ulong),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, ulonglong),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, int8),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, int16),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, int32),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, int64),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, uint8),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, uint16),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, uint32),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, uint64),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, size),
    TYPED_REG(alltoall, color_pairwise_exchange_signal, ptrdiff),

    TYPED_REG(alltoall, color_pairwise_exchange_barrier, float),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, double),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, longdouble),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, char),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, schar),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, short),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, int),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, long),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, longlong),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, uchar),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, ushort),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, uint),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, ulong),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, ulonglong),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, int8),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, int16),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, int32),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, int64),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, uint8),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, uint16),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, uint32),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, uint64),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, size),
    TYPED_REG(alltoall, color_pairwise_exchange_barrier, ptrdiff),

    TYPED_REG(alltoall, color_pairwise_exchange_counter, float),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, double),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, longdouble),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, char),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, schar),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, short),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, int),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, long),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, longlong),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, uchar),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, ushort),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, uint),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, ulong),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, ulonglong),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, int8),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, int16),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, int32),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, int64),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, uint8),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, uint16),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, uint32),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, uint64),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, size),
    TYPED_REG(alltoall, color_pairwise_exchange_counter, ptrdiff),

    TYPED_LAST};

static typed_op_t alltoalls_tab[] = {
    TYPED_REG(alltoalls, shift_exchange_barrier, float),
    TYPED_REG(alltoalls, shift_exchange_barrier, double),
    TYPED_REG(alltoalls, shift_exchange_barrier, longdouble),
    TYPED_REG(alltoalls, shift_exchange_barrier, char),
    TYPED_REG(alltoalls, shift_exchange_barrier, schar),
    TYPED_REG(alltoalls, shift_exchange_barrier, short),
    TYPED_REG(alltoalls, shift_exchange_barrier, int),
    TYPED_REG(alltoalls, shift_exchange_barrier, long),
    TYPED_REG(alltoalls, shift_exchange_barrier, longlong),
    TYPED_REG(alltoalls, shift_exchange_barrier, uchar),
    TYPED_REG(alltoalls, shift_exchange_barrier, ushort),
    TYPED_REG(alltoalls, shift_exchange_barrier, uint),
    TYPED_REG(alltoalls, shift_exchange_barrier, ulong),
    TYPED_REG(alltoalls, shift_exchange_barrier, ulonglong),
    TYPED_REG(alltoalls, shift_exchange_barrier, int8),
    TYPED_REG(alltoalls, shift_exchange_barrier, int16),
    TYPED_REG(alltoalls, shift_exchange_barrier, int32),
    TYPED_REG(alltoalls, shift_exchange_barrier, int64),
    TYPED_REG(alltoalls, shift_exchange_barrier, uint8),
    TYPED_REG(alltoalls, shift_exchange_barrier, uint16),
    TYPED_REG(alltoalls, shift_exchange_barrier, uint32),
    TYPED_REG(alltoalls, shift_exchange_barrier, uint64),
    TYPED_REG(alltoalls, shift_exchange_barrier, size),
    TYPED_REG(alltoalls, shift_exchange_barrier, ptrdiff),

    TYPED_REG(alltoalls, shift_exchange_counter, float),
    TYPED_REG(alltoalls, shift_exchange_counter, double),
    TYPED_REG(alltoalls, shift_exchange_counter, longdouble),
    TYPED_REG(alltoalls, shift_exchange_counter, char),
    TYPED_REG(alltoalls, shift_exchange_counter, schar),
    TYPED_REG(alltoalls, shift_exchange_counter, short),
    TYPED_REG(alltoalls, shift_exchange_counter, int),
    TYPED_REG(alltoalls, shift_exchange_counter, long),
    TYPED_REG(alltoalls, shift_exchange_counter, longlong),
    TYPED_REG(alltoalls, shift_exchange_counter, uchar),
    TYPED_REG(alltoalls, shift_exchange_counter, ushort),
    TYPED_REG(alltoalls, shift_exchange_counter, uint),
    TYPED_REG(alltoalls, shift_exchange_counter, ulong),
    TYPED_REG(alltoalls, shift_exchange_counter, ulonglong),
    TYPED_REG(alltoalls, shift_exchange_counter, int8),
    TYPED_REG(alltoalls, shift_exchange_counter, int16),
    TYPED_REG(alltoalls, shift_exchange_counter, int32),
    TYPED_REG(alltoalls, shift_exchange_counter, int64),
    TYPED_REG(alltoalls, shift_exchange_counter, uint8),
    TYPED_REG(alltoalls, shift_exchange_counter, uint16),
    TYPED_REG(alltoalls, shift_exchange_counter, uint32),
    TYPED_REG(alltoalls, shift_exchange_counter, uint64),
    TYPED_REG(alltoalls, shift_exchange_counter, size),
    TYPED_REG(alltoalls, shift_exchange_counter, ptrdiff),

    TYPED_REG(alltoalls, shift_exchange_signal, float),
    TYPED_REG(alltoalls, shift_exchange_signal, double),
    TYPED_REG(alltoalls, shift_exchange_signal, longdouble),
    TYPED_REG(alltoalls, shift_exchange_signal, char),
    TYPED_REG(alltoalls, shift_exchange_signal, schar),
    TYPED_REG(alltoalls, shift_exchange_signal, short),
    TYPED_REG(alltoalls, shift_exchange_signal, int),
    TYPED_REG(alltoalls, shift_exchange_signal, long),
    TYPED_REG(alltoalls, shift_exchange_signal, longlong),
    TYPED_REG(alltoalls, shift_exchange_signal, uchar),
    TYPED_REG(alltoalls, shift_exchange_signal, ushort),
    TYPED_REG(alltoalls, shift_exchange_signal, uint),
    TYPED_REG(alltoalls, shift_exchange_signal, ulong),
    TYPED_REG(alltoalls, shift_exchange_signal, ulonglong),
    TYPED_REG(alltoalls, shift_exchange_signal, int8),
    TYPED_REG(alltoalls, shift_exchange_signal, int16),
    TYPED_REG(alltoalls, shift_exchange_signal, int32),
    TYPED_REG(alltoalls, shift_exchange_signal, int64),
    TYPED_REG(alltoalls, shift_exchange_signal, uint8),
    TYPED_REG(alltoalls, shift_exchange_signal, uint16),
    TYPED_REG(alltoalls, shift_exchange_signal, uint32),
    TYPED_REG(alltoalls, shift_exchange_signal, uint64),
    TYPED_REG(alltoalls, shift_exchange_signal, size),
    TYPED_REG(alltoalls, shift_exchange_signal, ptrdiff),
    
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, float),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, double),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, longdouble),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, char),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, schar),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, short),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, int),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, long),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, longlong),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, uchar),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, ushort),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, uint),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, ulong),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, ulonglong),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, int8),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, int16),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, int32),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, int64),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, uint8),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, uint16),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, uint32),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, uint64),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, size),
    TYPED_REG(alltoalls, xor_pairwise_exchange_barrier, ptrdiff),

    TYPED_REG(alltoalls, color_pairwise_exchange_signal, float),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, double),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, longdouble),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, char),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, schar),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, short),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, int),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, long),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, longlong),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, uchar),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, ushort),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, uint),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, ulong),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, ulonglong),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, int8),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, int16),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, int32),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, int64),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, uint8),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, uint16),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, uint32),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, uint64),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, size),
    TYPED_REG(alltoalls, color_pairwise_exchange_signal, ptrdiff),

    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, float),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, double),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, longdouble),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, char),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, schar),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, short),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, int),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, long),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, longlong),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, uchar),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, ushort),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, uint),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, ulong),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, ulonglong),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, int8),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, int16),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, int32),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, int64),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, uint8),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, uint16),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, uint32),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, uint64),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, size),
    TYPED_REG(alltoalls, color_pairwise_exchange_barrier, ptrdiff),

    TYPED_REG(alltoalls, color_pairwise_exchange_counter, float),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, double),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, longdouble),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, char),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, schar),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, short),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, int),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, long),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, longlong),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, uchar),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, ushort),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, uint),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, ulong),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, ulonglong),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, int8),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, int16),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, int32),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, int64),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, uint8),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, uint16),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, uint32),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, uint64),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, size),
    TYPED_REG(alltoalls, color_pairwise_exchange_counter, ptrdiff),

    TYPED_LAST};

//////////////////////////////////////////////////////////////
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

/*
 * find the function(s) corresponding to the requested name.
 *
 * return 0 if found, non-zero otherwise
 *
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

static int register_typed(typed_op_t *tabp, const char *op, typed_coll_fn_t *fn) {
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

/*
 * given name, set up functions
 */

#define REGISTER_SIZED(_coll)                                                  \
  int register_##_coll(const char *op) {                                       \
    return register_sized(_coll##_tab, op, &colls._coll.f32,                   \
                          &colls._coll.f64);                                   \
  }

#define REGISTER_UNSIZED(_coll)                                                \
  int register_##_coll(const char *op) {                                       \
    return register_unsized(_coll##_tab, op, &colls._coll.f);                  \
  }

#define REGISTER_TYPED(_coll) \
  int register_##_coll(const char *op) { \
    return register_typed(_coll##_tab, op, &colls._coll.f); \
  }

REGISTER_TYPED(alltoall)
// REGISTER_SIZED(alltoalls)
REGISTER_TYPED(alltoalls)
REGISTER_SIZED(broadcast)
REGISTER_SIZED(collect)
REGISTER_SIZED(fcollect)

REGISTER_UNSIZED(barrier)
REGISTER_UNSIZED(barrier_all)
REGISTER_UNSIZED(sync)
REGISTER_UNSIZED(sync_all)

/*
 * TODO reductions
 */
