#if ENABLE_SHMEM_ENCRYPTION


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


#include "shmem_enc.h"
#include "shmemx.h"
#include "shmemu.h"
#include "shmem_mutex.h"
#include "shmemc.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pmix.h>


const unsigned char gcm_key[GCM_KEY_SIZE] = {'a','b','c','d','e','f','g','a','b','c','d','f','e','a','c','b','d','e','f','0','1','2','3','4','5','6','7','8','9','a','d','c'};


//unsigned char blocking_put_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char **nbi_put_ciphertext = NULL; //[NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbput_count = 0;

//unsigned char blocking_get_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char **nbi_get_ciphertext = NULL; //[NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbget_count = 0;

shmem_secure_attr_t *nb_put_ctr = NULL;
shmem_secure_attr_t *nb_get_ctr = NULL;


static volatile int active = -1;
EVP_CIPHER_CTX *enc_ctx = NULL;
EVP_CIPHER_CTX *dec_ctx = NULL;
int block_put_cipherlen = 0;
int block_get_cipheren = 0;
//unsigned char blocking_put_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
//unsigned char blocking_get_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};

//pmix_proc_t *my_second_pmix;
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


static const pmix_status_t ENC_SUCCESS = PMIX_EXTERNAL_ERR_BASE-1;
static const pmix_status_t DEC_SUCCESS = PMIX_EXTERNAL_ERR_BASE-2;


static void notif_cb_callback(pmix_status_t status,
      pmix_info_t *results, size_t nresults,
      void *cbdata){

   NO_WARN_UNUSED(status);
   NO_WARN_UNUSED(results);
   NO_WARN_UNUSED(nresults);
   NO_WARN_UNUSED(cbdata);
   DEBUG_SHMEM("Reception complete\n");

}

static void enc_notif_fn(size_t evhdlr_registration_id, pmix_status_t status,
                  const pmix_proc_t *source, pmix_info_t info[],size_t ninfo,
                  pmix_info_t results[], size_t nresults,
                  pmix_event_notification_cbfunc_fn_t cbfunc, 
                  void *cbdata){

    NO_WARN_UNUSED(cbfunc);
    NO_WARN_UNUSED(evhdlr_registration_id);
    NO_WARN_UNUSED(status);

    pmix_info_t buffer = info[1];
    uintptr_t dest = (uintptr_t)buffer.value.data.uint64;
    shmemu_assert(dest != 0, "dec_notif_fn: dest buffer is NULL!\n");
    int rank = info[2].value.data.uint32;
    int is_nonblocking = info[3].value.data.integer;
    size_t og_bytes = info[4].value.data.uint32;

    int source_rank = source ? source->rank : PMIX_RANK_UNDEF;
    shmemu_assert(source_rank != PMIX_RANK_UNDEF, "enc_notif_fn: source rank is undefined!");

    DEBUG_SHMEM( "pe: %d, source: %d, equal? %d\n", rank, source_rank, rank == source_rank); /* If not equal, then we're good to go */

    void *base = (void *) dest;
    int cipherlen = 0;

    shmemx_encrypt_single_buffer(base, proc.li.rank, (const void *)dest, rank, og_bytes, &cipherlen);

    DEBUG_SHMEM("Remote encryption went successfully\n");


}

