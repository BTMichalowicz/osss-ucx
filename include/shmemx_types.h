#ifndef _SHMEM_TYPES_H_
#define _SHMEM_TYPES_H_


#include <shmem.h>

typedef intptr_t shmem_internal_int;

typedef int shmemx_datatype;

#define shmemx_float            ((shmemx_datatype) 0x11000101)
#define shmemx_double           ((shmemx_datatype) 0x11000118)
#define shmemx_long_double      ((shmemx_datatype) 0x11000112)
#define shmemx_char             ((shmemx_datatype) 0x11000113)
#define shmemx_schar            ((shmemx_datatype) 0x11000114)
#define shmemx_uchar            ((shmemx_datatype) 0x11000115)
#define shmemx_int              ((shmemx_datatype) 0x11000120)
#define shmemx_long             ((shmemx_datatype) 0x11000122)
#define shmemx_long_long        ((shmemx_datatype) 0x11000123)
#define shmemx_ulong            ((shmemx_datatype) 0x11000124)
#define shmemx_ulong_long       ((shmemx_datatype) 0x11000125)
#define shmemx_int8             ((shmemx_datatype) 0x11000130)
#define shmemx_int16            ((shmemx_datatype) 0x11000131)
#define shmemx_int32            ((shmemx_datatype) 0x11000132)
#define shmemx_int64            ((shmemx_datatype) 0x11000133)
#define shmemx_uint8            ((shmemx_datatype) 0x11000140)
#define shmemx_uint16           ((shmemx_datatype) 0x11000141)
#define shmemx_uint32           ((shmemx_datatype) 0x11000142)
#define shmemx_uint64           ((shmemx_datatype) 0x11000143)
#define shmemx_size             ((shmemx_datatype) 0x11000150)
#define shmemx_ptrdiff          ((shmemx_datatype) 0x11000160)
#define shmemx_short            ((shmemx_datatype) 0x11000121)

#define shmemx_ident 0
#define shmemx_congruent 1
#define shmemx_similar 2
#define shmemx_unequal 3

#define shmemx_dtype_invalid 0x0
#define shmemx_dtype_builtin 0x1
#define shmemx_dtype_direct 0x2
#define shmemx_dtype_indirect 0x3

#define shmemx_handle_kind_mask 0x10000000
#define shmemx_handle_kind_shift 30
#define shmemx_handle_get_kind (((unsigned)(a)&shmemx_handle_kind_mask)>>shmemx_handle_kind_shift)

typedef struct shmemx_datatype_contents {
    int combiner,
        nr_ints,
        nr_aints,
        nr_types;
} shmemx_datatype_contents;

typedef int shmemx_ref_counter;

#define shmemx_obj_header                           \
    int handle;                                     \
    shmemx_ref_counter ref_count /* No semicolon */

#define shmemx_dev_dtype_decl void *dev_dtype;

typedef struct shmemx_internal_datatype {

    shmemx_obj_header;

    shmem_internal_int size,
                       extent, ub, lb, true_ub, true_lb;

    shmem_internal_int alignment;

    int has_sticky_ub, has_sticky_lb;
    int is_permanent, is_committed;

    shmemx_datatype_contents *contents;
    shmem_internal_int *offsets;

    int basic_type;
    shmem_internal_int n_builtin_elems,
                       builtin_elem_size;

    int is_contig;
    shmem_internal_int max_contig_blocks;

    int32_t cache_id; /* Down the line, consider type caches */


    /* Down the line; consider for GPU-SHMEM */
    shmemx_dev_dtype_decl /* To convert accordingly for GPUs or other accelerators */


} shmemx_internal_datatype_t;

shmemx_dtype_create_contig(int elem_count, shmemx_datatype old_type, shmemx_datatype *new_type);
shmemx_dtype_create_vector(int elem_count, int block_count, int stride, 
                           shmemx_datatype old_type, shmemx_datatype *new_type);

shmemx_ddt_put(shmem_ctx_t ctx, void *dst, const void *src, shmemx_datatype dtype, int elem_count);
shmemx_ddt_get(shmem_ctx_t ctx, const void *dst, void *src, shmemx_datatype dtype, int elem_count);
shmemx_ddt_put_nbi(shmem_ctx_t ctx, void *dst, const void *src, shmemx_datatype dtype, int elem_count);
shmemx_ddt_get_nbi(shmem_ctx_t ctx, const void *src, void *dst, shmemx_datatype dtype, int elem_count);

#define shmemx_dtype_contig_lb_ub (cnt_, old_lb_, old_ub_, old_extent_, lb_, ub_)       \
    do {                                                                                \
        if (cnt_ == 0) {                                                                \
            lb_ = old_lb_;                                                              \
            ub_ = old_ub_;                                                              \
        }                                                                               \
        else if (old_ub_ >= old_lb_) {                                                  \
            lb_ = old_lb_;                                                              \
            ub_ = old_ub_ + (old_extent_) * (cnt_ - 1);                                 \
        }                                                                               \
        else /* negative extent */ {                                                    \
            lb_ = old_lb_ + (old_extent_) * (cnt_ - 1);                                 \
            ub_ = old_ub_;                                                              \
        }                                                                               \
    } while(0)


#define shmemx_dtype_vector_lb_ub (cnt_,            \
        stride_,         \
        blklen_,         \
        old_lb_,         \
        old_ub_,         \
        old_extent_,     \
        lb_,         \
        ub_)         \
do {                                \
    if (cnt_ == 0 || blklen_ == 0) {                \
        lb_ = old_lb_;                  \
        ub_ = old_ub_;                  \
    }                               \
    else if (stride_ >= 0 && (old_extent_) >= 0) {      \
        lb_ = old_lb_;                  \
        ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1) +   \
        (stride_) * ((cnt_) - 1);               \
    }                               \
    else if (stride_ < 0 && (old_extent_) >= 0) {       \
        lb_ = old_lb_ + (stride_) * ((cnt_) - 1);       \
        ub_ = old_ub_ + (old_extent_) * ((blklen_) - 1);    \
    }                               \
    else if (stride_ >= 0 && (old_extent_) < 0) {       \
        lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1);    \
        ub_ = old_ub_ + (stride_) * ((cnt_) - 1);       \
    }                               \
    else {                          \
        lb_ = old_lb_ + (old_extent_) * ((blklen_) - 1) +   \
        (stride_) * ((cnt_) - 1);               \
        ub_ = old_ub;                           \
    }                                           \
}while (0)


typedef struct shmex_typed_hashmap {
    shmemx_datatype handle;
    shmemx_internal_*datatype representation;
} shmemx_typed_hashmap;


#endif /* _SHMEM_TYPES_H_ */
