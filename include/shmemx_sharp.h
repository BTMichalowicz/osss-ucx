#ifndef _SHMEMX_SHARP_H_
#define _SHMEMX_SHARP_H_
//#include "shmem.h"

/**
 * @defgroup SHARP SUPPORT 
 * @brief Integrating SHARP support into the OpenSHMEM Extensions
 * @{
 */

#if ENABLE_SHMEM_SHARP

#include <api/sharp.h>
#include <api/sharp_coll.h>

typedef enum shmemx_datatype {
    shmemx_unsigned,
    shmemx_int,
    shmemx_ulong,
    shmemx_long,
    shmemx_float,
    shmemx_double,
    shmemx_ushort,
    shmemx_short,
    shmemx_uint8,
    shmemx_uint16,
    shmemx_uint32,
    shmemx_uint64,
    shmemx_int8,
    shmemx_int16,
    shmemx_int32,
    shmemx_int64, 
    shmemx_type_null,
} shmemx_datatype;

typedef enum shmemx_reduce_ops{ 
    shmemx_band,
    shmemx_bor,
    shmemx_bxor,
    shmemx_max,
    shmemx_min,
    shmemx_sum,
    shmemx_prod,
    shmemx_op_null
} shmemx_reduce_ops;



typedef struct shmemx_coll_sharp_module {
    struct sharp_coll_comm *sharp_coll_comm;
    int is_leader;
    int ppn;
    shmem_team_t comm;
} shmemx_coll_sharp_module_t;

typedef struct shmemx_sharp_conf {
    int rank;
    int size;
    int port;
    char *hostlist;
    char *jobid;
    char *ibdev_list;
    char *ibdev_name;
} shmemx_sharp_conf_t;

typedef struct shmemx_sharp_reduce_dtype_size {
    shmemx_datatype dtype;
    int size;
} shmemx_sharp_reduce_dtype_size_t;

typedef struct shmemx_coll_sharp_component {
    struct sharp_coll_context *sharp_coll_ctx;
    struct sharp_coll_caps  sharp_caps;
} shmemx_coll_sharp_component_t;

typedef struct shmemx_sharp_info {
    shmemx_coll_sharp_module_t *sharp_comm_module;
    shmemx_sharp_conf_t        *sharp_conf;
} shmemx_sharp_info_t;


typedef struct shmemx_sharp_reduce_type_size {
    enum sharp_datatype sharp_type;
    int size;
} shmemx_sharp_reduce_type_size_t;

int shmemx_sharp_coll_init(shmemx_sharp_conf_t *sharp_conf, int pe, int local_pe, shmem_team_t team);
int shmemx_sharp_comm_init(shmemx_coll_sharp_module_t *sharp_module);
char *shmemx_sharp_create_hostlist (shmem_team_t team);
int shmemx_setup_sharp_env(shmemx_sharp_conf_t *sharp_conf, shmem_team_t team);
int shmemx_sharp_init(shmem_team_t team);

shmemx_reduce_ops find_op_type(const char *op);
shmemx_datatype find_datatype(const char *type);
enum sharp_reduce_op shmemx_get_sharp_reduce_op(shmemx_reduce_ops op);
void shmemx_get_sharp_datatype(shmemx_datatype dtype, shmemx_sharp_reduce_type_size_t **out); 
void shmemx_register_sharp_buffer(size_t len, void *buffer, void **memhandle);

#define EXPAND_AND_STRINGIFY(x) #x
#define STRINGIFY_EXPANDED(x) EXPAND_AND_STRINGIFY(x)


#define SHMEMX_PROC_LEN 256
#define JOBID_LEN 100
#endif /* ENABLE_SHMEM_SHARP */




#endif /* _SHMEMX_SHARP_H_*/