static void dec_notif_fn(size_t evhdlr_registration_id, pmix_status_t status,
                  const pmix_proc_t *source, pmix_info_t info[],size_t ninfo,
                  pmix_info_t results[], size_t nresults,
                  pmix_event_notification_cbfunc_fn_t cbfunc, 
                  void *cbdata){

   // NO_WARN_UNUSED(cbfunc);
    NO_WARN_UNUSED(evhdlr_registration_id);
    NO_WARN_UNUSED(status);

    pmix_info_t buffer = info[1];
    uintptr_t dest = (uintptr_t)buffer.value.data.uint64;
    shmemu_assert(dest != 0, "dec_notif_fn: dest buffer is NULL!\n");
    pmix_info_t size = info[2];
    int  cipherlen = size.value.data.integer;
    pmix_info_t dest_rank = info[3];
    uint32_t pe = dest_rank.value.data.uint32;
    pmix_info_t non_blocking = info[4];
    int is_nonblocking = non_blocking.value.data.integer;
    pmix_info_t og_bytes = info[5];
    size_t og_size = og_bytes.value.data.uint32;
 
    int source_rank = source ? source->rank : PMIX_RANK_UNDEF;

    shmemu_assert(source_rank != PMIX_RANK_UNDEF, "dec_notif_fn: source rank is undefined!");

    DEBUG_SHMEM( "pe: %d, source: %d, equal? %d\n", pe, source_rank, pe == source_rank); /* If not equal, then we're good to go */

    DEBUG_SHMEM("decryption pe %d source rank %d address %p og_bytes %d; is_nonblocking %d,ciphertext_size %ld\n",
          pe, source_rank, dest, og_size, is_nonblocking, cipherlen);

    shmemu_assert(source != NULL, "dec_notif_fn: source proc struct is NULL\n");

 
    void *r_dest_ciphertext = malloc(cipherlen);
    DEBUG_SHMEM("Setting up ciphertext from address for cipherlen %d\n", cipherlen);
    memcpy(r_dest_ciphertext, (void*)dest, cipherlen);

    size_t cipher_len = 0;

    DEBUG_SHMEM("Decryption time!\n");

    shmemx_decrypt_single_buffer((unsigned char *)r_dest_ciphertext, pe, dest, proc.li.rank, og_size + AES_RAND_BYTES, cipherlen);

   /* Hopefully the above is "it" for the 
    * decryption. Would need to see in an actual application
    */
    
    //shmem_global_exit(-1);
    if (cbfunc){
       cbfunc(PMIX_SUCCESS, NULL, 0, NULL, NULL, cbdata);
    }
}



