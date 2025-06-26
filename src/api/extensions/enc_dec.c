#if ENABLE_SHMEM_ENCRYPTION


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


#include "shmem_enc.h"
#include "shmemx.h"
#include "shmemu.h"
#include "shmem_mutex.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pmix.h>

static volatile int active = -1;

/*
 * -- helpers ----------------------------------------------------------------
 */

/*
 * shortcut to look up the UCP endpoint of a context
 */
inline static ucp_ep_h lookup_ucp_ep(shmemc_context_h ch, int pe) {
  return ch->eps[pe];
}

/*
 * find rkey for memory "region" on PE "pe"
 */
inline static ucp_rkey_h lookup_rkey(shmemc_context_h ch, size_t region,
                                     int pe) {
  return ch->racc[region].rinfo[pe].rkey;
}

/*
 * -- translation helpers ---------------------------------------------------
 */

/*
 * is the given address in this memory region?  Non-zero if yes, 0 if
 * not.
 */
inline static int in_region(uint64_t addr, size_t region) {
  const mem_info_t *mip = &proc.comms.regions[region].minfo[proc.li.rank];

  return (mip->base <= addr) && (addr < mip->end);
}

/*
 * find memory region that ADDR is in, or -1 if none
 */
inline static long lookup_region(uint64_t addr) {
  long r;

  /*
   * Let's search down from top heap to globals (#0) under
   * assumption most data in heaps and newest one is most likely
   * (may need to revisit)
   */
  for (r = proc.comms.nregions - 1; r >= 0; --r) {
    if (in_region(addr, (size_t)r)) {
      return r;
      /* NOT REACHED */
    }
  }

  return -1L;
}



/*
 * where the heap lives on PE "pe"
 */

inline static size_t get_base(size_t region, int pe) {
  return proc.comms.regions[region].minfo[pe].base;
}

inline static uint64_t translate_region_address(uint64_t local_addr,
                                                size_t region, int pe) {
  if (region == 0) {
    return local_addr;
  } else {
    const long my_offset = local_addr - get_base(region, proc.li.rank);

    if (my_offset < 0) {
      return 0;
    }

    return my_offset + get_base(region, pe);
  }
}

inline static uint64_t translate_address(uint64_t local_addr, int pe) {
  long r = lookup_region(local_addr);

  if (r < 0) {
    return 0;
  }

  return translate_region_address(local_addr, r, pe);
}

/*
 * All ops here need to find remote keys and addresses
 */
inline static void get_remote_key_and_addr(shmemc_context_h ch,
                                           uint64_t local_addr, int pe,
                                           ucp_rkey_h *rkey_p,
                                           uint64_t *raddr_p) {
  const long r = lookup_region(local_addr);

  shmemu_assert(r >= 0, MODULE ": can't find memory region for %p",
                (void *)local_addr);

  *rkey_p = lookup_rkey(ch, r, pe);
  *raddr_p = translate_region_address(local_addr, r, pe);
}


static const pmix_status_t ENC_SUCCESS = PMIX_ERR_START_DECRYPTION;
static const pmix_status_t DEC_SUCCESS = PMIX_ERR_START_ENCRYPTION;

void enc_notif_fn(size_t evhdlr_registration_id, pmix_status_t status,
        const pmix_proc_t *source, pmix_info_t info[],
        size_t ninfo, pmix_info_t results[],
        size_t nresults,
        pmix_event_notification_cbfunc_fn_t cbfunc){

    NO_WARN_UNUSED(cbfunc);
    NO_WARN_UNUSED(evhdlr_registration_id);
    NO_WARN_UNUSED(status);

}

void dec_notif_fn(size_t evhdlr_registration_id, pmix_status_t status,
        const pmix_proc_t *source, pmix_info_t info[],
        size_t ninfo, pmix_info_t results[],
        size_t nresults,
        pmix_event_notification_cbfunc_fn_t cbfunc){

    NO_WARN_UNUSED(cbfunc);
    NO_WARN_UNUSED(evhdlr_registration_id);
    NO_WARN_UNUSED(status);

    pmix_info_t buffer = info[0];
    void *dest = buffer.value.data.ptr;
    pmix_info_t size = info[1];
    size_t cipherlen = size.value.data.uint32;
    pmix_info_t dest_rank = info[2];
    uint32_t pe = dest_rank.value.data.uint32;
    pmix_info_t proc_rank = info[3];

    int source_rank = source ? source->rank : PMIX_RANK_UNDEF;

    DEBUG_SHMEM( "pe: %d, source: %d, equal? %d\n", pe, source_rank, pe == source_rank);

    shmemu_assert(source != NULL, "dec_notif_fn: source proc struct is NULL\n");

    shmemc_context_h ch = defcp; 
    uint64_t r_dest;  /* address on other PE */
    ucp_rkey_h r_key; /* rkey for remote address */

    get_remote_key_and_addr(ch, (uint64_t)dest, pe, &r_key, &r_dest);

    void *r_dest_ciphertext = (void *)r_dest; 

    //shmem_global_exit(-1);

}



