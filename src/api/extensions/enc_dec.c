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
#include <omp.h>


const unsigned char gcm_key[GCM_KEY_SIZE] = {'a','b','c','d','e','f','g','a','b','c','d','f','e','a','c','b','d','e','f','0','1','2','3','4','5','6','7','8','9','a','d','c'};

unsigned char blocking_put_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char nbi_put_ciphertext[NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbput_count = 0;

//unsigned char blocking_get_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char **nbi_get_ciphertext = NULL; //[NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbget_count = 0;

shmem_secure_attr_t *nb_put_ctr = NULL;
shmem_secure_attr_t *nb_get_ctr = NULL;

EVP_CIPHER_CTX *openmp_enc_ctx[MAX_THREAD_COUNT]; /* Can I get around the AEAD limitation from BoringSSL? */
EVP_CIPHER_CTX *openmp_dec_ctx[MAX_THREAD_COUNT]; /* Answer to the above comment? Yes */


static volatile int active = -1;

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

  shmemu_assert(r >= 0, "shmem_enc/dec, get_rkey/addr: can't find memory region for %p",
                (void *)local_addr);

  *rkey_p = lookup_rkey(ch, r, pe);
  *raddr_p = translate_region_address(local_addr, r, pe);
}

/*ucs_status_t put_handler(void *arg, const void *header, size_t h_size,
        void *data, size_t len, const ucp_am_recv_param_t *param){

    NO_WARN_UNUSED(arg);
    NO_WARN_UNUSED(header);
    NO_WARN_UNUSED(h_size);

    int rank = proc.li.rank;
    func_args_t *func_data = (func_args_t *)data;
    uint64_t dest = (uint64_t)(func_data->local_buffer);
    uint64_t r_dest = func_data->remote_buffer;
//    const long r = lookup_region(r_dest);
//    r_dest = translate_region_address(dest, r, rank);
    shmemu_assert (r_dest >= 0, "put_handler: rdest is 0, can't find region of %p", (void *) dest);
   // memcpy((void *)r_dest, (void *)dest, func_data->local_size+AES_TAG_LEN+AES_RAND_BYTES);

    switch (func_data->optype){
        case PT2PT:
            shmemx_decrypt_single_buffer(dest, 0, r_dest, 0, func_data->local_size + AES_TAG_LEN, ((size_t)(func_data->local_size)));
            break;
        case COLL:
            shmemx_decrypt_single_buffer(dest, func_data->src_pe, r_dest, func_data->dst_pe, func_data->local_size + AES_TAG_LEN, ((size_t)(func_data->local_size)));
        default:
            ERROR_SHMEM("You goofball!\n");
            shmemu_assert(0, "Error in decryption setup");
    }
    return UCS_OK;
}

ucs_status_t get_enc_handler(void *arg, const void *header, size_t h_size,
        void *data, size_t len, const ucp_am_recv_param_t *param){

    int rank = proc.li.rank;
    func_args_t *func_data = (func_args_t *)data;
    uint64_t dest = (uint64_t)(func_data->remote_buffer);
    uint64_t r_dest;
    const long r = lookup_region(dest);
    r_dest = translate_region_address(dest, r, rank);
    shmemu_assert (r_dest >= 0, "put_handler: rdest is 0, can't find region of %p", (void *) dest);
    //memcpy((void *)r_dest, (void *)dest, func_data->local_size+AES_TAG_LEN+AES_RAND_BYTES);

    switch (func_data->optype){
        case PT2PT:
            shmemx_encrypt_single_buffer((unsigned char *)r_dest, 0, (void *)r_dest, 0, func_data->local_size + AES_TAG_LEN, ((size_t *)(&func_data->encrypted_size)));
            break;
        case COLL:
            shmemx_encrypt_single_buffer((unsigned char *)r_dest, func_data->src_pe, (void *)r_dest, func_data->dst_pe, func_data->local_size + AES_TAG_LEN, ((size_t)(func_data->local_size)));
        default:
            ERROR_SHMEM("You goofball!\n");
            shmemu_assert(0, "Error in decryption setup");
    }

    size_t res_size = func_data->local_size+AES_TAG_LEN+AES_RAND_BYTES;

    func_args_t *response = (func_args_t *)malloc(sizeof(func_args_t) + res_size); //func_data->local_size+AES_TAG_LEN+AES_RAND_BYTES);
    response->local_size = func_data->local_size;
    memcpy(response->remote_buffer, r_dest, res_size);// func_data->local_size+AES_TAG_LEN+AES_RAND_BYTES);
    response->optype = func_data->optype;


    ucp_request_param_t ack_param = {
        .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
        .cb.send = NULL,
        .datatype = ucp_dt_make_contig(1)
    };
    ucp_am_send_nbx(param->reply_ep, AM_GET_DEC_RESPONSE, NULL, 0, response, sizeof(func_args_t)+res_size, &ack_param);

    return UCS_OK;
}


ucs_status_t get_dec_resp_handler(void *arg, const void *header, size_t h_size,
        void *data, size_t len, const ucp_am_recv_param_t *param){

    int rank = proc.li.rank;
    func_args_t *func_data = (func_args_t *)data;
    uint64_t dest = (uint64_t)(func_data->remote_buffer);
    uint64_t r_dest;
    const long r = lookup_region(r_dest);
    r_dest = translate_region_address(dest, r, rank);
    shmemu_assert (r_dest >= 0, "put_handler: rdest is 0, can't find region of %p", (void *) dest);
    //memcpy((void *)r_dest, (void *)dest, func_data->local_size+AES_TAG_LEN+AES_RAND_BYTES);

    switch (func_data->optype){
        case PT2PT:
            shmemx_decrypt_single_buffer((unsigned char *)dest, 0, (void *)r_dest, 0, func_data->local_size + AES_TAG_LEN, ((size_t)(func_data->local_size)));
            break;
        case COLL:
            shmemx_decrypt_single_buffer((unsigned char *)dest, func_data->src_pe, (void *)r_dest, func_data->dst_pe, func_data->local_size + AES_TAG_LEN, ((size_t)(func_data->local_size)));
        default:
            ERROR_SHMEM("You goofball!\n");
            shmemu_assert(0, "Error in decryption setup");
    }

    ucp_request_param_t ack_param = {
        .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
        .cb.send = NULL,
        .datatype = ucp_dt_make_contig(1)
    };
    ucp_am_send_nbx(param->reply_ep, AM_GET_DEC_RESPONSE, NULL, 0, 1, sizeof(int), &ack_param);

    return UCS_OK;
}

*/

/* OLD STUFF BELOW!!! NEW STUFF ABOVE!!! */



static const pmix_status_t ENC_SUCCESS = PMIX_EXTERNAL_ERR_BASE-1;
static const pmix_status_t DEC_SUCCESS = PMIX_EXTERNAL_ERR_BASE-2;

