#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemx.h"
#include "shmemu.h"
#include "shmem_mutex.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include "shmemx_sharp.h"


shmemx_coll_sharp_component_t coll_sharp_component;

const struct sharp_coll_config shmemx_sharp_coll_default_config = {
    .ib_dev_list                = "mlx5_0",
    .user_progress_num_polls    = 128,
    .coll_timeout               = 200
};



char *op_to_string [shmemx_op_null+1] = {
    [shmemx_band] = "band",
    [shmemx_bor] = "bor",
    [shmemx_bxor] = "bxor",
    [shmemx_max] = "max",
    [shmemx_min] = "min",
    [shmemx_sum] = "sum",
    [shmemx_prod] = "prod",
    [shmemx_op_null] = "null_op"
};

char *type_to_string[shmemx_type_null+1] = {
    [shmemx_unsigned]   = "uint",
    [shmemx_int]        = "int",
    [shmemx_ulong]      = "ulong",
    [shmemx_long]       = "long",
    [shmemx_float]      = "float",
    [shmemx_double]     = "double",
    [shmemx_ushort]     = "ushort",
    [shmemx_short]      = "short",
    [shmemx_uint8]      = "uint8",
    [shmemx_uint16]     = "uint16",
    [shmemx_uint32]     = "uint32",
    [shmemx_uint64]     = "uint64",
    [shmemx_int8]       = "int8",
    [shmemx_int16]      = "int16",
    [shmemx_int32]      = "int32",
    [shmemx_int64]      = "int64", 
    [shmemx_type_null]  = "null_type"
};



struct sharp_dtypes_t {
    char                *name;
    enum sharp_datatype dtype;
    shmemx_datatype     shmem_dtype;
    int size;
};

struct sharp_otypes_t {
    char                *name;
    enum sharp_reduce_op sharp_op_type;
    shmemx_reduce_ops     shmem_otype;
};

struct sharp_dtypes_t supported_dtypes [] = 
{
    {"UINT_32_BIT", SHARP_DTYPE_UNSIGNED, shmemx_unsigned, 4},
    {"INT_32_BIT", SHARP_DTYPE_INT, shmemx_int, 4},
    {"UINT_64_BIT", SHARP_DTYPE_UNSIGNED_LONG, shmemx_ulong, 8},
    {"INT_64_BIT", SHARP_DTYPE_LONG, shmemx_long, 8},
    {"FLOAT_32_BIT", SHARP_DTYPE_FLOAT, shmemx_float, 4},
    {"FLOAT_64_BIT", SHARP_DTYPE_DOUBLE, shmemx_double, 8},
    {"UINT_8_BIT", SHARP_DTYPE_UINT8, shmemx_uint8, 1},
    {"INT_8_BIT", SHARP_DTYPE_INT8, shmemx_int8, 1},
    {"INT_16_BIT", SHARP_DTYPE_SHORT, shmemx_int16, 2},
    {"UINT_16_BIT", SHARP_DTYPE_UNSIGNED_SHORT, shmemx_uint16, 2},
    {"NULL", SHARP_DTYPE_NULL, shmemx_type_null, shmemx_op_null, 0},
};
struct sharp_otypes_t supported_otypes [] =
{
    {"MAX", SHARP_OP_MAX, shmemx_max},
    {"MIN", SHARP_OP_MIN, shmemx_min},
    {"SUM", SHARP_OP_SUM, shmemx_sum},
    {"BOR", SHARP_OP_BOR, shmemx_bor},
    {"BAND", SHARP_OP_BAND, shmemx_band},
    {"BXOR", SHARP_OP_BXOR, shmemx_bxor},
    {"PROD", SHARP_OP_PROD, shmemx_prod},
    {"NOOP", SHARP_OP_NULL, shmemx_op_null},
};


static int shmemx_oob_bcast(void *team_ctx, void *src_dst, int size, int root){
    shmem_team_t internal_team = (shmem_team_t) team_ctx;

    int result = shmem_broadcastmem(internal_team, src_dst, (const void*) src_dst, size, root);
    shmemu_assert(result == 0, "oob_bcast failed!\n");
    return result;
}

static int shmemx_oob_barrier(void *team_ctx){
    shmem_barrier_all();
    return 0;
}

static int shmemx_oob_gather(void *team_ctx, int root, void *src, void *dst, int len){
    shmem_team_t internal_team = (shmem_team_t) team_ctx;
    int result = shmem_fcollectmem(internal_team, dst, src, len);

    shmemu_assert(result == 0, "oob_gather failed!\n");
    return result;
}