static void enc_notif_callbk(pmix_status_t status, size_t evhandler_ref, void *cbdata){
    volatile int *act = (volatile int *)cbdata;

    NO_WARN_UNUSED(status);
    NO_WARN_UNUSED(evhandler_ref);

    shmemu_assert(status == PMIX_SUCCESS,
            "shmem_enc_init_cb can't register event for encryption");
    *act = status;
}

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
        if (!(enc_ctx = EVP_CIPHER_CTX_new())){
            handleErrors("cipher failed to be created");
        }
        /* Begin using AES_256_gcm */
        res = EVP_EncryptInit_ex(enc_ctx, EVP_aes_256_gcm(), NULL, gcm_key, NULL);
        if (res != 1){
            handleErrors("failed to begin encryption portion");
        }

        res = EVP_CIPHER_CTX_ctrl(enc_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
        if (res != 1){
            handleErrors("Failed to set up the Initialization Vector Length");
        }
//    }

 //   if (defcp->dec_ctx == NULL){
        if (!(dec_ctx = EVP_CIPHER_CTX_new())){
            handleErrors("cipher failed to be created");
        }
        /* Begin using AES_256_gcm */
        res = EVP_DecryptInit_ex(dec_ctx, EVP_aes_256_gcm(), NULL, gcm_key, NULL);
        if (res != 1){
            handleErrors("failed to begin Decryption portion");
        }

        res = EVP_CIPHER_CTX_ctrl(dec_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
        if (res != 1){
            handleErrors("Failed to set up the Initialization Vector Length");
        }
//    }

    nbi_put_ciphertext = malloc(sizeof(unsigned char *)*NON_BLOCKING_OP_COUNT*2);
    nbi_get_ciphertext = malloc(sizeof(unsigned char *)*NON_BLOCKING_OP_COUNT*2);
    nb_put_ctr = (shmem_secure_attr_t *)malloc(sizeof(shmem_secure_attr_t) * NON_BLOCKING_OP_COUNT*2);
    nb_get_ctr = (shmem_secure_attr_t *)malloc(sizeof(shmem_secure_attr_t) * NON_BLOCKING_OP_COUNT*2);

    pmix_status_t sp = ENC_SUCCESS;
    active = -1;

    PMIx_Register_event_handler(&sp, 1, NULL, 0, enc_notif_fn,
            enc_notif_callbk, (void *)&active);
    while(active == -1){}


    shmemu_assert(active == 0, "shmem_enc_init: PMIx_enc_handler reg failed");

    active = -1;

    sp = DEC_SUCCESS;

    PMIx_Register_event_handler(&sp, 1, NULL, 0, dec_notif_fn,
            enc_notif_callbk, (void *)&active);
    while(active == -1){}
    shmemu_assert(active == 0, "shmem_enc_init: PMIx_dec_handler reg failed");
    DEBUG_SHMEM("Encrypion initialization has been completed\n");
    return;
}

int shmemx_encrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, 
        const void *sbuf, unsigned long long dest, size_t bytes, size_t *cipherlen){

   int res = 0;

    int const_bytes = AES_RAND_BYTES;
    DEBUG_SHMEM("Entering rand_bytes with cipherbuf+src: %p+0x%x\n",
          cipherbuf, src);
    RAND_bytes(cipherbuf+src, const_bytes);

    DEBUG_SHMEM("send_buf: %p, src %llu, dest %llu, cipherbuf: %p, enc_ctx: %p\n",
          sbuf, src, dest, cipherbuf, enc_ctx);
    
    DEBUG_SHMEM("Byte count :%d \n", (int)bytes);
    if((res = EVP_EncryptInit_ex(enc_ctx, EVP_aes_256_gcm(), NULL, gcm_key, cipherbuf+src)) != 1){
       ERROR_SHMEM("EncryptInit_ex from error %d: %s\n",
             ERR_get_error(), ERR_error_string(res, NULL));
       memset(NULL, 0, 10);
    }

   DEBUG_SHMEM("EncryptInit passed\n");

    if ((res = EVP_EncryptUpdate(enc_ctx,cipherbuf+src+const_bytes, cipherlen, ((const unsigned char *)(sbuf+dest)), (int)bytes))!=1){
       ERROR_SHMEM("Encrypt_Update failed: %s\n",
             ERR_error_string(ERR_get_error(), NULL));
       memset(NULL, 0, 10);
    }

    DEBUG_SHMEM("EncryptUpdate passed; block_put_cipherlen: %d\n", cipherlen);

    shmemu_assert(cipherlen >0, "shmemx_encrypt_single_buffer: ciphertext is 0...\n");

    if ((res = EVP_EncryptFinal_ex(enc_ctx, cipherbuf+const_bytes+src+(*cipherlen), cipherlen))!= 1){
       ERROR_SHMEM("EncryptFinal_ex failed: %s\n",
             ERR_error_string(ERR_get_error(), NULL));
       memset(NULL, 0, 10);
    }

    if ((res = EVP_CIPHER_CTX_ctrl(enc_ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_LEN, cipherbuf+const_bytes+src+bytes))!= 1){
        ERROR_SHMEM("EncryptFinal_ex failed: %s\n",
              ERR_error_string(ERR_get_error(), NULL));
        memset(NULL, 0, 10);
    }

    DEBUG_SHMEM("Ciphertext %p, Cipher_len %d\n", cipherbuf, (int)(*cipherlen));
    
  
    return 0;
}

