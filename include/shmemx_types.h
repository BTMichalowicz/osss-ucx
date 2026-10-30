#ifndef _SHMEM_TYPES_H_
#define _SHMEM_TYPES_H_ 1


#include <shmem.h>

typedef intptr_t shmem_internal_int;

typedef int shmemx_datatype;

#define shmemx_float            ((shmemx_datatype) 0x1100040a)
#define shmemx_double           ((shmemx_datatype) 0x1100080b)
#define shmemx_long_double      ((shmemx_datatype) 0x1100100c)
#define shmemx_char             ((shmemx_datatype) 0x11000101)
#define shmemx_schar            ((shmemx_datatype) 0x11000118)
#define shmemx_uchar            ((shmemx_datatype) 0x11000102)
#define shmemx_int              ((shmemx_datatype) 0x11000405)
#define shmemx_long             ((shmemx_datatype) 0x11000807)
#define shmemx_long_long        ((shmemx_datatype) 0x11000809)
#define shmemx_ulong            ((shmemx_datatype) 0x11000808)
#define shmemx_ulong_long       ((shmemx_datatype) 0x11000819)
#define shmemx_int8             ((shmemx_datatype) 0x11000104)
#define shmemx_int16            ((shmemx_datatype) 0x11000231)
#define shmemx_int32            ((shmemx_datatype) 0x11000432)
#define shmemx_int64            ((shmemx_datatype) 0x11000833)
#define shmemx_uint8            ((shmemx_datatype) 0x11000140)
#define shmemx_uint16           ((shmemx_datatype) 0x11000241)
#define shmemx_uint32           ((shmemx_datatype) 0x11000442)
#define shmemx_uint64           ((shmemx_datatype) 0x11000843)
#define shmemx_size             ((shmemx_datatype) 0x11000850)
#define shmemx_ptrdiff          ((shmemx_datatype) 0x11000860)
#define shmemx_short            ((shmemx_datatype) 0x11000221)

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

#define shmemx_type_get_basic_id(a) ((a) & 0x000000ff)
#define shmemx_type_get_basic_size(a) (((a) & 0x0000ff00) >> 8)

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


typedef struct shmex_typed_entry {
    shmemx_datatype handle;
    shmemx_internal_datatype_t *representation;
} shmemx_typed_entry_t;

#define shmemx_dtype_add_ref(dptr)                          \
    do {                                                    \
        int kind_ = shmemx_handle_get_kind(dptr->handle);   \
        if (kind_ != shmemx_dtype_builtin){                 \
            dptr->ref_count++;                              \
        }                                                   \
    }while (0)

#define shmemx_dtype_remove_ref(dptr, inuse_ptr)                            \
do {                                                                        \
    int kind_ = shmemx_handle_get_kind(dptr->handle);                       \
    if (kind_ != shmemx_dtype_builtin){                                     \
        dptr->ref_count--;                                                  \
        (*inuse_ptr) = dptr->ref_count < 0 ? 0 : dptr->ref_count;           \
        if (dptr->ref_count < 0) {                                          \
            dptr->ref_count = 0;                                            \
        }                                                                   \
    }else{ *inuse_ptr = 1;}                                                 \
}while (0)

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


/* The APIs themselves */

/* put/get/put_nbi/get_nbi 
 * look either at the shared heap to attempt to condense the buffers
 * OR have it perform some level of optimized strided get (Verbs, see Kaushik's 
 * discussion from years ago 
 */

/* Collectives are a little easier to deal with because we can do optimized
 * pack/unpack within the collective call
 * - Latency-hiding schemes
 * - UMR schemes
 * etc.
 */

int shmemx_dtype_init();

int shmemx_dtype_create_contig(int elem_count, shmemx_datatype old_type, shmemx_datatype *new_type);
int shmemx_dtype_create_vector(int elem_count, int block_count, int stride, 
                           shmemx_datatype old_type, shmemx_datatype *new_type);

void shmemx_ddt_put(shmem_ctx_t ctx, void *dst, const void *src, shmemx_datatype dtype, int elem_count);
void shmemx_ddt_get(shmem_ctx_t ctx, const void *dst, void *src, shmemx_datatype dtype, int elem_count);
void shmemx_ddt_put_nbi(shmem_ctx_t ctx, void *dst, const void *src, shmemx_datatype dtype, int elem_count);
void shmemx_ddt_get_nbi(shmem_ctx_t ctx, const void *src, void *dst, shmemx_datatype dtype, int elem_count);
void shmemx_ddt_bcast(shmem_team_t, const void *src, void *dst, shmemx_datatype dtype, int elem_count);
void shmemx_ddt_collect(shmem_team_t, const void *src, void *dst, shmemx_datatype dtype, int elem_count);
void shmemx_ddt_fcollect(shmem_team_t, const void *src, void *dst, shmemx_datatype dtype, int elem_count);
void shmemx_ddt_alltoall(shmem_team_t, const void *src, void *dst, shmemx_datatype dtype, int elem_count);
void shmemx_ddt_alltoalls(shmem_team_t, const void *src, void *dst, shmemx_datatype dtype, int elem_count);


#define ERROR_SHMEM(fmt, args...)                                   \
    do {                                                            \
        /* fflush(stdout);                                          \
         *       fflush(stderr);                               */    \
        fprintf(stderr, "[rank_%d][%s:%d][%s][ERROR] "fmt,          \
                proc.li.rank, __FILE__, __LINE__, __func__,         \
##args);                                                            \
        /*fflush(stderr);                                           \
         *       fflush(stdout);                              */     \
    } while(0);

#endif /* _SHMEM_TYPES_H_ */