int shmemx_sharp_coll_init(shmemx_sharp_conf_t *sharp_conf, int pe, int local_pe, shmem_team_t team){
    shmemc_team_h c_team = (shmemc_team_h) team;

    int result = SHARP_COLL_SUCCESS;
    struct sharp_coll_init_spec *init_spec =
        malloc(sizeof(struct sharp_coll_init_spec));
    shmemu_assert(init_spec != NULL, "shmemx_sharp_coll_init: failed to allocate the init spec");

    init_spec->progress_func = NULL;
    init_spec->job_id = atol(sharp_conf->jobid);
    init_spec->world_local_rank = local_pe;
    init_spec->enable_thread_support = 0;
    init_spec->oob_colls.barrier = shmemx_oob_barrier;
    init_spec->oob_colls.bcast = shmemx_oob_bcast;
    init_spec->oob_colls.gather = shmemx_oob_gather;
    init_spec->config = shmemx_sharp_coll_default_config;
    init_spec->group_channel_idx = pe;
    init_spec->config.ib_dev_list = sharp_conf->ibdev_list;

    result = sharp_coll_init(init_spec, &(coll_sharp_component.sharp_coll_ctx));

    if (result != SHARP_COLL_SUCCESS){
        if (proc.li.rank == 0){
            ERROR_SHMEM("SHARP_Initilization failed. Continuing without sharp support: %d %s\n",
                    result, sharp_coll_strerror(result));
        }
        shmemu_assert(result == SHARP_COLL_SUCCESS, "coll_init failed\n");
    }

    result = sharp_coll_caps_query(coll_sharp_component.sharp_coll_ctx, 
            &coll_sharp_component.sharp_caps);

    if (result != SHARP_COLL_SUCCESS){
        if (proc.li.rank == 0){
            ERROR_SHMEM("Query network caps failed: %d %s\n",
                    result, sharp_coll_strerror(result));
        }
        shmemu_assert(result == SHARP_COLL_SUCCESS, "caps_query failed\n");
    }

    free(init_spec);
    init_spec = NULL;
    return result;
}

int shmemx_sharp_comm_init(shmemx_coll_sharp_module_t *sharp_module){

    int result = SHARP_COLL_SUCCESS;
    int me = 0, size = 0;
    shmem_team_t internal_team;
    shmemc_team_h team_handler;
    internal_team = sharp_module->comm;
    me = shmem_team_my_pe(internal_team);
    size = shmem_team_n_pes(internal_team);
    team_handler = (shmemc_team_h) internal_team;
    
    struct sharp_coll_comm_init_spec *comm_spec = 
        malloc(sizeof(struct sharp_coll_comm_init_spec));

    shmemu_assert(comm_spec != NULL, "sharp_comm_init: could not malloc structure\n");

    comm_spec->rank = me;
    comm_spec->size = size;
    comm_spec->oob_ctx = sharp_module;
    sharp_module->is_leader = 1;

    result = sharp_coll_comm_init(coll_sharp_component.sharp_coll_ctx,
            comm_spec, &(sharp_module->sharp_coll_comm));

    if (result != SHARP_COLL_SUCCESS){
        if (proc.li.rank == 0){
            ERROR_SHMEM("comm_init failed: %d %s\n",
                    result, sharp_coll_strerror(result));
        }
        shmemu_assert(result == SHARP_COLL_SUCCESS, "comm_init failed\n");
    }

    free (comm_spec);
    return result;
}

int shmemx_setup_sharp_env(shmemx_sharp_conf_t *sharp_conf, shmem_team_t team){

    shmemc_team_h handler = (shmemc_team_h) team;
    char *dev_list = NULL; 

    /* TODO: Fix this for hardcoding beyond UCX */
    char *devlist = malloc(strlen("mlx5_0") + 3);
    snprintf(devlist, strlen(devlist) + 3, "%s:%d",
            "mlx5_0:", 0);

    sharp_conf->ibdev_list = devlist;

    char *id_str = malloc(JOBID_LEN);
    memset(id_str, 0, JOBID_LEN);
    char hostname[JOBID_LEN] = {'\0'};
    if (gethostname(hostname, JOBID_LEN) != 0){
        ERROR_SHMEM("Cannot find hostname!\n");
        shmem_global_exit(errno);
    }

    pid_t pid = getpid();
    char *jobID = getenv("SLURM_JOBID");
    shmemu_assert(jobID != NULL, "setup_sharp_env: not in a SLURM job!\n");
    sprintf(id_str, "%d_%s_%d_0", atoi(jobID), hostname, pid);

    sharp_conf->jobid = id_str;
    sharp_conf->rank = shmem_team_my_pe(team);
    sharp_conf->size = shmem_team_n_pes(team);
    return 0;
}
    