int shmemx_decrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, 
        void *rbuf, unsigned long long dest, size_t bytes, size_t cipher_len){

//   struct ossl_param_st enc_data = {};
//   enc_data.key = gcm_key;
//   enc_data.data = cipherbuf+src;
//   enc_data.data_size = (cipher_len);
//   enc_data.return_size = bytes;

   DEBUG_SHMEM("Cipher_len placeholder %ld, expectd bytes %ld\n", cipher_len, bytes);
   int res = 0;

   DEBUG_SHMEM("cipherbuf %p, src %llu, rbuf %p, dest %llu, bytes %u\n",
         cipherbuf, src, rbuf, dest, bytes);


   if ((res = EVP_DecryptInit_ex(dec_ctx, EVP_aes_256_gcm(), NULL, gcm_key, cipherbuf+src)) != 1){
      ERROR_SHMEM("DecryptInit_ex failed: %d %s\n", ERR_get_error(), ERR_error_string(res, NULL));
      memset(NULL, 0, 10);
   }

   DEBUG_SHMEM("DecryptInit_ex passed \n");

   if ((res = EVP_DecryptUpdate(dec_ctx, ((unsigned char *)(rbuf+dest)), &cipher_len, cipherbuf+src+AES_RAND_BYTES, (bytes-AES_RAND_BYTES))) != 1){
      ERROR_SHMEM("DecryptUpdate failed: %d %s\n", ERR_get_error(), ERR_error_string(res, NULL));
      memset(NULL, 0, 10);
   }

   DEBUG_SHMEM("DecryptUpdated passed; cipherlen: %u\n", cipher_len);


    if ((res = EVP_CIPHER_CTX_ctrl(dec_ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_LEN, (cipherbuf+dest+(bytes))))!= 1){
       ERROR_SHMEM("CIPHER_CTX_ctrl failed: %d %s\n", ERR_get_error(), ERR_error_string(res, NULL));
       memset(NULL, 0, 10);

    }

    DEBUG_SHMEM("CTX_ctrl passed \n");
    if ((res = EVP_DecryptFinal_ex(dec_ctx, (rbuf+dest+(cipher_len)), &cipher_len)) != 1){
        /*handleErrors*/
       ERROR_SHMEM("Decryption Tag Verification Failed\n");
    }

    DEBUG_SHMEM("DecryptFinal_ex passed\n");
    return 0;
}


void shmemx_secure_put_nbi(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

    size_t cipherlen = 0;
    nbi_put_ciphertext[nbput_count] = malloc(sizeof(char) * nbytes);
    shmemx_encrypt_single_buffer(
            ((unsigned char *)nbi_put_ciphertext[nbput_count]),
            0, src, 0, nbytes, ((size_t *)(&cipherlen)));

    DEBUG_SHMEM("Encryption successful\n");
    shmemc_ctx_put_nbi(ctx, dest, 
            ((unsigned char*)(nbi_put_ciphertext[nbput_count][0])),
            cipherlen, pe);

    DEBUG_SHMEM("Non-blocking_put successful\n");

    shmemc_context_h ch = (shmemc_context_h) ctx; 
    uint64_t r_dest;  /* address on other PE */
    ucp_rkey_h r_key; /* rkey for remote address */
    DEBUG_SHMEM("Getting rkey and addr\n");
    get_remote_key_and_addr(ch, (uint64_t)dest, pe, &r_key, &r_dest);

    uint64_t local_dest;
    ucp_rkey_h local_rkey;

    get_remote_key_and_addr(ch, (uint64_t) src, proc.li.rank, &local_rkey, &local_dest);


    nb_put_ctr[nbput_count].src_pe = proc.li.rank;
    nb_put_ctr[nbput_count].dst_pe = pe;
    nb_put_ctr[nbput_count].plaintext_size = nbytes;
    nb_put_ctr[nbput_count].encrypted_size = cipherlen;
    nb_put_ctr[nbput_count].remote_buf_addr = r_dest;
    nb_put_ctr[nbput_count].local_buf = (uintptr_t) src;
    

    nbput_count++;

    

    /* TODO: SIGNAL DECRYPTION IN WAIT!! */
}