int put_thread_no = 0;
int get_thread_no = 0;
static void notif_cb_callback(pmix_status_t status,
            void *cbdata){

   //NO_WARN_UNUSED(status);
   NO_WARN_UNUSED(cbdata);
   shmemu_assert(status == PMIX_SUCCESS, "notif_cb_callback: PMIX_failed here\n");
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
    shmemu_assert(dest != 0, "enc_notif_fn: dest buffer is NULL!\n");
    int rank = info[3].value.data.uint32;
    //int is_nonblocking = info[4].value.data.integer;
    size_t og_bytes = info[6].value.data.uint32;

    int source_rank = source ? (int)(info[4].value.data.uint32) : PMIX_RANK_UNDEF;
    shmemu_assert(source_rank != PMIX_RANK_UNDEF, "enc_notif_fn: source rank is undefined!");
    
    void *base = (void *) dest;
    size_t cipherlen = 0;

    shmemx_encrypt_single_buffer_omp(base, source_rank, (const void *)dest, rank, og_bytes, &cipherlen);

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
    pmix_info_t non_blocking = info[5];
    int is_nonblocking = non_blocking.value.data.integer;
    pmix_info_t og_bytes = info[6];
    size_t og_size = og_bytes.value.data.uint32;
 
    int source_rank = source ? (int)(info[4].value.data.uint32) : PMIX_RANK_UNDEF;

    shmemu_assert(source_rank != PMIX_RANK_UNDEF, "dec_notif_fn: source rank is undefined!");

   
    DEBUG_SHMEM("decryption pe %d source rank %d address %p og_bytes %lu; is_nonblocking %d,ciphertext_size %d\n",
          pe, source_rank, (void *)dest, og_size, is_nonblocking, cipherlen);

    shmemu_assert(source != NULL, "dec_notif_fn: source proc struct is NULL\n");

 
    //void *r_dest_ciphertext = malloc(cipherlen + AES_RAND_BYTES+AES_TAG_LEN);
    //memcpy(r_dest_ciphertext, (void*)dest, cipherlen + AES_RAND_BYTES+AES_TAG_LEN);

    size_t cipher_len = cipherlen;

    int thread_no = shmemx_decrypt_single_buffer_omp((unsigned char *)dest, pe, (void *)dest, source_rank, og_size, (int)cipher_len);


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



void shmemx_ctx_sec_init(shmem_ctx_t shmem_ctx){
   int res = 0;
   shmemc_context_h ch = (shmemc_context_h)shmem_ctx;
   
   shmemu_assert(ch != NULL, "shmemx_ctx_sec_init : context is NULL!\n");
   if (!(ch->enc_ctx = EVP_CIPHER_CTX_new())){
      handleErrors("cipher failed to be created");
   }
   /* Begin using AES_256_gcm */
   res = EVP_EncryptInit_ex(ch->enc_ctx, EVP_aes_256_gcm(), NULL, gcm_key, NULL);
   if (res != 1){
      handleErrors("failed to begin encryption portion");
   }

   res = EVP_CIPHER_CTX_ctrl(ch->enc_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
   if (res != 1){
      handleErrors("Failed to set up the Initialization Vector Length");
   }
   if (!(ch->dec_ctx = EVP_CIPHER_CTX_new())){
      handleErrors("cipher failed to be created");
   }
   /* Begin using AES_256_gcm */
   res = EVP_DecryptInit_ex(ch->dec_ctx, EVP_aes_256_gcm(), NULL, gcm_key, NULL);
   if (res != 1){
      handleErrors("failed to begin Decryption portion");
   }

   res = EVP_CIPHER_CTX_ctrl(ch->dec_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
   if (res != 1){
      handleErrors("Failed to set up the Initialization Vector Length");
   }

}

void shmemx_sec_init(){

    char *enc_dec = NULL;
    int res = 0;


    if ((enc_dec = getenv("SHMEM_ENABLE_ENCRYPTION")) != NULL){
        proc.env.shmem_encryption = atoi(enc_dec);
        assert(proc.env.shmem_encryption == 0 || proc.env.shmem_encryption == 1);
    }

    if (!(defcp->enc_ctx = EVP_CIPHER_CTX_new())){
        handleErrors("cipher failed to be created");
    }
    /* Begin using AES_256_gcm */
    res = EVP_EncryptInit_ex(defcp->enc_ctx, EVP_aes_256_gcm(), NULL, gcm_key, NULL);
    if (res != 1){
        handleErrors("failed to begin encryption portion");
    }

    res = EVP_CIPHER_CTX_ctrl(defcp->enc_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
    if (res != 1){
        handleErrors("Failed to set up the Initialization Vector Length");
    }
    if (!(defcp->dec_ctx = EVP_CIPHER_CTX_new())){
        handleErrors("cipher failed to be created");
    }
    /* Begin using AES_256_gcm */
    res = EVP_DecryptInit_ex(defcp->dec_ctx, EVP_aes_256_gcm(), NULL, gcm_key, NULL);
    if (res != 1){
        handleErrors("failed to begin Decryption portion");
    }

    res = EVP_CIPHER_CTX_ctrl(defcp->dec_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
    if (res != 1){
        handleErrors("Failed to set up the Initialization Vector Length");
    }

    //nbi_put_ciphertext = malloc(sizeof(unsigned char *)*NON_BLOCKING_OP_COUNT*2);
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
    //DEBUG_SHMEM("Encrypion initialization has been completed\n");
    //


    int i = 0;
    for(i = 0; i < MAX_THREAD_COUNT; i++){

       if (!(openmp_enc_ctx[i] = EVP_CIPHER_CTX_new())){
          handleErrors("OpenMP cipher failed to be created");
       }
       /* Begin using AES_256_gcm */
       res = EVP_EncryptInit_ex(openmp_enc_ctx[i], EVP_aes_256_gcm(), NULL, gcm_key, NULL);
       if (res != 1){
          handleErrors("failed to begin encryption portion");
       }

       res = EVP_CIPHER_CTX_ctrl(openmp_enc_ctx[i], EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
       if (res != 1){
          handleErrors("Failed to set up the Initialization Vector Length");
       }

       if (!(openmp_dec_ctx[i] = EVP_CIPHER_CTX_new())){
          handleErrors("OpenMP cipher failed to be created");
       }
       /* Begin using AES_256_gcm */
       res = EVP_DecryptInit_ex(openmp_dec_ctx[i], EVP_aes_256_gcm(), NULL, gcm_key, NULL);
       if (res != 1){
          handleErrors("failed to begin encryption portion");
       }

       res = EVP_CIPHER_CTX_ctrl(openmp_dec_ctx[i], EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
       if (res != 1){
          handleErrors("Failed to set up the Initialization Vector Length");
       }
    }
    return;
}


int shmemx_encrypt_single_buffer_omp(unsigned char *cipherbuf, unsigned long long src,
      const void *sbuf, unsigned long long dest, size_t bytes, size_t *cipherlen){

   int res = 0;
   int segment_count = 0, count = 0;
   int local_cipherlen = 0;
   int max_data = 0;
   *cipherlen = 0; /* Just in case */

   int thread_no = 1; // Starting thread point

   if (bytes < SIX_FOUR_K){
      thread_no = 1;
   }else if (bytes < ONE_TWO_EIGHT_K){
      thread_no = 2;
   }else if (bytes < TWO_FIVE_SIX_K){
      thread_no = 4;
   }else if (bytes < FIVE_TWELVE_K){
      thread_no = 8;
   }else{
      thread_no = 16;
   }

   put_thread_no = thread_no;

   int data = bytes / thread_no;
   //data++;
   DEBUG_SHMEM("bytes = %d  max_thread_no %d\n",  data, thread_no);

   if (bytes <= 16){
      segment_count = 1;
      data = bytes;
   }else{
      segment_count = (bytes-1)/data + 1;
   }

   int enc_data = data;

   int position = 0;

   int temp_cipherlen = 0;

   //unsigned char *key = &(gcm_key[0]);
   DEBUG_SHMEM("Segment_count %d, data = %d, bytes = %d  max_thread_no %d\n", segment_count, data, bytes, thread_no);

 //   if (segment_count == 1){
 //      return shmemx_encrypt_single_buffer(cipherbuf, src, sbuf, dest, bytes, cipherlen);
 //   }

   DEBUG_SHMEM("[START_ENCRYPTION] Starting parallel for plaintext: %s \n", (char *)sbuf);
#pragma omp parallel for schedule (dynamic) default(none) private(max_data, position, res, local_cipherlen, enc_data) shared(src, dest, openmp_enc_ctx, stdout,stderr, segment_count, data, sbuf, cipherbuf, temp_cipherlen, proc, bytes, gcm_key) num_threads(thread_no)
   for (count = 0; count < segment_count ; count++){

      int tn = omp_get_thread_num();
      //int cipher_temp = 0;
      //
      //cipherbuf[count*(data+AES_TAG_LEN+AES_RAND_BYTES)+src]
   //   void *tmp_buf = cipherbuf + (count * (data+AES_TAG_LEN+AES_RAND_BYTES))+src;
      position = count * (data + AES_TAG_LEN + AES_RAND_BYTES);
      int pos2 = count * data;
      void *tmp_buf = cipherbuf + position;
      void *tmp_buf2 = sbuf + pos2;



            //  RAND_bytes(tmp_buf, AES_RAND_BYTES);
      EVP_CIPHER_CTX *local_ctx = openmp_enc_ctx[tn];
      RAND_bytes(tmp_buf+src, AES_RAND_BYTES);
      enc_data = data;
      max_data = enc_data+AES_TAG_LEN;
     

      if ((count == segment_count - 1) ){
         enc_data = bytes - (data * (segment_count - 1));
         max_data = enc_data + AES_TAG_LEN;
         DEBUG_SHMEM("Last count enc: enc_data %d, max_data %d\n",
               enc_data, max_data);
      }
      DEBUG_SHMEM("[T_%d] local_ctx %p enc_data %d max_data %d tmp_buf: %p (cipher buf %p + count %d * (data %d + AES_TAG_LEN %d + AES_RAND_BYTES %d))\n", 
            tn, local_ctx, enc_data, max_data, tmp_buf, (void *)cipherbuf+src, count, enc_data, AES_TAG_LEN, AES_RAND_BYTES);

      if ((res = EVP_EncryptInit_ex(local_ctx, NULL, NULL, NULL, tmp_buf)) != 1){
         ERROR_SHMEM("[T_%d] EncryptInit_ex from error %lu: %s\n",
               tn, ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
         memset(NULL, 0, 10);
      }

      DEBUG_SHMEM("[T_%d] EncryptInit_ex passed\n", tn);

      if ((res = EVP_EncryptUpdate(local_ctx, tmp_buf+src+AES_RAND_BYTES, &local_cipherlen, ((const unsigned char *)(tmp_buf2 + dest)), (int) enc_data)) != 1){
         ERROR_SHMEM("[T_%d] EncryptUpdate Failed: %lu %s\n", 
               tn, ERR_get_error(), ERR_error_string(res, NULL));
         memset(NULL, 0, 10);
      }

//      DEBUG_SHMEM("[T_%d] EncrypUpdate passed with cipherlen %d\n", tn, local_cipherlen);
      temp_cipherlen += local_cipherlen; /* Adds to the global cipherlen length */

      DEBUG_SHMEM("[T_%d] Entering final with local_ctx %p tmp_buf %p + AES_TAG_LEN %d + src %llu + local_cipherlen %d\n",
            tn, local_ctx, tmp_buf, AES_TAG_LEN, src, local_cipherlen);

         if ((res = EVP_EncryptFinal_ex(local_ctx, tmp_buf+AES_RAND_BYTES+src+local_cipherlen, &local_cipherlen)) != 1){
            ERROR_SHMEM("[T_%d] EncryptFinal_ex failed: %lu %s\n", tn,
                  ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
            memset(NULL, 0, 10);
         }
      

//#pragma omp barrier

      DEBUG_SHMEM("[T_%d] EncrypFinal_ex passed\n", tn);
      if ((res = EVP_CIPHER_CTX_ctrl(local_ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_LEN, tmp_buf+AES_RAND_BYTES+local_cipherlen))!= 1){
         ERROR_SHMEM("[T_%d]: CTX_CTRL: %s\n", tn,
               ERR_error_string(ERR_get_error(), NULL));
         memset(NULL, 0, 10);
      }
      DEBUG_SHMEM("[T_%d] CIPHER_CTX_CTRL passed\n", tn);

   }

   *cipherlen = temp_cipherlen;
//   DEBUG_SHMEM("[END_ENCRYPTION] Final cipherlen: %lu, CIPHERTEXT: %s\n", *cipherlen, cipherbuf);

   return segment_count;
}


int shmemx_encrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, 
        const void *sbuf, unsigned long long dest, size_t bytes, size_t *cipherlen){

   int res = 0;
   int len = 0, temp_len = 0;

    int const_bytes = AES_RAND_BYTES;
    //DEBUG_SHMEM("Entering rand_bytes with cipherbuf+src: %p+0x%x\n",
    //      cipherbuf, src);
    RAND_bytes(cipherbuf+src, const_bytes);

 //   DEBUG_SHMEM("send_buf: %p, src %llu, dest %llu, cipherbuf: %p, defcp->enc_ctx: %p\n",
 //         sbuf, src, dest, cipherbuf, defcp->enc_ctx);
    
//    DEBUG_SHMEM("Byte count :%d \n", (int)bytes);
    if((res = EVP_EncryptInit_ex(defcp->enc_ctx, EVP_aes_256_gcm(), NULL, gcm_key, cipherbuf+src)) != 1){
       ERROR_SHMEM("EncryptInit_ex from error %lu: %s\n",
             ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
       memset(NULL, 0, 10);
    }

   DEBUG_SHMEM("EncryptInit passed\n");

    if ((res = EVP_EncryptUpdate(defcp->enc_ctx,cipherbuf+src+const_bytes, &len, ((const unsigned char *)(sbuf+dest)), (int)bytes))!=1){
       ERROR_SHMEM("Encrypt_Update failed: %s\n",
             ERR_error_string(ERR_get_error(), NULL));
       memset(NULL, 0, 10);
    }

    DEBUG_SHMEM("EncryptUpdate passed; block_put_cipherlen: %lu\n", (size_t) len);

    shmemu_assert(len >0, "shmemx_encrypt_single_buffer: ciphertext is 0...\n");

    temp_len = len;

    if ((res = EVP_EncryptFinal_ex(defcp->enc_ctx, cipherbuf+const_bytes+src+(len), &len))!= 1){
       ERROR_SHMEM("EncryptFinal_ex failed: %s\n",
             ERR_error_string(ERR_get_error(), NULL));
       memset(NULL, 0, 10);
    }

    DEBUG_SHMEM("EncryptFinal passed. Len: %d\n", temp_len+len);

    if ((res = EVP_CIPHER_CTX_ctrl(defcp->enc_ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_LEN, cipherbuf+const_bytes+src+bytes))!= 1){
        ERROR_SHMEM("CTX_CTRL: %s\n",
              ERR_error_string(ERR_get_error(), NULL));
        memset(NULL, 0, 10);
    }
    temp_len+=len;

    DEBUG_SHMEM("Ciphertext %p, Cipher_len %d\n", cipherbuf, temp_len);

    *cipherlen = temp_len;
  
    return 0;
}

int shmemx_decrypt_single_buffer_omp(unsigned char *cipherbuf, unsigned long long src, 
        void *rbuf, unsigned long long dest, size_t bytes, size_t cipher_len){

   int res = 0;
   int segment_count = 0, count = 0;
   int local_cipherlen = 0;
   int max_data = 0;

   int other_cipherlen = 0;
   int thread_no = 1; // Starting thread point

   if (bytes < SIX_FOUR_K){
      thread_no = 1;
   }else if (bytes < ONE_TWO_EIGHT_K){
      thread_no = 2;
   }else if (bytes < TWO_FIVE_SIX_K){
      thread_no = 4;
   }else if (bytes < FIVE_TWELVE_K){
      thread_no = 8;
   }else{
      thread_no = 16;
   }

   int data = bytes / thread_no;
   DEBUG_SHMEM("Data: %d\n", data);
   max_data = data + AES_RAND_BYTES;
   data++;

   if (bytes <=16){
      segment_count = 1;
      data = bytes;
   }else{
      segment_count = (bytes-1)/data + 1;
   }

   int position = 0;
   int enc_data = data;
   max_data = data + AES_TAG_LEN;


   int temp_cipherlen = 0;

   DEBUG_SHMEM("Segment_count %d, data = %d, max_thread_no %d\n", segment_count, data, thread_no);

   //unsigned char *key = &(gcm_key[0]);

  // if (segment_count == 1){
  //    return shmemx_decrypt_single_buffer(cipherbuf, src, rbuf, dest, bytes, cipher_len);
  // }

  // DEBUG_SHMEM("Segment_count %d, data = %d, max_thread_no %d\n", segment_count, data, thread_no);
   
//   DEBUG_SHMEM("[START_DECRYPTION] Ciphertext: %s\n", cipherbuf);
//private (segment_count, count, local_cipherlen, cipherbuf, rbuf, openmp_dec_ctx, stdout, stderr, max_data, bytes, data, position, src, dest,  proc, res, key, cipher_len, temp_cipherlen)
//int position = 0;
#pragma omp parallel for schedule(dynamic) default(none) private(count, max_data, position, res, local_cipherlen, enc_data) shared(segment_count, stdout, stderr, openmp_dec_ctx, data, cipherbuf, rbuf, cipher_len, src, dest, bytes, proc, gcm_key) num_threads(thread_no)
   for (count = 0; count < segment_count ; count++){

      int tn = omp_get_thread_num();
 //     DEBUG_SHMEM("[T_%d] start\n", tn);
      //int cipher_temp = 0;
      position = count * (data+AES_TAG_LEN+AES_RAND_BYTES);
      void *tmp_buf = cipherbuf + position;
      void *tmp_buf2 = rbuf + (count * data);
      enc_data = data;
      max_data = data + AES_TAG_LEN;

      EVP_CIPHER_CTX *local_ctx = openmp_dec_ctx[tn];
      
      if ((count == segment_count - 1)){
         enc_data = (bytes - data*(segment_count - 1));
         max_data = enc_data + AES_TAG_LEN;
      }

      DEBUG_SHMEM("T_%d Params: ctx %p, rbuf+(%d): %p, cipher_len ptr %p, cipherbuf %p + src %d + RAND BYTES %d, bytes %d - AES_RAND_BYTES %d\n", tn, local_ctx, dest, (rbuf+dest), (&cipher_len), cipherbuf, src, AES_RAND_BYTES, enc_data, AES_RAND_BYTES);

      if ((res = EVP_DecryptInit_ex(local_ctx, NULL, NULL, NULL, tmp_buf)) != 1){
         ERROR_SHMEM("[T_%d] DecryptInit_ex failed: %lu %s\n",tn, ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
         memset(NULL, 0, 10);
      }


      if ((res = EVP_DecryptUpdate(local_ctx, ((unsigned char *)(tmp_buf2+dest)), (int *)(&local_cipherlen), tmp_buf+AES_RAND_BYTES+src, (max_data-AES_TAG_LEN))) != 1){
      ERROR_SHMEM("[T_%d] DecryptUpdate failed: %lu %s\n", tn, ERR_get_error(), ERR_error_string(res, NULL));
      memset(NULL, 0, 10);
   }

   DEBUG_SHMEM("T_%d DecryptUpdated passed; cipherlen: %u\n", tn, cipher_len);


    if ((res = EVP_CIPHER_CTX_ctrl(local_ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_LEN, tmp_buf+src+enc_data-AES_TAG_LEN))!= 1){
       ERROR_SHMEM("[T_%d] CIPHER_CTX_ctrl failed: %lu %s\n", tn, ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
       memset(NULL, 0, 10);

    }
 
    DEBUG_SHMEM("T_%d CTX_ctrl passed \n", tn);
    if ((res = EVP_DecryptFinal_ex(local_ctx, (tmp_buf2+dest+local_cipherlen), (int *)( &local_cipherlen))) != 1){
        /*handleErrors*/
       ERROR_SHMEM("[T_%d] Decryption Tag Verification Failed %lu %s\n", tn, ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
    }

    DEBUG_SHMEM("T_%d DecryptFinal_ex passed\n", tn);

   }

   DEBUG_SHMEM("[END_DECRYPTION] plaintext: %s\n", (char *)rbuf);


   return 0;
}

int shmemx_decrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, 
        void *rbuf, unsigned long long dest, size_t bytes, size_t cipher_len){



   int res = 0;

   DEBUG_SHMEM("cipherbuf %p, src %llu, rbuf %p, dest %llu, bytes %lu\n",
         cipherbuf, src, (void *)rbuf, dest, bytes);


   if ((res = EVP_DecryptInit_ex(defcp->dec_ctx, EVP_aes_256_gcm(), NULL, gcm_key, cipherbuf+src)) != 1){
      ERROR_SHMEM("DecryptInit_ex failed: %lu %s\n", ERR_get_error(), ERR_error_string(res, NULL));
      memset(NULL, 0, 10);
   }

 //  DEBUG_SHMEM("DecryptInit_ex passed \n");
   DEBUG_SHMEM("Params: ctx %p, rbuf+(%llu): %p, cipher_len ptr %p, cipherbuf %p + src %llu + RAND BYTES %d, bytes %lu - AES_RAND_BYTES %d\n", defcp->dec_ctx, dest, (rbuf+dest), (&cipher_len), cipherbuf, src, AES_RAND_BYTES, bytes, AES_RAND_BYTES);

   if ((res = EVP_DecryptUpdate(defcp->dec_ctx, ((unsigned char *)(rbuf+dest)), (int *)(&cipher_len), cipherbuf+src+AES_RAND_BYTES, (bytes-AES_RAND_BYTES))) != 1){
      ERROR_SHMEM("DecryptUpdate failed: %lu %s\n", ERR_get_error(), ERR_error_string(res, NULL));
      memset(NULL, 0, 10);
   }

   DEBUG_SHMEM("DecryptUpdated passed; cipherlen: %lu\n", cipher_len);


    if ((res = EVP_CIPHER_CTX_ctrl(defcp->dec_ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_LEN, (cipherbuf+dest+(bytes))))!= 1){
       ERROR_SHMEM("CIPHER_CTX_ctrl failed: %lu %s\n", ERR_get_error(), ERR_error_string(res, NULL));
       memset(NULL, 0, 10);

    }
 
    int temp_len = cipher_len;

    DEBUG_SHMEM("CTX_ctrl passed \n");
    if ((res = EVP_DecryptFinal_ex(defcp->dec_ctx, (rbuf+dest+cipher_len), (int *)( &cipher_len))) != 1){
        /*handleErrors*/
       ERROR_SHMEM("Decryption Tag Verification Failed\n");
    }

    DEBUG_SHMEM("DecryptFinal_ex passed\n");
    return res != 0 ? 0 : 0;
}


void shmemx_secure_put_nbi(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

    size_t cipherlen = 0;
    memset(nbi_put_ciphertext[nbput_count], 0, sizeof(char) * nbytes + AES_TAG_LEN+AES_RAND_BYTES);
    shmemx_encrypt_single_buffer_omp(
            ((unsigned char *)(&(nbi_put_ciphertext[nbput_count][0]))),
            0, src, 0, nbytes, ((size_t *)(&cipherlen)));

    DEBUG_SHMEM("Encryption successful\n");
    shmemc_ctx_put_nbi(ctx, dest, 
            ((nbi_put_ciphertext[nbput_count])),
            nbytes + AES_TAG_LEN + AES_RAND_BYTES, pe);

    DEBUG_SHMEM("Non-blocking_put successful\n");

    shmemc_context_h ch = (shmemc_context_h) ctx; 
    uint64_t r_dest;  /* address on other PE */
    ucp_rkey_h r_key; /* rkey for remote address */
    get_remote_key_and_addr(ch, (uint64_t)dest, pe, &r_key, &r_dest);

    uint64_t local_dest;
    ucp_rkey_h local_rkey;

    get_remote_key_and_addr(ch, (uint64_t) src, proc.li.rank, &local_rkey, &local_dest);


    nb_put_ctr[nbput_count].src_pe = 0; //proc.li.rank;
    nb_put_ctr[nbput_count].dst_pe = 0;
    nb_put_ctr[nbput_count].res_pe = pe;
    nb_put_ctr[nbput_count].plaintext_size = nbytes + AES_RAND_BYTES;
    nb_put_ctr[nbput_count].encrypted_size = cipherlen;
    nb_put_ctr[nbput_count].remote_buf_addr = r_dest;
    nb_put_ctr[nbput_count].local_buf_addr = (uintptr_t) src;
    nb_put_ctr[nbput_count].local_buf = (uintptr_t) src;
    

    nbput_count++;

    

    /* TODO: SIGNAL DECRYPTION IN WAIT!! */
}

void shmemx_secure_put(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

   size_t cipherlen = 0;

   double total_t1, total_t2,
          enc_t1, enc_t2,
          put_t1, put_t2,
          pmix_t1, pmix_t2; /* These last 2 would be for decrypting/encrypting */

   int thread_no = 1;
   if (nbytes < SIX_FOUR_K){
      thread_no = 1;
   }else if (nbytes < ONE_TWO_EIGHT_K){
      thread_no = 2;
   }else if (nbytes < TWO_FIVE_SIX_K){
      thread_no = 4;
   }else if (nbytes < FIVE_TWELVE_K){
      thread_no = 8;
   }else{
      thread_no = 16;
   }

   memset(blocking_put_ciphertext, 0, nbytes+AES_TAG_LEN+AES_RAND_BYTES+1);
    //int res  = 0;
//    unsigned char *blocking_put_ciphertext = malloc( nbytes+(AES_TAG_LEN+AES_RAND_BYTES));
    total_t1 = shmemx_wtime();

    enc_t1 = shmemx_wtime();
 //   shmemx_encrypt_single_buffer(
 //           blocking_put_ciphertext,
 //           0, src, 0, nbytes, ((size_t *)(&block_put_cipherlen)));
 //   enc_t2 = (shmemx_wtime() - enc_t1)*1e6;

   DEBUG_SHMEM("bytes: %lu\n", nbytes);
    int segment_count = shmemx_encrypt_single_buffer_omp(
          &(blocking_put_ciphertext[0]),
          0, src, 0, nbytes, ((size_t *)(&block_put_cipherlen)));
    
    DEBUG_SHMEM( "Encryption end, ciphertext: %p, cipherlen: %d \n",
          &(blocking_put_ciphertext[0]), block_put_cipherlen);

    put_t1 = shmemx_wtime();
    shmemc_ctx_put(ctx, dest, 
            blocking_put_ciphertext,
            block_put_cipherlen+(AES_TAG_LEN+AES_RAND_BYTES) /*nbytes+AES_TAG_LEN+AES_RAND_BYTES*/, 
            pe);
    put_t2 = (shmemx_wtime() - put_t1)*1e6;
    DEBUG_SHMEM( "Put end\n");

//    free(blocking_put_ciphertext);

    /* Only begin PMIX Construction AFTER the put has been set up appropriately
     * */

    /* Step 1, Get PMIX_Rank from the other peer proc.
     * Step 2: Set up notification in custom range 
     * Step 3: Profit?
     */

    pmix_t1 = shmemx_wtime();
    shmemc_context_h ch = (shmemc_context_h) ctx; 
    uint64_t r_dest;  /* address on other PE */
    ucp_rkey_h r_key; /* rkey for remote address */
    DEBUG_SHMEM("Getting rkey and addr\n");
    get_remote_key_and_addr(ch, (uint64_t)dest, pe, &r_key, &r_dest);


    pmix_status_t ps;
    pmix_info_t si[7];


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
    DEBUG_SHMEM("dest ptr, one passed in 0x%lx: %p\n", r_dest, dest);

    PMIX_INFO_CONSTRUCT(&si[1]);
    PMIX_LOAD_KEY(si[1].key, "Remote_secure_buffer");
    si[1].value.type = PMIX_UINT64;
    si[1].value.data.uint64 = (uint64_t)r_dest;
   // PMIX_INFO_LOAD(&si, PMIX_GRANK, &success, PMIX_INT);
    PMIX_INFO_CONSTRUCT(&si[2]);
    PMIX_LOAD_KEY(si[2].key, "Remote_buffer_enc_size");
    si[2].value.type = PMIX_INT;
    si[2].value.data.integer = block_put_cipherlen;  // +AES_TAG_LEN+AES_RAND_BYTES;

    PMIX_INFO_CONSTRUCT(&si[3]);
    PMIX_LOAD_KEY(si[3].key, "Destination_rank");
    si[3].value.type = PMIX_UINT32;
    si[3].value.data.uint32 = 0;


    PMIX_INFO_CONSTRUCT(&si[4]);
    PMIX_LOAD_KEY(si[4].key, "Source_rank");
    si[4].value.type = PMIX_UINT32;
    si[4].value.data.uint32 = 0;


    PMIX_INFO_CONSTRUCT(&si[5]);
    PMIX_LOAD_KEY(si[5].key, "is_nonblocking");
    si[5].value.type = PMIX_INT;
    si[5].value.data.integer = 0;

    PMIX_INFO_CONSTRUCT(&si[6]);
    PMIX_LOAD_KEY(si[5].key, "og_bytes");
    si[6].value.type = PMIX_UINT32;
    si[6].value.data.uint32 = nbytes+AES_RAND_BYTES;
    double pmix_construct_time = (shmemx_wtime()-pmix_t1)*1e6;



    DEBUG_SHMEM( "Starting signaling\n");
    pmix_t1 = shmemx_wtime();
    ps = PMIx_Notify_event(DEC_SUCCESS, procs, PMIX_RANGE_CUSTOM, &(si[0]),
            7, NULL, NULL);
    pmix_t2 = (shmemx_wtime()-pmix_t1)*1e6;
//    PMIx_Fence(NULL, 0, NULL, 0);

    if (ps != PMIX_SUCCESS){
        shmemu_assert(ps == PMIX_SUCCESS,
                " shmem_ctx_secure_put: PMIx can't notify decryption: %s",
                PMIx_Error_string(ps));
    }

//    DEBUG_SHMEM( "Signaling success? %s\n", ps == PMIX_SUCCESS ? "yes" : "no");

    PMIX_DATA_ARRAY_DESTRUCT(&pmix_darray);
    //free(blocking_put_ciphertext);
    total_t2 = (shmemx_wtime()-total_t1)*1e6;

 /*  DEBUG_SHMEM("For %u bytes\n"
                "Encrypt_time %.3f\nPut_time: %.3f\npmix_construct: %.3f\n"
                "PMIX_notify_time: %.3f\nTotal_time: %.3f\n",
                nbytes, enc_t2, put_t2, pmix_construct_time,
                pmix_t2, total_t2);*/
}



void shmemx_secure_get_nbi(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

    nbi_get_ciphertext[nbget_count] = malloc(sizeof(char) * nbytes+AES_TAG_LEN+AES_RAND_BYTES);

    size_t cipherlen = 0;

     
    shmemc_context_h ch = (shmemc_context_h) ctx; 
    uint64_t r_dest;  /* address on other PE */
    ucp_rkey_h r_key; /* rkey for remote address */
    DEBUG_SHMEM("Getting rkey and addr\n");
    get_remote_key_and_addr(ch, (uint64_t)src, pe, &r_key, &r_dest);


    pmix_status_t ps;
    pmix_info_t si[7];

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
    DEBUG_SHMEM("dest ptr: %p\n", (void *)r_dest);

    PMIX_INFO_CONSTRUCT(&si[1]);
    PMIX_LOAD_KEY(si[1].key, "Remote_secure_buffer");
    si[1].value.type = PMIX_UINT64;
    si[1].value.data.uint64 = (uint64_t)r_dest;
//    PMIX_INFO_LOAD(&si, PMIX_GRANK, &success, PMIX_INT);
    PMIX_INFO_CONSTRUCT(&si[2]);
    PMIX_LOAD_KEY(si[2].key, "Remote_buffer_enc_size");
    si[2].value.type = PMIX_INT;
    si[2].value.data.integer = 0;

    PMIX_INFO_CONSTRUCT(&si[3]);
    PMIX_LOAD_KEY(si[3].key, "Destination_rank");
    si[3].value.type = PMIX_UINT32;
    si[3].value.data.uint32 = 0;

    PMIX_INFO_CONSTRUCT(&si[4]);
    PMIX_LOAD_KEY(si[4].key, "Source_rank");
    si[4].value.type = PMIX_UINT32;
    si[4].value.data.uint32 = 0;

    PMIX_INFO_CONSTRUCT(&si[5]);
    PMIX_LOAD_KEY(si[5].key, "is_nonblocking");
    si[5].value.type = PMIX_INT;
    si[5].value.data.integer = 1;

    PMIX_INFO_CONSTRUCT(&si[6]);
    PMIX_LOAD_KEY(si[6].key, "og_bytes");
    si[6].value.type = PMIX_UINT32;
    si[6].value.data.uint32 = nbytes;


      DEBUG_SHMEM("Starting signaling\n");
    ps = PMIx_Notify_event(ENC_SUCCESS, procs, PMIX_RANGE_CUSTOM, &(si[0]),
            7, NULL, NULL);

    if (ps != PMIX_SUCCESS){
        shmemu_assert(ps == PMIX_SUCCESS,
                " shmem_ctx_secure_get_nbi: PMIx can't notify decryption: %s",
                PMIx_Error_string(ps));
    };

    DEBUG_SHMEM( "Encryption Signaling success? %s\n", ps == PMIX_SUCCESS ? "yes" : "no");

    PMIX_DATA_ARRAY_DESTRUCT(&pmix_darray);
    //free(procs);

    procs = NULL;
   // nprocs = 2;
   // pmix_proc_t fence_proc[PROC_ENC_DEC_FENCE_COUNT];
    
 //   PMIX_LOAD_PROCID(&(fence_proc[0]), my_second_pmix->nspace, proc.li.rank);
 //   PMIX_LOAD_PROCID(&(fence_proc[1]), my_second_pmix->nspace, pe);

   // PMIx_Fence(fence_proc, PROC_ENC_DEC_FENCE_COUNT, NULL, 0);    

    
    shmemc_ctx_get_nbi(ctx, 
            ((unsigned char *)nbi_get_ciphertext[nbget_count]),
            src,
            nbytes+AES_TAG_LEN+AES_RAND_BYTES, pe);

    nb_get_ctr[nbget_count].src_pe = si[4].value.data.uint32;
    nb_get_ctr[nbget_count].dst_pe = si[3].value.data.uint32;
    nb_get_ctr[nbget_count].res_pe = pe;
    nb_get_ctr[nbget_count].plaintext_size = nbytes;
    nb_get_ctr[nbget_count].encrypted_size = nbytes+AES_TAG_LEN+AES_RAND_BYTES;
    nb_get_ctr[nbget_count].local_buf_addr = (uintptr_t) dest;
    nb_get_ctr[nbget_count].local_buf = (uintptr_t) dest;
    nbget_count++;

    int res_bytes = nbytes+AES_RAND_BYTES;
    PMIX_INFO_CONSTRUCT(&si[2]);
    PMIX_LOAD_KEY(si[2].key, "Remote_buffer_enc_size");
    si[2].value.type = PMIX_INT;
    si[2].value.data.integer = nbytes; 

    ps = PMIx_Notify_event(DEC_SUCCESS, procs, PMIX_RANGE_CUSTOM, &(si[0]),
          7, notif_cb_callback, NULL);

    //pmix_proc_t fence_proc[PROC_ENC_DEC_FENCE_COUNT];
    
   // PMIX_LOAD_PROCID(&(fence_proc[0]), my_second_pmix->nspace, proc.li.rank);
   // PMIX_LOAD_PROCID(&(fence_proc[1]), my_second_pmix->nspace, pe);

    //PMIx_Fence(fence_proc, PROC_ENC_DEC_FENCE_COUNT, NULL, 0);    

    free(procs);
}


int shmemx_secure_quiet(void){

   //int shmem_errno = 0;
   pmix_status_t ps;
   pmix_info_t si[7]; 
   pmix_proc_t put_proc = {};
   int nprocs = 1;

   pmix_data_array_t pmix_darray;
   pmix_darray.size = nprocs;
   pmix_darray.type = PMIX_PROC;
   PMIX_DATA_ARRAY_CONSTRUCT(&pmix_darray, nprocs, PMIX_PROC);
   /* Need the put-based PMIx array for the remote cleanup.
    * As long as it's not super-time consuming, then we're
    * okay... I think
    */

//   DEBUG_SHMEM("nbput_count: %llu, nbget_count: %llu\n",
//         nbput_count, nbget_count);

   int ctr = 0;
   pmix_proc_t fence_proc[PROC_ENC_DEC_FENCE_COUNT];

   while (ctr < nbput_count){
      shmem_secure_attr_t put_data = nb_put_ctr[ctr];
      PMIX_LOAD_PROCID(&put_proc, my_second_pmix->nspace, put_data.res_pe);

      memcpy(pmix_darray.array, &put_proc, sizeof(pmix_proc_t));

      PMIX_INFO_CONSTRUCT(&si[0]);
      PMIX_INFO_LOAD(&si[0], PMIX_EVENT_CUSTOM_RANGE, &pmix_darray, PMIX_DATA_ARRAY);

      PMIX_INFO_CONSTRUCT(&si[1]);
      PMIX_LOAD_KEY(si[1].key, "Remote_secure_buffer");
      si[1].value.type = PMIX_UINT64;
      si[1].value.data.uint64 = (uint64_t)put_data.remote_buf_addr;
      // PMIX_INFO_LOAD(&si, PMIX_GRANK, &success, PMIX_INT);
      PMIX_INFO_CONSTRUCT(&si[2]);
      PMIX_LOAD_KEY(si[2].key, "Remote_buffer_enc_size");
      si[2].value.type = PMIX_INT;
      si[2].value.data.integer = put_data.encrypted_size;

      PMIX_INFO_CONSTRUCT(&si[3]);
      PMIX_LOAD_KEY(si[3].key, "Destination_rank");
      si[3].value.type = PMIX_UINT32;
      si[3].value.data.uint32 = put_data.dst_pe;

      PMIX_INFO_CONSTRUCT(&si[4]);
      PMIX_LOAD_KEY(si[4].key, "Source_rank");
      si[4].value.type = PMIX_UINT32;
      si[4].value.data.uint32 = put_data.src_pe;

      PMIX_INFO_CONSTRUCT(&si[5]);
      PMIX_LOAD_KEY(si[5].key, "is_nonblocking");
      si[5].value.type = PMIX_INT;
      si[5].value.data.integer = 1;

      PMIX_INFO_CONSTRUCT(&si[6]);
      PMIX_LOAD_KEY(si[6].key, "og_bytes");
      si[6].value.type = PMIX_UINT32;
      si[6].value.data.uint32 = put_data.plaintext_size+AES_RAND_BYTES;


      DEBUG_SHMEM( "Starting signaling with r_dest %p, og_bytes %lu, dest_rank %u \n",
              (void *)put_data.remote_buf_addr, put_data.plaintext_size, put_data.res_pe);
      ps = PMIx_Notify_event(DEC_SUCCESS, &put_proc, PMIX_RANGE_CUSTOM, &(si[0]),
            7, //notif_cb_callback 
             NULL, NULL);


//      PMIX_LOAD_PROCID(&(fence_proc[0]), my_second_pmix->nspace, proc.li.rank);
//      PMIX_LOAD_PROCID(&(fence_proc[1]), my_second_pmix->nspace, put_data.res_pe);

      //PMIx_Fence(fence_proc, PROC_ENC_DEC_FENCE_COUNT, NULL, 0);    


      if (ps != PMIX_SUCCESS){
         shmemu_assert(ps == PMIX_SUCCESS,
               " shmem_ctx_secure_put_nbi quiet: PMIx can't notify decryption: %s",
               PMIx_Error_string(ps));
      };

     // free(nbi_put_ciphertext[ctr]);
      ctr++;
   }

   ctr = 0;
   nbput_count = 0;

   while (ctr < nbget_count){
      shmem_secure_attr_t get_data = nb_get_ctr[ctr];
      

      DEBUG_SHMEM("Local_decryption\n");
      if (shmemx_decrypt_single_buffer_omp(nbi_get_ciphertext[ctr], get_data.src_pe, 
               (void *)(get_data.local_buf),get_data.dst_pe, get_data.plaintext_size,
               get_data.encrypted_size) != 0){
         ERROR_SHMEM("Failed to decrypt on buffer %p with ciphertext %p, counter %d\n",
               (void *)get_data.local_buf, nbi_get_ciphertext[ctr], ctr);
         memset(NULL, 0, 10);
      }

      PMIX_LOAD_PROCID(&put_proc, my_second_pmix->nspace, get_data.res_pe);

      memcpy(pmix_darray.array, &put_proc, sizeof(pmix_proc_t));

      PMIX_INFO_CONSTRUCT(&si[0]);
      PMIX_INFO_LOAD(&si[0], PMIX_EVENT_CUSTOM_RANGE, &pmix_darray, PMIX_DATA_ARRAY);

      PMIX_INFO_CONSTRUCT(&si[1]);
      PMIX_LOAD_KEY(si[1].key, "Remote_secure_buffer");
      si[1].value.type = PMIX_UINT64;
      si[1].value.data.uint64 = (uint64_t)get_data.remote_buf_addr;
      // PMIX_INFO_LOAD(&si, PMIX_GRANK, &success, PMIX_INT);
      PMIX_INFO_CONSTRUCT(&si[2]);
      PMIX_LOAD_KEY(si[2].key, "Remote_buffer_enc_size");
      si[2].value.type = PMIX_INT;
      si[2].value.data.integer = get_data.encrypted_size;

      PMIX_INFO_CONSTRUCT(&si[3]);
      PMIX_LOAD_KEY(si[3].key, "Destination_rank");
      si[3].value.type = PMIX_UINT32;
      si[3].value.data.uint32 = get_data.dst_pe;

      PMIX_INFO_CONSTRUCT(&si[4]);
      PMIX_LOAD_KEY(si[4].key, "Source_rank");
      si[4].value.type = PMIX_UINT32;
      si[4].value.data.uint32 = get_data.src_pe;

      PMIX_INFO_CONSTRUCT(&si[5]);
      PMIX_LOAD_KEY(si[5].key, "is_nonblocking");
      si[5].value.type = PMIX_INT;
      si[5].value.data.integer = 1;

      PMIX_INFO_CONSTRUCT(&si[6]);
      PMIX_LOAD_KEY(si[6].key, "og_bytes");
      si[6].value.type = PMIX_UINT32;
      si[6].value.data.uint32 = get_data.plaintext_size;

    //  DEBUG_SHMEM( "Starting signaling with r_dest %p, og_bytes %lu, dest_rank %u \n",
    //        (void *)get_data.remote_buf_addr, get_data.plaintext_size, get_data.res_pe);
    //  ps = PMIx_Notify_event(DEC_SUCCESS, &put_proc, PMIX_RANGE_CUSTOM, &(si[0]),
    //        7, notif_cb_callback , NULL);

    free(nbi_get_ciphertext[ctr]);
/*      PMIX_LOAD_PROCID(&(fence_proc[0]), my_second_pmix->nspace, proc.li.rank);
      PMIX_LOAD_PROCID(&(fence_proc[1]), my_second_pmix->nspace, ge_data.res_pe);

      PMIx_Fence(fence_proc, PROC_ENC_DEC_FENCE_COUNT, NULL, 0);    */




      /* Need to perform the decryption on the remote side HERE */

      nbi_get_ciphertext[ctr] = NULL;
      ctr++;
   }
   nbget_count = 0;

   memset(nb_put_ctr, 0, (sizeof(shmem_secure_attr_t) * NON_BLOCKING_OP_COUNT*2));
   memset(nb_get_ctr, 0, (sizeof(shmem_secure_attr_t) * NON_BLOCKING_OP_COUNT*2));


   return 0;
}


void shmemx_secure_get(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){
    unsigned char *blocking_get_ciphertext = malloc( nbytes+(AES_TAG_LEN+AES_RAND_BYTES));

    size_t cipherlen = 0;
    double total_t1, total_t2,
           dec_t1, dec_t2,
           get_t1, get_t2,
           pmix_t1, pmix_t2; /* These last 2 would be for decrypting/encrypting */

    double pmix_construct_time = 0;
    total_t1 = shmemx_wtime();
    pmix_t1 = shmemx_wtime();
    shmemc_context_h ch = (shmemc_context_h) ctx; 
    uint64_t r_dest;  /* address on other PE */
    ucp_rkey_h r_key; /* rkey for remote address */
    get_remote_key_and_addr(ch, (uint64_t)src, pe, &r_key, &r_dest);


    pmix_status_t ps;
    pmix_info_t si[7];

    pmix_proc_t *procs;
    size_t nprocs = 1;
    procs = (pmix_proc_t *)malloc(sizeof(pmix_proc_t)*nprocs);
    DEBUG_SHMEM("Entering PMIX_LOAD_PROC_ID\n");
    PMIX_LOAD_PROCID(procs, my_second_pmix->nspace, pe);

    pmix_data_array_t pmix_darray;
    pmix_darray.size = nprocs;
    pmix_darray.type = PMIX_PROC;
    PMIX_DATA_ARRAY_CONSTRUCT(&pmix_darray, nprocs, PMIX_PROC);
    memcpy(pmix_darray.array, procs, nprocs*sizeof(pmix_proc_t));

   
    PMIX_INFO_CONSTRUCT(&si[0]);
    PMIX_INFO_LOAD(&si[0], PMIX_EVENT_CUSTOM_RANGE, &pmix_darray, PMIX_DATA_ARRAY);
    DEBUG_SHMEM("dest ptr: %p\n", (void *)r_dest);

    PMIX_INFO_CONSTRUCT(&si[1]);
    PMIX_LOAD_KEY(si[1].key, "Remote_secure_buffer");
    si[1].value.type = PMIX_UINT64;
    si[1].value.data.uint64 = (uint64_t)r_dest;

    PMIX_INFO_CONSTRUCT(&si[2]);
    PMIX_LOAD_KEY(si[2].key, "Remote_buffer_enc_size");
    si[2].value.type = PMIX_INT;
    si[2].value.data.integer = nbytes+AES_TAG_LEN+AES_RAND_BYTES; //block_get_cipherlen;

    PMIX_INFO_CONSTRUCT(&si[3]);
    PMIX_LOAD_KEY(si[3].key, "Destination_rank");
    si[3].value.type = PMIX_UINT32;
    si[3].value.data.uint32 = 0;

    PMIX_INFO_CONSTRUCT(&si[4]);
    PMIX_LOAD_KEY(si[4].key, "Source_rank");
    si[4].value.type = PMIX_UINT32;
    si[4].value.data.uint32 = 0;


    PMIX_INFO_CONSTRUCT(&si[5]);
    PMIX_LOAD_KEY(si[5].key, "is_nonblocking");
    si[5].value.type = PMIX_INT;
    si[5].value.data.integer = 0;

    PMIX_INFO_CONSTRUCT(&si[4]);
    PMIX_LOAD_KEY(si[4].key, "og_bytes");
    si[6].value.type = PMIX_UINT32;
    si[6].value.data.uint32 = nbytes;
    pmix_construct_time = (shmemx_wtime()-pmix_t1)*1e6;


      DEBUG_SHMEM("Starting signaling\n");
      pmix_t1 = shmemx_wtime();
    ps = PMIx_Notify_event(ENC_SUCCESS, procs, PMIX_RANGE_CUSTOM, &(si[0]),
            7, NULL, NULL);
      pmix_t2 = (shmemx_wtime()-pmix_t1)*1e6;

    if (ps != PMIX_SUCCESS){
        shmemu_assert(ps == PMIX_SUCCESS,
                " shmem_ctx_secure_get: PMIx can't notify decryption: %s",
                PMIx_Error_string(ps));
    };

//    DEBUG_SHMEM( "Encryption Signaling success? %s\n", ps == PMIX_SUCCESS ? "yes" : "no");

    PMIX_DATA_ARRAY_DESTRUCT(&pmix_darray);
    free(procs);
    
    pmix_proc_t fence_proc[PROC_ENC_DEC_FENCE_COUNT];
    
//    PMIX_LOAD_PROCID(&(fence_proc[0]), my_second_pmix->nspace, proc.li.rank);
//    PMIX_LOAD_PROCID(&(fence_proc[1]), my_second_pmix->nspace, pe);

    //PMIx_Fence(fence_proc, PROC_ENC_DEC_FENCE_COUNT, NULL, 0);    
/* Technically, this SHOULD be feasible after a notify? */
    //free(procs);


    get_t1 = shmemx_wtime();
    shmemc_ctx_get(ctx, 
            (blocking_get_ciphertext),
            src,
            nbytes+AES_TAG_LEN+AES_RAND_BYTES, 
            pe);
    get_t2 = (shmemx_wtime()-get_t1)*1e6;

    DEBUG_SHMEM("Get passed\n");

    dec_t1 = shmemx_wtime();

    int res_bytes = nbytes+AES_RAND_BYTES;
    DEBUG_SHMEM("first decryption \n");
    shmemx_decrypt_single_buffer_omp(blocking_get_ciphertext, 0, dest, 0, res_bytes,  (size_t)res_bytes);
    dec_t2 = (shmemx_wtime()-dec_t1)*1e6;

   DEBUG_SHMEM("Second Decryption passing after first one. Need to talk to remote proc to do it there, too\n");

    PMIX_INFO_CONSTRUCT(&si[6]);
    PMIX_LOAD_KEY(si[6].key, "og_bytes");
    si[6].value.type = PMIX_INT;
    si[6].value.data.integer = res_bytes; 
    double dec2_t1 = shmemx_wtime();
    ps = PMIx_Notify_event(DEC_SUCCESS, procs, PMIX_RANGE_CUSTOM, &(si[0]),
          7, notif_cb_callback, NULL);
    double dec2_t2 = (shmemx_wtime()-dec2_t1)*1e6;
    if (ps != PMIX_SUCCESS){
       shmemu_assert(ps == PMIX_SUCCESS,
             " shmem_ctx_secure_get: PMIx can't notify decryption: %s",
             PMIx_Error_string(ps));
    };

  //  free (procs);

    //pmix_proc_t fence_proc[PROC_ENC_DEC_FENCE_COUNT];
    
    //PMIX_LOAD_PROCID(&(fence_proc[0]), my_second_pmix->nspace, proc.li.rank);
    //PMIX_LOAD_PROCID(&(fence_proc[1]), my_second_pmix->nspace, pe);

    //PMIx_Fence(fence_proc, PROC_ENC_DEC_FENCE_COUNT, NULL, 0);    

    
    free(blocking_get_ciphertext);
    total_t2 = (shmemx_wtime()-total_t1)*1e6;

/*      DEBUG_SHMEM("For %u bytes\n"
                "Decrypt_time %.3f\nGet_time: %.3f\npmix_construct: %.3f\n"
                "PMIX_notify_time: %.3f\n2nd_dec: %.3f\nTotal_time: %.3f\n",
                nbytes, dec_t2, get_t2, pmix_construct_time,
                pmix_t2, dec2_t2, total_t2); */   
}

#endif /* ENABLE_SHMEM_ENCRYPTION */