char *shmemx_sharp_create_hostlist (shmem_team_t team){
    shmemc_team_h team_handler = (shmemc_team_h) team;
 
    int size = 0, i = 0, rank = 0;
    int *len = shmem_malloc(sizeof(int));
    char *name = (char *)shmem_malloc(SHMEMX_PROC_LEN);
    size = shmem_team_n_pes(team);
    rank = shmem_team_my_pe(team);
    int offsets[size];
//    int name_len[size];
    int *name_len = shmem_malloc(size*sizeof(int));

    int sharp_res = 0;
    sharp_res = gethostname(name, SHMEMX_PROC_LEN);

    if (sharp_res != 0){
        ERROR_SHMEM("Could not gethostname\n");
        shmem_global_exit(errno);
    }
    *(len) = strlen(name);
    if (rank < size-1){
        name[*(len)++] = ',';
    }
    sharp_res = shmem_int_fcollect(team, name_len, len, 1);
    if (sharp_res != 0){
        ERROR_SHMEM("internal shmem_fcollect failed\n");
        shmem_global_exit(errno);
    }
    int bytes = 0;
    for (i = 0; i < size; i++){
        offsets[i] = bytes;
        bytes+=name_len[i];
    }
    bytes++;
    char *recv_buf = shmem_malloc(bytes);
    char *out_buf = malloc(bytes);
    sharp_res = shmem_char_collect(team, name, recv_buf+offsets[rank], name_len[rank]);
    if (sharp_res != 0){
        ERROR_SHMEM("Internal shmem_collect failed!\n");
        shmem_global_exit(errno);
    }

    memcpy(out_buf, recv_buf, bytes);
    shmem_free(len);
    shmem_free(name_len);
    shmem_free(recv_buf);
    return out_buf;
}


int shmemx_sharp_init(shmem_team_t team){
    shmemc_team_h teamh = (shmemc_team_h)(team);
    teamh->sharp_conf = malloc(sizeof(shmemx_sharp_conf_t));
    shmemu_assert(teamh->sharp_conf != NULL, "shmemx_sharp_init: Failed to malloc 1\n");
    teamh->sharp_module = malloc(sizeof(shmemx_coll_sharp_module_t));
    shmemu_assert(teamh->sharp_module != NULL, "shmemx_sharp_init: Failed to malloc 1\n");

    if (shmemx_setup_sharp_env(teamh->sharp_conf, SHMEM_TEAM_WORLD) != 0){
        ERROR_SHMEM("Failed to set up sharp env!\n");
        shmem_global_exit(-1);
    }
    teamh->sharp_conf->hostlist = shmemx_sharp_create_hostlist(SHMEM_TEAM_WORLD);
    if (teamh->sharp_conf->hostlist == NULL){
        ERROR_SHMEM("Failed to set up sharp hostlist!\n");
        shmem_global_exit(-1);
    }
    teamh->sharp_module->comm = (void *)(team);

    if (shmemx_sharp_coll_init(teamh->sharp_conf, proc.li.rank, proc.li.rank, teamh->sharp_module->comm)!= 0){
        ERROR_SHMEM("Failed to initialize collective items!\n");
        shmem_global_exit(-1);
    }

    if (shmemx_sharp_comm_init(teamh->sharp_module)!=0){
        ERROR_SHMEM("Failed to finish comm_init!\n");
        shmem_global_exit(-1);
    }
    return 0;
}

shmemx_reduce_ops find_op_type(const char *op){
    int len = strlen(op);

    int i = 0;
    for (i = 0 ; i< shmemx_op_null; i++){
        if (strncmp(op_to_string[i], op, len) == 0){
            return i;
        }
    }

    return shmemx_op_null;
}

shmemx_datatype find_datatype(const char *type){
    int len = strlen(type);

    int i = 0;
    for (i = 0; i<shmemx_type_null; i++){
        if (strncmp(type_to_string[i], type, len) == 0){
            return i;
        }
    }

    return shmemx_type_null;
}

enum sharp_reduce_op shmemx_get_sharp_reduce_op(shmemx_reduce_ops op){
    int i = 0;
    for (i = 0; supported_otypes[i].sharp_op_type != SHARP_OP_NULL; i++){
        if (op == supported_otypes[i].shmem_otype){
            return supported_otypes[i].sharp_op_type;
        }
    }
    return SHARP_OP_NULL;
}

void shmemx_register_sharp_buffer(size_t len, void *buffer, void **memhandle){
    sharp_coll_reg_mr(coll_sharp_component.sharp_coll_ctx, buffer, len, memhandle);
}


void shmemx_get_sharp_datatype(shmemx_datatype dtype, shmemx_sharp_reduce_type_size_t **out){
    int i = 0;
    shmemx_sharp_reduce_type_size_t *res = malloc(sizeof(shmemx_sharp_reduce_type_size_t));

    res->sharp_type = SHARP_DTYPE_NULL;
    for (i = 0; supported_dtypes[i].dtype != SHARP_DTYPE_NULL; i++){
        if (dtype == supported_dtypes[i].shmem_dtype){
            res->sharp_type = supported_dtypes[i].dtype;
            res->size = supported_dtypes[i].size;
            *out = res;
            break;
        }
    }
}

