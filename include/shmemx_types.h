#ifndef _SHMEM_TYPES_H_
#define _SHMEM_TYPES_H_


#include <shmem.h>

typedef intptr_t shmem_internal_int;

typedef int shmemx_datatype;

#define shmemx_float            ((shmemx_datatype) 0x10001)
#define shmemx_double           ((shmemx_datatype) 0x10018)
#define shmemx_long_double      ((shmemx_datatype) 0x10012)
#define shmemx_char             ((shmemx_datatype) 0x10013)
#define shmemx_schar            ((shmemx_datatype) 0x10014)
#define shmemx_uchar            ((shmemx_datatype) 0x10015)
#define shmemx_int              ((shmemx_datatype) 0x10020)
#define shmemx_long             ((shmemx_datatype) 0x10022)
#define shmemx_long_long        ((shmemx_datatype) 0x10023)
#define shmemx_ulong            ((shmemx_datatype) 0x10024)
#define shmemx_ulong_long       ((shmemx_datatype) 0x10025)
#define shmemx_int8             ((shmemx_datatype) 0x10030)
#define shmemx_int16            ((shmemx_datatype) 0x10031)
#define shmemx_int32            ((shmemx_datatype) 0x10032)
#define shmemx_int64            ((shmemx_datatype) 0x10033)
#define shmemx_uint8            ((shmemx_datatype) 0x10040)
#define shmemx_uint16           ((shmemx_datatype) 0x10041)
#define shmemx_uint32           ((shmemx_datatype) 0x10042)
#define shmemx_uint64           ((shmemx_datatype) 0x10043)
#define shmemx_size             ((shmemx_datatype) 0x10050)
#define shmemx_ptrdiff          ((shmemx_datatype) 0x10060)
#define shmemx_short            ((shmemx_datatype) 0x10021)

typedef struct shmemx_datatype_contents {
    int combiner,
        nr_ints,
        nr_aints,
        nr_types;
} shmemx_datatype_contents;

typedef struct shmemx_internal_datatype {

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


} shmemx_internal_datatype_t;



#endif /* _SHMEM_TYPES_H_ */