void shmemx_secure_put(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

       size_t cipherlen = 0;

    int res  = 0;
    unsigned char *blocking_put_ciphertext = calloc(1, nbytes*4);
    shmemx_encrypt_single_buffer(
            &(blocking_put_ciphertext[0]),
            proc.li.rank, src, pe, nbytes, ((size_t *)(&block_put_cipherlen)));

    DEBUG_SHMEM( "Encryption end, ciphertext: %p, cipherlen: %d \n",
          blocking_put_ciphertext, block_put_cipherlen);

    shmemc_ctx_put(ctx, dest, 
            blocking_put_ciphertext,
            block_put_cipherlen /*nbytes+AES_TAG_LEN+AES_RAND_BYTES*/, 
            pe);

    DEBUG_SHMEM( "Put end\n");

    /* Only begin PMIX Construction AFTER the put has been set up appropriately
     * */

    /* Step 1, Get PMIX_Rank from the other peer proc.
     * Step 2: Set up notification in custom range 
     * Step 3: Profit?
     */

    shmemc_context_h ch = (shmemc_context_h) ctx; 
    uint64_t r_dest;  /* address on other PE */
    ucp_rkey_h r_key; /* rkey for remote address */
    DEBUG_SHMEM("Getting rkey and addr\n");
    get_remote_key_and_addr(ch, (uint64_t)dest, pe, &r_key, &r_dest);


    pmix_status_t ps;
    pmix_info_t si[6];

    pmix_proc_t *procs;
    size_t nprocs = 1;
    procs = (pmix_proc_t *)malloc(sizeof(pmix_proc_t)*nprocs);
    DEBUG_SHMEM("Entering PMIX_LOAD_PROC_ID\n");
    PMIX_LOAD_PROCID(procs, my_second_pmix->nspace, pe);
    //  PMIX_LOAD_PROCID(&procs[1], PMIX_RANGE_NAMESPACE, dest);

    pmix_data_array_t pmix_darray;
    pmix_darray.size = nprocs;
    pmix_darray.type = PMIX_PROC;
    PMIX_DATA_ARRAY_CONSTRUCT(&pmix_darray, nprocs, PMIX_PROC);
    memcpy(pmix_darray.array, procs, nprocs*sizeof(pmix_proc_t));

   
    PMIX_INFO_CONSTRUCT(&si[0]);
    PMIX_INFO_LOAD(&si[0], PMIX_EVENT_CUSTOM_RANGE, &pmix_darray, PMIX_DATA_ARRAY);
    DEBUG_SHMEM("dest ptr: %p\n", r_dest);

    PMIX_INFO_CONSTRUCT(&si[1]);
    PMIX_LOAD_KEY(si[1].key, "Remote_secure_buffer");
    si[1].value.type = PMIX_UINT64;
    si[1].value.data.uint64 = (uint64_t)r_dest;
   // PMIX_INFO_LOAD(&si, PMIX_GRANK, &success, PMIX_INT);
    PMIX_INFO_CONSTRUCT(&si[2]);
    PMIX_LOAD_KEY(si[2].key, "Remote_buffer_enc_size");
    si[2].value.type = PMIX_INT;
    si[2].value.data.integer = block_put_cipherlen;

    PMIX_INFO_CONSTRUCT(&si[3]);
    PMIX_LOAD_KEY(si[3].key, "Destination_rank");
    si[3].value.type = PMIX_UINT32;
    si[3].value.data.uint32 = pe;

    PMIX_INFO_CONSTRUCT(&si[4]);
    PMIX_LOAD_KEY(si[4].key, "is_nonblocking");
    si[4].value.type = PMIX_INT;
    si[4].value.data.integer = 0;

    PMIX_INFO_CONSTRUCT(&si[5]);
    PMIX_LOAD_KEY(si[5].key, "og_bytes");
    si[5].value.type = PMIX_UINT32;
    si[5].value.data.uint32 = nbytes;


      DEBUG_SHMEM( "Starting signaling\n");
    ps = PMIx_Notify_event(DEC_SUCCESS, procs, PMIX_RANGE_CUSTOM, &(si[0]),
            6, NULL, NULL);

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

    
    shmemc_context_h ch = (shmemc_context_h) ctx; 
    uint64_t r_dest;  /* address on other PE */
    ucp_rkey_h r_key; /* rkey for remote address */
    DEBUG_SHMEM("Getting rkey and addr\n");
    get_remote_key_and_addr(ch, (uint64_t)src, pe, &r_key, &r_dest);


    pmix_status_t ps;
    pmix_info_t si[5];

    pmix_proc_t *procs;
    size_t nprocs = 1;
    procs = (pmix_proc_t *)malloc(sizeof(pmix_proc_t)*nprocs);
    DEBUG_SHMEM("Entering PMIX_LOAD_PROC_ID\n");
    PMIX_LOAD_PROCID(procs, my_second_pmix->nspace, pe);
    //  PMIX_LOAD_PROCID(&procs[1], PMIX_RANGE_NAMESPACE, dest);

    pmix_data_array_t pmix_darray;
    pmix_darray.size = nprocs;
    pmix_darray.type = PMIX_PROC;
    PMIX_DATA_ARRAY_CONSTRUCT(&pmix_darray, nprocs, PMIX_PROC);
    memcpy(pmix_darray.array, procs, nprocs*sizeof(pmix_proc_t));

   
    PMIX_INFO_CONSTRUCT(&si[0]);
    PMIX_INFO_LOAD(&si[0], PMIX_EVENT_CUSTOM_RANGE, &pmix_darray, PMIX_DATA_ARRAY);
    DEBUG_SHMEM("dest ptr: %p\n", r_dest);

    PMIX_INFO_CONSTRUCT(&si[1]);
    PMIX_LOAD_KEY(si[1].key, "Remote_secure_buffer");
    si[1].value.type = PMIX_UINT64;
    si[1].value.data.uint64 = (uint64_t)r_dest;
   // PMIX_INFO_LOAD(&si, PMIX_GRANK, &success, PMIX_INT);
 //   PMIX_INFO_CONSTRUCT(&si[2]);
 //   PMIX_LOAD_KEY(si[2].key, "Remote_buffer_enc_size");
 //   si[2].value.type = PMIX_INT;
 //   si[2].value.data.integer = block_get_cipherlen;

    PMIX_INFO_CONSTRUCT(&si[2]);
    PMIX_LOAD_KEY(si[2].key, "Destination_rank");
    si[2].value.type = PMIX_UINT32;
    si[2].value.data.uint32 = pe;

    PMIX_INFO_CONSTRUCT(&si[3]);
    PMIX_LOAD_KEY(si[3].key, "is_nonblocking");
    si[3].value.type = PMIX_INT;
    si[3].value.data.integer = 0;

    PMIX_INFO_CONSTRUCT(&si[4]);
    PMIX_LOAD_KEY(si[4].key, "og_bytes");
    si[4].value.type = PMIX_UINT32;
    si[4].value.data.uint32 = nbytes;


      DEBUG_SHMEM("Starting signaling\n");
    ps = PMIx_Notify_event(ENC_SUCCESS, procs, PMIX_RANGE_CUSTOM, &(si[0]),
            5, NULL, NULL);

    if (ps != PMIX_SUCCESS){
        shmemu_assert(ps == PMIX_SUCCESS,
                " shmem_ctx_secure_get: PMIx can't notify decryption: %s",
                PMIx_Error_string(ps));
    };

    DEBUG_SHMEM( "Encryption Signaling success? %s\n", ps == PMIX_SUCCESS ? "yes" : "no");

    PMIX_DATA_ARRAY_DESTRUCT(&pmix_darray);
    free(procs);

    
    shmemc_ctx_get_nbi(ctx, dest, 
            ((unsigned char *)nbi_get_ciphertext[nbget_count]),
            cipherlen, pe);

    nb_get_ctr[nbget_count].src_pe = pe;
    nb_get_ctr[nbget_count].dst_pe = proc.li.rank;
    nb_get_ctr[nbget_count].plaintext_size = nbytes;
    nb_get_ctr[nbget_count].encrypted_size = nbytes+AES_TAG_LEN+AES_RAND_BYTES;
    nb_get_ctr[nbget_count].local_buf_addr = (uintptr_t) dest;
    

    /*TODO: SIGNAL DECRYPTION IN WAIT!! */

}


