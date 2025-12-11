#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemx.h"
#include "shmemu.h"
#include "shmem_mutex.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include "shmemx_sharp.h"
//#include "pmi_client.h"


shmemx_coll_sharp_component_t coll_sharp_component;

const struct sharp_coll_config shmemx_sharp_coll_default_config = {
    .ib_dev_list                = "mlx5_0:1",
    .user_progress_num_polls    = 128,
    .coll_timeout               = 100
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

#define CODE 2

static int shmemx_oob_bcast(void *team_ctx, void *buf, int size, int root){
    shmem_team_t internal_team = (shmem_team_t) team_ctx;
    int result = 0;

    /*
#if 0
    return result;
#else
    void *tmp_buf = shmem_malloc(size);
    if (tmp_buf == NULL){
        ERROR_SHMEM("Can't shmem_malloc anymore\n");
        shmem_global_exit(-1);
    }

    if (src_dst == NULL){
        ERROR_SHMEM("Internal error, buffer passed in is NULL\n");
        shmem_global_exit(-1);
    }
    int rank = shmem_team_my_pe(internal_team);

    char *tmp = (char *) src_dst;
    if (rank == root){
        DEBUG_SHMEM(" ROOT: src_dst_contents: %s\n", tmp);
    }    

    if (src_dst == NULL){ 
        DEBUG_SHMEM("Buffer is NULL \n"); 
        return -1; 
    }


    result = shmem_broadcastmem(internal_team, tmp_buf, src_dst, size, root); 
    //shmem_fcollectmem(internal_team, tmp_buf, src_dst, size);
    shmem_quiet();
    shmem_barrier_all();
   //int result = shmem_broadcastmem(internal_team, tmp_buf, src_dst, size, root);

  
    if (rank != root){
        memcpy(src_dst, tmp_buf, size);
    }
    shmem_free(tmp_buf);
    DEBUG_SHMEM("bcast result contents: %s\n", (char *)src_dst);
    DEBUG_SHMEM("Cleared bcast with result %d\n", result);
    return result;
#endif
*/

    /* Code from Ferrol Aderholt @NVIDIA as of December 11, 2025 -- MANY THANKS TO FERROL */
        static void *bcast_symmetric_buf = NULL;
    static size_t bcast_buf_size = 0;
    static long *pSync_bcast = NULL;

    /* Allocate symmetric buffer and pSync on first call */
    if (bcast_symmetric_buf == NULL) {
        bcast_buf_size = 8192; /* Initial size */
        bcast_symmetric_buf = shmem_malloc(bcast_buf_size);
        pSync_bcast = (long*)shmem_malloc(SHMEM_BCAST_SYNC_SIZE * sizeof(long));
        if (bcast_symmetric_buf == NULL || pSync_bcast == NULL) {
            fprintf(stderr, "Failed to allocate symmetric memory for broadcast\n");
            return -1;
        }
        
        /* Initialize pSync array */
        for (int i = 0; i < SHMEM_BCAST_SYNC_SIZE; i++) {
            pSync_bcast[i] = SHMEM_SYNC_VALUE;
        }
        shmem_barrier_all();
    }
    
    /* Reallocate if buffer is too small */
    if (bcast_buf_size < size) {
        shmem_free(bcast_symmetric_buf);
        bcast_buf_size = size * 2;
        bcast_symmetric_buf = shmem_malloc(bcast_buf_size);
        if (bcast_symmetric_buf == NULL) {
            fprintf(stderr, "Failed to reallocate symmetric memory for broadcast\n");
            return -1;
        }
        shmem_barrier_all();
    }
    
    /* Root copies data to symmetric buffer */
    if (shmem_my_pe() == root) {
        memcpy(bcast_symmetric_buf, buf, size);
    }
    
    shmem_barrier_all();
    
    /* Broadcast using native OpenSHMEM broadcast */
#if SHMEM_MINOR_VERSION <= 4
    /* OpenSHMEM 1.4 or earlier: use broadcast64 for 64-bit chunks */
    size_t num_longs = (size + sizeof(long) - 1) / sizeof(long);
    shmem_broadcast64((long*)bcast_symmetric_buf, (long*)bcast_symmetric_buf, 
                      num_longs, root, 0, 0, shmem_n_pes(), pSync_bcast);
#else
    /* OpenSHMEM 1.5+: use team-based broadcastmem */
    shmem_broadcastmem(internal_team, bcast_symmetric_buf, bcast_symmetric_buf, size, root);
#endif
    
    /* Copy from symmetric buffer to output buffer */
    memcpy(buf, bcast_symmetric_buf, size);
    
    return 0;

}

static int shmemx_oob_barrier(void *team_ctx){
    //shmem_team_t internal_team = (shmem_team_t) team_ctx;
#if 0
    return 0;
#else
    DEBUG_SHMEM("Calling a shmem_team_sync, and not a barrier\n");
    int result = 0;
    if (team_ctx != NULL){
        // shmem_team_t internal_team = (shmem_team_t) team_ctx;
        //shmem_team_sync(internal_team);
        // int result = 
        shmem_barrier_all();    
        if (result != 0){
            ERROR_SHMEM("TEAM_SYNC_FAILED\n");
            memset(NULL, 0, 10);
        }
        
    }
    DEBUG_SHMEM("Finished barrier_all, result %d\n", result);
    return 0;
#endif
}

static int shmemx_oob_gather(void *team_ctx, int root, void *sbuf, void *rbuf, int len){
    shmem_team_t internal_team = (shmem_team_t) team_ctx;
/*    int result = 0;
#if 0
    return result;
#else
    DEBUG_SHMEM("src %p dst %p\n", src, dst);
    int rank = shmem_team_my_pe(internal_team);

    void *src2 = shmem_malloc(len);
    DEBUG_SHMEM("starting oob_gather, contents: %s\n", (char *) src);

    result = shmem_fcollectmem(internal_team, src2, src, len);
    if (rank == root){
        memcpy(dst, src2, len);
    }
    shmem_quiet();
    shmem_barrier_all();
    DEBUG_SHMEM("Finished gather with result %d\n", result);
    return result;
#endif*/

    /* Credit to Dr. Ferrol Aderholt of NVIDIA on December 11, 2025 */
    static void *gather_symmetric_sbuf = NULL;
    static size_t gather_sbuf_size = 0;
#if SHMEM_MINOR_VERSION > 4
    static void *gather_symmetric_rbuf = NULL;
    static size_t gather_rbuf_size = 0;
    int npes = shmem_n_pes();
#endif
    
    /* Allocate or reallocate symmetric send buffer if needed */
    if (gather_symmetric_sbuf == NULL || gather_sbuf_size < len) {
        if (gather_symmetric_sbuf != NULL) {
            shmem_free(gather_symmetric_sbuf);
        }
        gather_sbuf_size = len * 2;
        gather_symmetric_sbuf = shmem_malloc(gather_sbuf_size);
        if (gather_symmetric_sbuf == NULL) {
            fprintf(stderr, "Failed to allocate symmetric memory for gather\n");
            return ENOMEM;
        }
    }

#if SHMEM_MINOR_VERSION > 4
    /* Allocate or reallocate symmetric receive buffer if needed */
    if (gather_symmetric_rbuf == NULL || gather_rbuf_size < len * npes) {
        if (gather_symmetric_rbuf != NULL) {
            shmem_free(gather_symmetric_rbuf);
        }
        gather_rbuf_size = len * npes * 2;
        gather_symmetric_rbuf = shmem_malloc(gather_rbuf_size);
        if (gather_symmetric_rbuf == NULL) {
            fprintf(stderr, "Failed to allocate symmetric receive memory for gather\n");
            return ENOMEM;
        }
    }
#endif
    
    /* Each PE copies its send data to symmetric buffer */
    memcpy(gather_symmetric_sbuf, sbuf, len);
    
#if SHMEM_MINOR_VERSION <= 4
    /* OpenSHMEM 1.4 or earlier: use manual getmem loop */
    shmem_barrier_all();
    
    if (shmem_my_pe() == root) {
        /* Root gathers data from all PEs */
        for (int i = 0; i < shmem_n_pes(); i++) {
            shmem_getmem_nbi((char*)rbuf + i * len, gather_symmetric_sbuf, len, i);
        }
    }
    
    shmem_barrier_all();
#else
    /* OpenSHMEM 1.5+: use native fcollectmem */
    shmem_fcollectmem(internal_team, gather_symmetric_rbuf, gather_symmetric_sbuf, len);

    /* Only root needs the result */
    if (shmem_my_pe() == root) {
        memcpy(rbuf, gather_symmetric_rbuf, len * npes);
    }
#endif
    return 0;

}

int shmemx_sharp_coll_init(shmemx_sharp_conf_t *sharp_conf, int pe, int local_pe, shmem_team_t team){
    shmemc_team_h c_team = (shmemc_team_h) team;

    int result = SHARP_COLL_SUCCESS;
    struct sharp_coll_init_spec init_spec = {};
    //    shmem_malloc(sizeof(struct sharp_coll_init_spec));
    //shmemu_assert(init_spec != NULL, "shmemx_sharp_coll_init: failed to allocate the init spec");

    init_spec.progress_func = NULL;
    init_spec.job_id = atol(sharp_conf->jobid);
    DEBUG_SHMEM("JobID: %ld\n", init_spec.job_id);
    init_spec.world_local_rank = local_pe;
    init_spec.enable_thread_support = 0;
    init_spec.oob_colls.barrier = shmemx_oob_barrier;
    init_spec.oob_colls.bcast = shmemx_oob_bcast;
    init_spec.oob_colls.gather = shmemx_oob_gather;
    init_spec.config = shmemx_sharp_coll_default_config;
    init_spec.group_channel_idx = 0;
    init_spec.config.ib_dev_list = sharp_conf->ibdev_list;
    DEBUG_SHMEM("ibdev_list: %s\n", sharp_conf->ibdev_list);
    init_spec.world_rank = shmem_team_my_pe(team);
    init_spec.world_size = shmem_team_n_pes(team);
    init_spec.oob_ctx = team;

    shmem_barrier_all();

    DEBUG_SHMEM("Calling sharp_coll_init\n");
    result = sharp_coll_init(&init_spec, &(coll_sharp_component.sharp_coll_ctx));
    shmem_barrier_all();
    if (result != SHARP_COLL_SUCCESS){
        //if (proc.li.rank == 0){
            ERROR_SHMEM("SHARP_Initilization failed. %d %s\n",
                    result, sharp_coll_strerror(result));
        //}
        shmemu_assert(result == SHARP_COLL_SUCCESS, "coll_init failed\n");
        memset(NULL, 0, 10);
        shmem_global_exit(result);
    }

    DEBUG_SHMEM("Finished coll_init; going to caps_query\n");

    result = sharp_coll_caps_query(coll_sharp_component.sharp_coll_ctx, 
            &coll_sharp_component.sharp_caps);

    if (result != SHARP_COLL_SUCCESS){
      //  if (proc.li.rank == 0){
            ERROR_SHMEM("Query network caps failed: %d %s\n",
                    result, sharp_coll_strerror(result));
     //   }
        shmemu_assert(result == SHARP_COLL_SUCCESS, "caps_query failed\n");
        shmem_global_exit(result);
        //memset(NULL, 0, 10);
    }

    //shmem_free(init_spec);
    //init_spec = NULL;
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
  //  char *devlist = malloc(strlen("mlx5_0") + 3);
  //  snprintf(devlist, strlen("mlx5_0")+3, "%s",
  //          "mlx5_0:1");

    sharp_conf->ibdev_list = "mlx5_0:1";

//    DEBUG_SHMEM("devlist: %s\n", devlist);
 
    char *id_str = malloc(JOBID_LEN);
    memset(id_str, 0, JOBID_LEN);
    char hostname[JOBID_LEN] = {'\0'};

    if (gethostname(hostname, JOBID_LEN) != 0){
        ERROR_SHMEM("Cannot find hostname!\n");
        shmem_global_exit(errno);
    }

    pid_t pid = getpid();
    char *jobID = getenv("SLURM_JOBID");
    DEBUG_SHMEM("jobID: %s\n", jobID);
    shmemu_assert(jobID != NULL, "setup_sharp_env: not in a SLURM job!\n");
    sprintf(id_str, "%d_%s_%d_0", atoi(jobID), hostname, pid);
    DEBUG_SHMEM("Job_ID: %s\n", id_str);


    DEBUG_SHMEM("ID_Str: %s\n", id_str);
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

 //   DEBUG_SHMEM("GetHostName: %s\n", name);
    if (sharp_res != 0){
        ERROR_SHMEM("Could not gethostname\n");
        shmem_global_exit(errno);
    }
    len[0] = strlen(name);
//    DEBUG_SHMEM("Length: %d\n", *len);
    if (rank < size-1){
        name[len[0]++] = ',';
    }
    sharp_res = shmem_int_fcollect(team, name_len, len, 1);
    if (sharp_res != 0){
        ERROR_SHMEM("internal shmem_fcollect failed\n");
        shmem_global_exit(errno);
    }
 //   DEBUG_SHMEM("name: %s\n", name);
    int bytes = 0;
    for (i = 0; i < size; i++){
        offsets[i] = bytes;
        bytes+=name_len[i];
    }
    bytes++;
 //   DEBUG_SHMEM("Byte count: %d\n", bytes);
    char *recv_buf = shmem_malloc(bytes);
    recv_buf[bytes-1] = 0;
    char *out_buf = malloc(bytes);
    sharp_res = shmem_char_collect(team, recv_buf, name, name_len[rank]);
    if (sharp_res != 0){
        ERROR_SHMEM("Internal shmem_collect failed!\n");
        shmem_global_exit(errno);
    }

 //   DEBUG_SHMEM("recv_buf: %s\n", recv_buf);

    memcpy(out_buf, recv_buf, bytes);

//    DEBUG_SHMEM("Out_buf: %s\n", out_buf);
   // shmem_free(len);
  //  shmem_free(name_len);
  //  shmem_free(recv_buf);
    return out_buf;
}


int shmemx_sharp_init(shmem_team_t team){
    shmemc_team_h teamh = (shmemc_team_h)(team);
    teamh->sharp_conf = malloc(sizeof(shmemx_sharp_conf_t));
    shmemu_assert(teamh->sharp_conf != NULL, "shmemx_sharp_init: Failed to malloc 1\n");
    teamh->sharp_module = malloc(sizeof(shmemx_coll_sharp_module_t));
    shmemu_assert(teamh->sharp_module != NULL, "shmemx_sharp_init: Failed to malloc 1\n");

    if (shmemx_setup_sharp_env(teamh->sharp_conf, team) != 0){
        ERROR_SHMEM("Failed to set up sharp env!\n");
        shmem_global_exit(-1);
    }
    DEBUG_SHMEM("dev_list: %s\n", teamh->sharp_conf->ibdev_list);
    teamh->sharp_conf->hostlist = shmemx_sharp_create_hostlist(team);
    if (teamh->sharp_conf->hostlist == NULL){
        ERROR_SHMEM("Failed to set up sharp hostlist!\n");
        shmem_global_exit(-1);
    }
    teamh->sharp_module->comm = (void *)(team);
    
    DEBUG_SHMEM("Beginning Coll init. world_team: %p, teamh->sharp_module->comm %p \n", SHMEM_TEAM_WORLD, teamh->sharp_module->comm);
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