static void enc_notif_callbk(pmix_status_t status, size_t evhandler_ref, void *cbdata){
    volatile int *act = (volatile int *)cbdata;

    NO_WARN_UNUSED(status);
    NO_WARN_UNUSED(evhandler_ref);

    shmemu_assert(status == PMIX_SUCCESS,
            "shmem_enc_init_cb can't register event for encryption");
    *act = status;
}

const unsigned char gcm_key[GCM_KEY_SIZE] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
//unsigned char blocking_put_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char **nbi_put_ciphertext = NULL; //[NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbput_count = 0;

//unsigned char blocking_get_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char **nbi_get_ciphertext = NULL; //[NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbget_count = 0;

static inline void handleErrors(char *message){
    ERR_print_errors_fp(stderr);
    shmemu_fatal("shmem_enc_dec: %s\n", message);
}

void shmemx_sec_init(){

   //  DEBUG_SHMEM( "Starting sec init\n");
   char *enc_dec = NULL;
   int res = 0;


   if ((enc_dec = getenv("SHMEM_ENABLE_ENCRYPTION")) != NULL){
      proc.env.shmem_encryption = atoi(enc_dec);
      assert(proc.env.shmem_encryption == 0 || proc.env.shmem_encryption == 1);
   }

//   DEBUG_SHMEM( "shmem_encryption: %d\n",
//         proc.env.shmem_encryption);

 //   if (defcp->enc_ctx == NULL){
        if (!(defcp->enc_ctx = EVP_CIPHER_CTX_new())){
            handleErrors("cipher failed to be created");
        }
        /* Begin using AES_256_gcm */
        res = EVP_EncryptInit_ex2(defcp->enc_ctx, EVP_aes_256_gcm(), gcm_key, NULL, NULL);
        if (res != 1){
            handleErrors("failed to begin encryption portion");
        }

        res = EVP_CIPHER_CTX_ctrl(defcp->enc_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
        if (res != 1){
            handleErrors("Failed to set up the Initialization Vector Length");
        }
//    }

 //   if (defcp->dec_ctx == NULL){
        if (!(defcp->dec_ctx = EVP_CIPHER_CTX_new())){
            handleErrors("cipher failed to be created");
        }
        /* Begin using AES_256_gcm */
        res = EVP_DecryptInit_ex2(defcp->dec_ctx, EVP_aes_256_gcm(), gcm_key, NULL, NULL);
        if (res != 1){
            handleErrors("failed to begin Decryption portion");
        }

        res = EVP_CIPHER_CTX_ctrl(defcp->dec_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
        if (res != 1){
            handleErrors("Failed to set up the Initialization Vector Length");
        }
//    }

    nbi_put_ciphertext = malloc(sizeof(unsigned char *)*NON_BLOCKING_OP_COUNT);
    nbi_get_ciphertext = malloc(sizeof(unsigned char *)*NON_BLOCKING_OP_COUNT);

    pmix_status_t sp = DEC_SUCCESS;
    active = -1;

    PMIx_Register_event_handler(&sp, 1, NULL, 0, enc_notif_fn,
            enc_notif_callbk, (void *)&active);
    while(active == -1){}
    //DEBUG_SHMEM( "Active 1: %d\n", active);

    shmemu_assert(active == 0, "shmem_enc_init: PMIx_enc_handler reg failed");

    active = -1;

    sp = ENC_SUCCESS;

    PMIx_Register_event_handler(&sp, 1, NULL, 0, dec_notif_fn,
            enc_notif_callbk, (void *)&active);
    while(active == -1){}
   // DEBUG_SHMEM( "Active 2: %d\n", active);

    shmemu_assert(active == 0, "shmem_enc_init: PMIx_dec_handler reg failed");

    return;
}

int shmemx_encrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, 
        const void *sbuf, unsigned long long dest, size_t bytes,
        shmem_ctx_t shmem_ctx, size_t *cipher_len){

        
    shmemc_context_h c2 = (shmemc_context_h)shmem_ctx;
    int const_bytes = AES_RAND_BYTES;
    DEBUG_SHMEM("Entering rand_buytes with cipherbuf+src: %p+0x%x\n",
          cipherbuf, src);
    RAND_bytes(cipherbuf+src, const_bytes);

    DEBUG_SHMEM("send_buf: %p, src %llu, dest %llu, cipherbuf: %p, c2: %p, enc_ctx: %p\n",
          sbuf, src, dest, cipherbuf, c2, c2->enc_ctx);

    struct ossl_param_st enc_data = {};
    enc_data.key = gcm_key;
    enc_data.data = cipherbuf+src;
    enc_data.data_size = bytes;
    EVP_EncryptInit_ex2(c2->enc_ctx, EVP_aes_256_gcm(), gcm_key, cipherbuf+src, &enc_data);;

    EVP_EncryptUpdate(c2->enc_ctx,cipherbuf+src+const_bytes,cipher_len, sbuf+dest, bytes);

    EVP_EncryptFinal_ex(c2->enc_ctx, cipherbuf+const_bytes+src+(*cipher_len), (int *)cipher_len);

    EVP_CIPHER_CTX_ctrl(c2->enc_ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_LEN, cipherbuf+const_bytes+src+bytes);
    
  
    return 0;
}

int shmemx_decrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, 
        const void *rbuf, unsigned long long dest, size_t bytes, 
        shmem_ctx_t shmem_ctx, size_t  *cipher_len){

    shmemc_context_h c2 = (shmemc_context_h)shmem_ctx;
    EVP_DecryptInit_ex2(c2->dec_ctx, NULL, NULL, NULL, cipherbuf+src);
    EVP_DecryptUpdate(c2->dec_ctx, rbuf+dest, cipher_len, cipherbuf+src+AES_RAND_BYTES, (bytes-AES_RAND_BYTES));
    EVP_CIPHER_CTX_ctrl(c2->dec_ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_LEN, (rbuf+dest+(*cipher_len)));
    if (!(EVP_DecryptFinal_ex(c2->dec_ctx, (rbuf+dest+(*cipher_len)), (int *)cipher_len) > 0)){
        handleErrors("Decryption Tag Verification Failed\n");
    }
    return 0;
}