void shmemx_secure_get(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){
    unsigned char *blocking_get_ciphertext = calloc(1, nbytes*4);

    size_t cipherlen = 0;


    shmemc_context_h ch = (shmemc_context_h) ctx; 
    uint64_t r_dest;  /* address on other PE */
    ucp_rkey_h r_key; /* rkey for remote address */
    DEBUG_SHMEM("Getting rkey and addr\n");
    get_remote_key_and_addr(ch, (uint64_t)src, pe, &r_key, &r_dest);


    pmix_status_t ps;
    pmix_info_t si[5];

    pmix_proc_t *procs;
    size_t nprocs = 1;
    procs = (pmix_proc_t *)malloc(sizeof(pmix_proc_t)*nprocs);
    DEBUG_SHMEM("Entering PMIX_LOAD_PROC_ID\n");
    PMIX_LOAD_PROCID(procs, my_second_pmix->nspace, pe);
    //  PMIX_LOAD_PROCID(&procs[1], PMIX_RANGE_NAMESPACE, dest);

    pmix_data_array_t pmix_darray;
    pmix_darray.size = nprocs;
    pmix_darray.type = PMIX_PROC;
    PMIX_DATA_ARRAY_CONSTRUCT(&pmix_darray, nprocs, PMIX_PROC);
    memcpy(pmix_darray.array, procs, nprocs*sizeof(pmix_proc_t));

   
    PMIX_INFO_CONSTRUCT(&si[0]);
    PMIX_INFO_LOAD(&si[0], PMIX_EVENT_CUSTOM_RANGE, &pmix_darray, PMIX_DATA_ARRAY);
    DEBUG_SHMEM("dest ptr: %p\n", r_dest);

    PMIX_INFO_CONSTRUCT(&si[1]);
    PMIX_LOAD_KEY(si[1].key, "Remote_secure_buffer");
    si[1].value.type = PMIX_UINT64;
    si[1].value.data.uint64 = (uint64_t)r_dest;
   // PMIX_INFO_LOAD(&si, PMIX_GRANK, &success, PMIX_INT);
 //   PMIX_INFO_CONSTRUCT(&si[2]);
 //   PMIX_LOAD_KEY(si[2].key, "Remote_buffer_enc_size");
 //   si[2].value.type = PMIX_INT;
 //   si[2].value.data.integer = block_get_cipherlen;

    PMIX_INFO_CONSTRUCT(&si[2]);
    PMIX_LOAD_KEY(si[2].key, "Destination_rank");
    si[2].value.type = PMIX_UINT32;
    si[2].value.data.uint32 = pe;

    PMIX_INFO_CONSTRUCT(&si[3]);
    PMIX_LOAD_KEY(si[3].key, "is_nonblocking");
    si[3].value.type = PMIX_INT;
    si[3].value.data.integer = 0;

    PMIX_INFO_CONSTRUCT(&si[4]);
    PMIX_LOAD_KEY(si[4].key, "og_bytes");
    si[4].value.type = PMIX_UINT32;
    si[4].value.data.uint32 = nbytes;


      DEBUG_SHMEM("Starting signaling\n");
    ps = PMIx_Notify_event(ENC_SUCCESS, procs, PMIX_RANGE_CUSTOM, &(si[0]),
            5, NULL, NULL);

    if (ps != PMIX_SUCCESS){
        shmemu_assert(ps == PMIX_SUCCESS,
                " shmem_ctx_secure_get: PMIx can't notify decryption: %s",
                PMIx_Error_string(ps));
    };

    DEBUG_SHMEM( "Encryption Signaling success? %s\n", ps == PMIX_SUCCESS ? "yes" : "no");

    PMIX_DATA_ARRAY_DESTRUCT(&pmix_darray);
    free(procs);



    shmemc_ctx_get(ctx, 
            (blocking_get_ciphertext),
            src,
            nbytes+AES_TAG_LEN+AES_RAND_BYTES, 
            pe);

    DEBUG_SHMEM("Get passed\n");

    shmemx_decrypt_single_buffer(blocking_get_ciphertext, 0, dest, 0, nbytes+AES_RAND_BYTES, -1 );

    
    free(blocking_get_ciphertext);

}

#endif /* ENABLE_SHMEM_ENCRYPTION */