void shmemx_secure_put_nbi(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

    size_t cipherlen = 0;
    nbi_put_ciphertext[nbput_count] = malloc(sizeof(char) * nbytes);
    shmemx_encrypt_single_buffer(
            ((unsigned char *)nbi_put_ciphertext[nbput_count]),
            0, src, 0, nbytes, ctx, &cipherlen);


    shmemc_ctx_put_nbi(ctx, dest, 
            ((unsigned char*)(nbi_put_ciphertext[nbput_count++][0])),
            cipherlen, pe);

    /* TODO: SIGNAL DECRYPTION IN WAIT!! */
}

void shmemx_secure_put(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

    unsigned char *blocking_put_ciphertext = calloc(1, nbytes);
       size_t cipherlen = 0;

       //DEBUG_SHMEM( "encruption start\n");
    int res  = 0;
    SHMEMT_MUTEX_NOPROTECT(
    shmemx_encrypt_single_buffer(
            (blocking_put_ciphertext),
            proc.li.rank, src, pe, nbytes, ctx, &cipherlen));

    DEBUG_SHMEM( "Encryption end\n");

    shmemc_ctx_put(ctx, dest, 
            blocking_put_ciphertext,
            cipherlen, pe);

    DEBUG_SHMEM( "Put end\n");

    /* Only begin PMIX Construction AFTER the put has been set up appropriately
     * */

    /* Step 1, Get PMIX_Rank from the other peer proc.
     * Step 2: Set up notification in custom range 
     * Step 3: Profit?
     */

    pmix_status_t ps;
    pmix_info_t si[4];

    pmix_proc_t *procs;
    size_t nprocs = 1;
    procs = (pmix_proc_t *)malloc(sizeof(pmix_proc_t)*nprocs);
    DEBUG_SHMEM("Entering PMIX_LOAD_PROC_ID\n");
    PMIX_LOAD_PROCID(procs, procs->nspace, pe);
    //  PMIX_LOAD_PROCID(&procs[1], PMIX_RANGE_NAMESPACE, dest);

    pmix_data_array_t pmix_darray;
    pmix_darray.size = nprocs;
    pmix_darray.type = PMIX_PROC;
    PMIX_DATA_ARRAY_CONSTRUCT(&pmix_darray, nprocs, PMIX_PROC);
    memcpy(pmix_darray.array, procs, nprocs*sizeof(pmix_proc_t));

    
    PMIX_INFO_CONSTRUCT(&si[0]);
    PMIX_LOAD_KEY(si[0].key, "Remote_secure_buffer");
    si[0].value.type = PMIX_POINTER;
    si[0].value.data.ptr = dest;
   // PMIX_INFO_LOAD(&si, PMIX_GRANK, &success, PMIX_INT);
    PMIX_INFO_CONSTRUCT(&si[1]);
    PMIX_LOAD_KEY(si[1].key, "Remote_buffer_enc_size");
    si[1].value.type = PMIX_UINT32;
    si[1].value.data.uint32 = cipherlen;

    PMIX_INFO_CONSTRUCT(&si[2]);
    PMIX_LOAD_KEY(si[2].key, "Destination_rank");
    si[2].value.type = PMIX_UINT32;
    si[2].value.data.uint32 = pe;

    PMIX_INFO_CONSTRUCT(&si[3]);
    PMIX_INFO_LOAD(&si[3], "Proc_Data_array", &pmix_darray, PMIX_DATA_ARRAY);
    DEBUG_SHMEM( "Starting signaling\n");
    ps = PMIx_Notify_event(PMIX_ERR_START_DECRYPTION, procs, PMIX_RANGE_CUSTOM, &(si[0]),
            4, NULL, NULL);

    if (ps != PMIX_SUCCESS){
        shmemu_assert(ps == PMIX_SUCCESS,
                " shmem_ctx_secure_put: PMIx can't notify decryption: %s",
                PMIx_Error_string(ps));
    };

    DEBUG_SHMEM( "Signaling success? %s\n", ps == PMIX_SUCCESS ? "yes" : "no");

    PMIX_DATA_ARRAY_DESTRUCT(&pmix_darray);
    free(procs);
    free (blocking_put_ciphertext);
}



void shmemx_secure_get_nbi(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

    nbi_get_ciphertext[nbget_count] = malloc(sizeof(char) * nbytes);

    size_t cipherlen = 0;

    /* TODO: HOW TO GET BUFFER APPROPRIATELY FROM DEST SIDE FIXME  */
    /* signal_dest_encrypt ();  */

   // int res = shmemx_encrypt_single_buffer(
   //         ((unsigned char *)&(nbi_get_ciphertext[nbget_count][0])),
  //          0, src, 0, nbytes, ctx, &cipherlen);

    shmemc_ctx_get_nbi(ctx, dest, 
            ((unsigned char *)nbi_get_ciphertext[nbget_count++]),
            cipherlen, pe);

    /*TODO: SIGNAL DECRYPTION IN WAIT!! */

}


void shmemx_secure_get(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){
    unsigned char *blocking_get_ciphertext = calloc(1, nbytes);

    size_t cipherlen = 0;

    /* TODO: HOW TO GET BUFFER APPROPRIATELY FROM DEST SIDE FIXME  
     * Could technically bring out appropriate buffer for this from remote side? 
     * But memory is still on the host side :/ Hmmm... */
    /* signal_dest_encrypt ();  */

//    int res = shmemx_encrypt_single_buffer(
//            ((unsigned char *)&(nbi_put_ciphertext[nbput_count][0])),
//            0, src, 0. nbytes, ctx, &cipherlen);

    shmemc_ctx_get(ctx, 
            (blocking_get_ciphertext),
            src,
            nbytes+AES_TAG_LEN+AES_RAND_BYTES, 
            pe);

    shmemx_decrypt_single_buffer(blocking_get_ciphertext, 0, dest, 0, nbytes+AES_RAND_BYTES, ctx, &cipherlen);

    free(blocking_get_ciphertext);

}

#endif /* ENABLE_SHMEM_ENCRYPTION */
