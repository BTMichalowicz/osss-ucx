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
#include <omp.h>

#define use_ctr 1
#define use_gcm 0

const unsigned char gcm_key[GCM_KEY_SIZE] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'c', 'd',
    'f', 'e', 'a', 'c', 'b', 'd', 'e', 'f', '0', '1', '2',
    '3', '4', '5', '6', '7', '8', '9', 'a', 'd', 'c'};

#if use_ctr
unsigned char IV[AES_TAG_LEN] = {'L','F','g','l','j','2','M','T','M','z',
                                 'i','s','n','O','g','p'};
#endif /* use_ctr */


unsigned char blocking_put_ciphertext[MAX_MSG_SIZE + OFFSET] = {'\0'};
unsigned char nbi_put_ciphertext[NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE + COLL_OFFSET];
unsigned long long nbput_count = 0;

unsigned char blocking_get_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char **nbi_get_ciphertext =
    NULL; //[NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbget_count = 0;

shmem_secure_attr_t *nb_put_ctr = NULL;
shmem_secure_attr_t *nb_get_ctr = NULL;

EVP_CIPHER_CTX
    *openmp_enc_ctx[MAX_THREAD_COUNT]; /* Can I get around the AEAD limitation
                                          from BoringSSL? */
EVP_CIPHER_CTX
    *openmp_dec_ctx[MAX_THREAD_COUNT]; /* Answer to the above comment? Yes */


unsigned char enc_chunks[MAX_THREAD_COUNT][MAX_MSG_SIZE+COLL_OFFSET];
unsigned char dec_chunks[MAX_THREAD_COUNT][MAX_MSG_SIZE+COLL_OFFSET];
unsigned char plain_chunks[MAX_THREAD_COUNT][MAX_MSG_SIZE+COLL_OFFSET];

int block_put_cipherlen = 0;
/*
 * -- helpers ----------------------------------------------------------------
 */


inline static void noop_callbackx(void *req, ucs_status_t status, void *user_data) {
  NO_WARN_UNUSED(req);
  NO_WARN_UNUSED(status);
  NO_WARN_UNUSED(user_data);
}

/*
 * shortcut to look up the UCP endpoint of a context
 */
inline static ucp_ep_h lookup_ucp_ep(shmemc_context_h ch, int pe) {
  return ch->eps[pe];
}

static void am_send_cb(void *request, ucs_status_t status, void *user_data)
{
   NO_WARN_UNUSED(user_data);
   if (status != UCS_OK) {
      fprintf(stderr, "Active Message send failed: %s\n",
            ucs_status_string(status));
   }
   ucp_request_free(request);
}



#ifdef HAVE_UCP_REQUEST_CHECK_STATUS
#define UCX_REQUEST_CHECK(_request) ucp_request_check_status(_request)
#else
#define UCX_REQUEST_CHECK(_request) ucp_request_test(_request, NULL)
#endif /* HAVE_UCP_REQUEST_CHECK_STATUS */

inline static ucs_status_t check_wait_for_request(shmemc_context_h ch,
                                                  void *req) {
  DEBUG_SHMEM("Entered check_wait_for_request\n");
  if (req == NULL) { /* completed */
    DEBUG_SHMEM("Completed\n");
    return UCS_OK;
  } else if (UCS_PTR_IS_ERR(req)) {
    DEBUG_SHMEM("Canceling the request\n");
    ucp_request_cancel(ch->w, req);
    return UCS_PTR_STATUS(req);
  } else { /* wait for completion */
    ucs_status_t s;
    DEBUG_SHMEM("Waiting for completion\n");

    do {
      DEBUG_SHMEM("Entering request check with worker %p\n", ch->w);
      ucp_worker_progress(ch->w);

      s = UCX_REQUEST_CHECK(req);
    } while (s == UCS_INPROGRESS);
    ucp_request_free(req);

    return s;
  }
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

  shmemu_assert(r >= 0,
                "shmem_enc/dec, get_rkey/addr: can't find memory region for %p",
                (void *)local_addr);

  *rkey_p = lookup_rkey(ch, r, pe);
  *raddr_p = translate_region_address(local_addr, r, pe);
}

inline static int get_thread_count(size_t bytes) {
  int thread_no = 1;
  int ppn = proc.li.npeers;

  if (ppn < 4){
      if (bytes < THIRTY_TWO_K){
          thread_no = 1;
      }else if (bytes < SIX_FOUR_K) {
          thread_no = 2;
      } else if (bytes < ONE_TWO_EIGHT_K) {
          thread_no = 4;
      } else if (bytes < TWO_FIVE_SIX_K) {
          thread_no = 8;
      } else if (bytes < FIVE_TWELVE_K) {
          thread_no = 8;
      } else {
          thread_no = 8;
      }
  }else if (ppn < 8){
      if (bytes < THIRTY_TWO_K){
          thread_no = 1;
      }else if (bytes < SIX_FOUR_K) {
          thread_no = 2;
      } else if (bytes < ONE_TWO_EIGHT_K) {
          thread_no = 4;
      } else if (bytes < TWO_FIVE_SIX_K) {
          thread_no = 8;
      } else if (bytes < FIVE_TWELVE_K) {
          thread_no = 8;
      } else {
          thread_no = 8;
      }
  }else if (ppn < 16){
      if (bytes < THIRTY_TWO_K){
          thread_no = 1;
      }else if (bytes < SIX_FOUR_K) {
          thread_no = 2;
      } else if (bytes < ONE_TWO_EIGHT_K) {
          thread_no = 4;
      } else if (bytes < TWO_FIVE_SIX_K) {
          thread_no = 8;
      } else if (bytes < FIVE_TWELVE_K) {
          thread_no = 8;
      } else {
          thread_no = 8;
      }
  }else if (ppn < 32){
      if (bytes < THIRTY_TWO_K){
          thread_no = 1;
      }else if (bytes < SIX_FOUR_K) {
          thread_no = 2;
      } else if (bytes < ONE_TWO_EIGHT_K) {
          thread_no = 4;
      } else if (bytes < TWO_FIVE_SIX_K) {
          thread_no = 4;
      } else if (bytes < FIVE_TWELVE_K) {
          thread_no = 4;
      } else {
          thread_no = 4;
      }
  }else{
      if (bytes < THIRTY_TWO_K){
          thread_no = 1;
      }else if (bytes < SIX_FOUR_K) {
          thread_no = 2;
      } else if (bytes < ONE_TWO_EIGHT_K) {
          thread_no = 2;
      } else if (bytes < TWO_FIVE_SIX_K) {
          thread_no = 2;
      } else if (bytes < FIVE_TWELVE_K) {
          thread_no = 2;
      } else {
          thread_no = 2;
      }
  }



  return thread_no;
}

ucs_status_t put_dec_handler(void *arg, const void *header, size_t h_size,
                         void *data, size_t len,
                         const ucp_am_recv_param_t *param) {

  DEBUG_SHMEM("Entering put_dec_handler\n");
  //  NO_WARN_UNUSED(arg);
  NO_WARN_UNUSED(header);
  NO_WARN_UNUSED(h_size);

  func_args_t *func_data = (func_args_t *)data;
  uint64_t r_dest = func_data->remote_buffer;
#if use_ctr
  //memcpy(IV, func_data->IV, AES_TAG_LEN);
#endif /*use_ctr*/

  shmemu_assert(r_dest >= 0, "put_dec_handler: rdest is 0, can't find region of %p",
                (void *)r_dest);

  DEBUG_SHMEM("ciphertext: %p %s\n", r_dest, r_dest);
    shmemx_decrypt_single_buffer_omp(r_dest, 0, (void *)r_dest, 0,
                                     func_data->local_size,
                                     func_data->encrypted_size);
  return UCS_OK;
}



ucs_status_t get_enc_handler(void *arg, const void *header, size_t h_size,
                             void *data, size_t len,
                             const ucp_am_recv_param_t *param) {

  //int rank = proc.li.rank;
  func_args_t *func_data = (func_args_t *)data;
//  func_data->src_pe = proc.li.rank;
//  func_data->dst_pe = 
  uint64_t r_dest = (uint64_t)(func_data->remote_buffer);
  int offset_from_start = func_data->offset_from_start;
  int segment_count = func_data->segment_count;
  int remainder = func_data->remainder;

  //DEBUG_SHMEM("get enc handler with thread_count %d\n", thread_count);
  void *temp_buffer = malloc(func_data->local_size + (1*(AES_TAG_LEN+AES_RAND_BYTES)));
  void *temp_buffer_2 = malloc(func_data->local_size + 10);
  memcpy(temp_buffer_2, r_dest/*+offset_from_start*/, func_data->local_size);
  int segment_count_2 = 0;
  DEBUG_SHMEM("Get handler\n");

  DEBUG_SHMEM("use_gcm: %d, use_ctr: %d\n", use_gcm, use_ctr);
#if use_gcm
  DEBUG_SHMEM("ebcryption time for gcm\n");
    segment_count_2 = shmemx_encrypt_single_buffer_omp((unsigned char *)temp_buffer, 0, (void *)temp_buffer_2,
                                     0, func_data->local_size,
                                     ((size_t *)(&func_data->encrypted_size)));
#elif use_ctr
    DEBUG_SHMEM("encryption time for ctr\n");
    segment_count_2 = shmemx_encrypt_single_buffer_omp((unsigned char *)r_dest, 0, (void *)temp_buffer_2,
                                     0, func_data->local_size,
                                     ((size_t *)(&func_data->encrypted_size)));

#endif /* use_ctr ^ use_gcm */

    goto fn_exit;

  size_t res_size = func_data->encrypted_size ;//+ (segment_count_2 *  (AES_TAG_LEN + AES_RAND_BYTES));

  DEBUG_SHMEM("Res_size: %u\n", res_size);
//  memcpy(response->local_buffer, temp_buffer, res_size);

#if defined(HAVE_UCP_PUT_NBX) || defined(HAVE_UCP_PUT_NB)
  ucs_status_ptr_t sp;
#endif /* HAVE_UCP_PUT_NBX || HAVE_UCP_PUT_NB */
  ucs_status_t s;
  ucp_ep_h return_ep = lookup_ucp_ep(defcp, func_data->dst_pe);


#if 0
//  ucp_rkey_h rkey = func_data->put_rem_rkey;
  uint64_t caller_rdest = func_data->put_rem_buf;

  int pe = func_data->dst_pe;


//  ucp_request_param_t prm = {.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK,
//                                   .cb.send = noop_callbackx};

  DEBUG_SHMEM("Going back to the put based idea?\n");

  shmemc_ctx_put(defcp, caller_rdest, temp_buffer, func_data->encrypted_size, pe); 
//  sp = ucp_put_nbx(return_ep, temp_buffer, func_data->encrypted_size, caller_rdest, rkey, &prm);
//  s = check_wait_for_request(defcp, sp);
  shmemc_progress();
//  if (s != UCS_OK){
//     ERROR_SHMEM("Get-via-put failed. Going to fallback\n");
//     goto am_fallback;
//     //shmem_global_exit(s);
//  }

  free(temp_buffer);
  free(temp_buffer_2);
DEBUG_SHMEM("Setting up a callback to ensure that we decrypt appropriately...\n");
  func_args_t *response = (func_args_t *)malloc(sizeof(func_args_t)); 
  response->local_size = func_data->local_size;
  response->encrypted_size = func_data->encrypted_size;
  response->local_buf = func_data->local_buf;
  response->remote_buffer = func_data->local_buf;
  response->src_pe = func_data->src_pe;
  response->dst_pe = func_data->dst_pe;
  response->get_rem_buf = func_data->get_rem_buf;
  DEBUG_SHMEM("get_rem_buf: %p\n", func_data->get_rem_buf);
#if use_ctr
  memcpy(response->IV, IV, AES_TAG_LEN);
#endif
  response->encrypted_size = func_data->encrypted_size;

  ucp_request_param_t ack_param = {
      .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
      .cb.send = noop_callbackx,
      .datatype = ucp_dt_make_contig(sizeof(unsigned char))};


  sp =
     ucp_am_send_nbx(return_ep, AM_NBPUT_HANDLER, NULL, 0, response,
           sizeof(func_args_t), &ack_param);
  DEBUG_SHMEM("Progressing....\n");
  shmemc_progress();
  s = check_wait_for_request(defcp, sp);
  shmemu_assert(s == UCS_OK, "%s: failed (status: %s)", __func__,
        ucs_status_string(s));

  if (s != UCS_OK){
     ERROR_SHMEM("Get-via-put failed. Going to fallback\n");
     goto am_fallback;
     //shmem_global_exit(s);
  }

  free(response);
  goto fn_exit;


am_fallback:
#endif
  //free(response);
  func_args_t *response = (func_args_t *)malloc(sizeof(func_args_t)); // func_data->local_size+AES_TAG_LEN+AES_RAND_BYTES);
  response->local_size = func_data->local_size;
  response->local_buf = func_data->local_buf;
  DEBUG_SHMEM("response->local_buf: %p\n", response->local_buf);
  response->remote_buffer = func_data->remote_buffer;
  response->offset_from_start = offset_from_start;
  response->src_pe = func_data->src_pe;
  response->dst_pe = func_data->dst_pe;
  response->get_rem_buf = func_data->get_rem_buf;
  DEBUG_SHMEM("get_rem_buf: %p\n", func_data->get_rem_buf);
#if use_ctr
  //memcpy(response->IV, IV, AES_TAG_LEN);
#endif
  response->encrypted_size = func_data->encrypted_size;



  ucp_request_param_t ack_param2 = {
      .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
      .cb.send = noop_callbackx,
      .datatype = ucp_dt_make_contig(sizeof(unsigned char))};

  DEBUG_SHMEM("Heading to the remote decryption response handler!\n");
  sp =
      ucp_am_send_nbx(return_ep, AM_GET_DEC_RESPONSE, NULL, 0, response,
                      sizeof(func_args_t), &ack_param2);
   DEBUG_SHMEM("Progressing....\n");
 //shmemc_progress();
  s = check_wait_for_request(defcp, sp);
  shmemu_assert(s == UCS_OK, "%s: failed (status: %s)", __func__,
                ucs_status_string(s));

  s = check_wait_for_request(defcp, sp);
  shmemu_assert(s == UCS_OK, "%s: failed (status: %s)", __func__,
                ucs_status_string(s));
  if (s != UCS_OK){
     ERROR_SHMEM("AM Failed with status %s\n", ucs_status_string(s));
     shmem_global_exit(s);
  }
  

 // shmemc_progress();
  free(response);

  DEBUG_SHMEM("Done with the encryption and return to sender\n");

//  shmemx_decrypt_single_buffer_omp((unsigned char *)r_dest, 0, (void *)r_dest,
//        0, func_data->local_size,
//        ((size_t *)(func_data->encrypted_size)));


fn_exit:
  return UCS_OK;
}

ucs_status_t get_dec_resp_handler(void *arg, const void *header, size_t h_size,
                                  void *data, size_t len,
                                  const ucp_am_recv_param_t *param) {

  int rank = proc.li.rank;
  func_args_t *func_data = (func_args_t *)data;
  uint64_t dest = (uint64_t)(func_data->local_buf);
  int local_size = func_data->local_size;
  int remainder = func_data->remainder;
#if use_ctr
  //memcpy(IV, func_data->IV, AES_TAG_LEN);
#endif /* use_ctr */


  unsigned char *local_ptr = func_data->local_buf;
  DEBUG_SHMEM("Local_ptr: %p\n", local_ptr);

  int magic = SIXTEEN_K;

  int magic2 = 1024; /* Baseline for actual working items */


//  shmemc_ctx_get(defcp, dest, func_data->get_rem_buf, func_data->encrypted_size, func_data->src_pe);
//#if 0
  if (func_data->encrypted_size <= magic){
             shmemc_ctx_get_nbi(defcp, dest, func_data->get_rem_buf, func_data->encrypted_size, func_data->src_pe);
     
  }else{
     int counter = 0,
         segments = func_data->encrypted_size / magic,
         remainder = func_data->encrypted_size % magic,
         cur_total = 0;

     while (1) {
        cur_total+=magic;

        if (cur_total > func_data->encrypted_size){
           shmemc_ctx_get_nbi(defcp, dest+(counter*magic), func_data->get_rem_buf+(counter*magic), remainder, func_data->src_pe);
           break;
        }else{
           shmemc_ctx_get_nbi(defcp, dest+(counter*magic), func_data->get_rem_buf+(counter*magic), magic, func_data->src_pe);
        }
        counter++;
     }
  }
//#endif


  func_args_t *response = (func_args_t *)malloc(sizeof(func_args_t)); // func_data->local_size+AES_TAG_LEN+AES_RAND_BYTES);
  response->local_size = func_data->local_size;
  response->local_buf = func_data->local_buf;
  DEBUG_SHMEM("Local buffer again %p\n", response->local_buf);
  response->remote_buffer = func_data->remote_buffer;
  response->src_pe = func_data->src_pe;
  response->dst_pe = func_data->dst_pe;
  response->get_rem_buf = func_data->get_rem_buf;
#if use_ctr
  //memcpy(response->IV, IV, AES_TAG_LEN);
#endif
  response->encrypted_size = func_data->encrypted_size;

  shmemx_decrypt_single_buffer_omp((unsigned char *)local_ptr, 0, (void *)dest, 0,
        func_data->local_size,
        ((size_t)(func_data->encrypted_size)));

  ucp_ep_h return_ep;
  return_ep = lookup_ucp_ep(defcp, func_data->src_pe);

  ucp_request_param_t ack_param = {
      .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
      .cb.send = NULL,
      .datatype = ucp_dt_make_contig(sizeof(unsigned char))};

  DEBUG_SHMEM("Heading to the local decryption response handler!\n");
  ucs_status_ptr_t sp =
      ucp_am_send_nbx(return_ep, AM_GET_DEC_RESPONSE_2, NULL, 0, response,
                      sizeof(func_args_t), &ack_param);
  //shmemc_progress();
   DEBUG_SHMEM("Progressing....\n");
  ucs_status_t st = check_wait_for_request(defcp, sp);
  shmemu_assert(st == UCS_OK, "%s: failed (status: %s)", __func__,
                ucs_status_string(st));

 // st = check_wait_for_request(defcp, sp);
 // shmemu_assert(st == UCS_OK, "%s: failed (status: %s)", __func__,
  //              ucs_status_string(st));

 
   // memset(local_ptr+func_data->local_size, 0, 10);


  return UCS_OK;
}


ucs_status_t get_dec_resp_handler_rem(void *arg, const void *header, size_t h_size,
                                  void *data, size_t len,
                                  const ucp_am_recv_param_t *param) {

  int rank = proc.li.rank;
  func_args_t *func_data = (func_args_t *)data;
  uint64_t dest = (uint64_t)(func_data->remote_buffer);
  int offset_from_start = func_data->offset_from_start;
  int local_size = func_data->local_size;
  int remainder = func_data->remainder;
#if use_ctr
//  memcpy(IV, func_data->IV, AES_TAG_LEN);
#endif /* use_ctr */



   DEBUG_SHMEM("Decryption on remote side for remote dest: %p\n", dest); 
  shmemx_decrypt_single_buffer_omp((unsigned char *)dest, 0, (void *)dest, 0,
                                 func_data->local_size,
                                 ((size_t)(func_data->encrypted_size)));

  return UCS_OK;
}


ucs_status_t nbget_handler(void *arg, const void *header, size_t h_size,
                                  void *data, size_t len,
                                  const ucp_am_recv_param_t *param) {

  int rank = proc.li.rank;
  func_args_t *func_data = (func_args_t *)data;
  uint64_t r_dest = (uint64_t)(func_data->remote_buffer);
//   unsigned char *put_ptr = malloc(func_data->local_size + KILO);
#if use_ctr
//   memcpy(IV, func_data->IV, AES_TAG_LEN);
#endif /* use_ctr */

  int segment_count = 0;
    segment_count = shmemx_encrypt_single_buffer_omp((unsigned char *)r_dest, 0, (void *)r_dest, 0,
                                 func_data->local_size,
                                 &(func_data->encrypted_size));

 DEBUG_SHMEM("Encrypted size is: %d, local_size is %d\n", func_data->encrypted_size, func_data->local_size);
 // int count = func_data->encrypted_size + (segment_count * (AES_TAG_LEN + AES_RAND_BYTES));
 // DEBUG_SHMEM("Count post-encryption from local size %d and enc_size %d is %d\n", 
 //       func_data->local_size, func_data->encrypted_size, count);

  func_args_t *response = malloc(sizeof(func_args_t));
  response->remote_buffer = r_dest;
  response->local_buf = func_data->local_buf;
  response->get_rem_buf = func_data->get_rem_buf;
  response->local_size = func_data->local_size;
  response->encrypted_size = func_data->encrypted_size;
  response->src_pe = func_data->src_pe;
  response->dst_pe = func_data->dst_pe;
#if use_ctr
//  memcpy(response->IV, IV, AES_TAG_LEN);
#endif


  ucp_ep_h ep = NULL;
  ucp_request_param_t ack_param = {
     .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
      .cb.send = NULL,
      .datatype = ucp_dt_make_contig(sizeof(unsigned char))};

  ep = lookup_ucp_ep(defcp, response->dst_pe);

  ucs_status_ptr_t sp = ucp_am_send_nbx(ep, AM_NBGET_HANDLER_2, NULL, 0, response,
        sizeof(func_args_t), &ack_param);
  //shmemc_progress();
  ucs_status_t st = check_wait_for_request(defcp, sp);
  shmemu_assert(st == UCS_OK, "%s: nb get enc failed (status: %s)", __func__,
        ucs_status_string(st));

  free(response);
 
  return UCS_OK;
}

ucs_status_t nbget_handler_local(void *arg, const void *header, size_t h_size,
                                  void *data, size_t len,
                                  const ucp_am_recv_param_t *param) {

  int rank = proc.li.rank;
  func_args_t *func_data = (func_args_t *)data;
  //uint64_t r_dest = (uint64_t)(func_data->remote_buffer);
//   unsigned char *put_ptr = malloc(func_data->local_size + KILO);
#if use_ctr
//   memcpy(IV, func_data->IV, AES_TAG_LEN);
#endif /* use_ctr */
   DEBUG_SHMEM("About to non-block get\n");
   shmemc_ctx_get_nbi(defcp, func_data->local_buf, func_data->get_rem_buf, func_data->encrypted_size, func_data->src_pe);
   DEBUG_SHMEM("NBget done. Off to the rest of the application\n");

  return UCS_OK;
}



static inline void handleErrors(char *message) {
  ERR_print_errors_fp(stderr);
  shmemu_fatal("shmem_enc_dec: %s\n", message);
}

void shmemx_sec_init() {

  char *enc_dec = NULL;
  int res = 0;

  if ((enc_dec = getenv("SHMEM_ENABLE_ENCRYPTION")) != NULL) {
    proc.env.shmem_encryption = !!atoi(enc_dec);
    assert(proc.env.shmem_encryption == 0 || proc.env.shmem_encryption == 1);
  }


  // nbi_put_ciphertext = malloc(sizeof(unsigned char
  // *)*NON_BLOCKING_OP_COUNT*2);
  nbi_get_ciphertext =
      malloc(sizeof(unsigned char *) * NON_BLOCKING_OP_COUNT * 2);
  nb_put_ctr = (shmem_secure_attr_t *)malloc(sizeof(shmem_secure_attr_t) *
                                             NON_BLOCKING_OP_COUNT * 2);
  nb_get_ctr = (shmem_secure_attr_t *)malloc(sizeof(shmem_secure_attr_t) *
                                             NON_BLOCKING_OP_COUNT * 2);

  int i = 0;
  for (i = 0; i < MAX_THREAD_COUNT; i++) {
     if (!(openmp_enc_ctx[i] = EVP_CIPHER_CTX_new())) {
        handleErrors("OpenMP cipher failed to be created");
     }
     if (!(openmp_dec_ctx[i] = EVP_CIPHER_CTX_new())) {
        handleErrors("OpenMP cipher failed to be created");
     }



#if use_gcm
    /* Begin using AES_256_gcm */
    res = EVP_EncryptInit_ex(openmp_enc_ctx[i], EVP_aes_256_gcm(), NULL,
                             gcm_key, NULL);
    if (res != 1) {
      handleErrors("failed to begin encryption portion");
    }

    res = EVP_CIPHER_CTX_ctrl(openmp_enc_ctx[i], EVP_CTRL_GCM_SET_IVLEN,
                              (int)AES_RAND_BYTES, NULL);
    if (res != 1) {
      handleErrors("Failed to set up the Initialization Vector Length");
    }

    /* Begin using AES_256_gcm */
    res = EVP_DecryptInit_ex(openmp_dec_ctx[i], EVP_aes_256_gcm(), NULL,
                             gcm_key, NULL);
    if (res != 1) {
      handleErrors("failed to begin encryption portion");
    }

    res = EVP_CIPHER_CTX_ctrl(openmp_dec_ctx[i], EVP_CTRL_GCM_SET_IVLEN,
                              (int)AES_RAND_BYTES, NULL);
    if (res != 1) {
      handleErrors("Failed to set up the Initialization Vector Length");
    }
#elif use_ctr
#if 0
   res = EVP_EncryptInit_ex(openmp_enc_ctx[i], EVP_aes_256_ctr(), NULL,
          gcm_key, NULL);
    if (res != 1) {
      handleErrors("failed to begin encryption portion");
    }

    res = EVP_CIPHER_CTX_ctrl(openmp_enc_ctx[i], EVP_CTRL_GCM_SET_IVLEN,
                              (int)AES_RAND_BYTES, NULL);
    if (res != 1) {
      handleErrors("Failed to set up the Initialization Vector Length");
    }

    /* Begin using AES_256_ctr */
    res = EVP_DecryptInit_ex(openmp_dec_ctx[i], EVP_aes_256_ctr(), NULL,
                             gcm_key, NULL);
    if (res != 1) {
      handleErrors("failed to begin encryption portion");
    }

    res = EVP_CIPHER_CTX_ctrl(openmp_dec_ctx[i], EVP_CTRL_GCM_SET_IVLEN,
                              (int)AES_RAND_BYTES, NULL);
    if (res != 1) {
      handleErrors("Failed to set up the Initialization Vector Length");
    }
#endif /* if 0 */
#endif /* use_ctr or use_gcm */
  }

  ucs_status_t reg_status = UCS_OK;
  ucp_am_handler_param_t recv_handler_param = {
      .field_mask =
          UCP_AM_HANDLER_PARAM_FIELD_ID | UCP_AM_HANDLER_PARAM_FIELD_CB |
          UCP_AM_HANDLER_PARAM_FIELD_FLAGS | UCP_AM_HANDLER_PARAM_FIELD_ARG,
      //.id = AM_PUT_HANDLER,
      .flags = UCP_AM_FLAG_WHOLE_MSG,
      //.cb = put_handler,
      .arg = NULL};

  recv_handler_param.id = AM_GET_ENC_HANDLER;
  DEBUG_SHMEM("Registering get_enc handler\n");
  recv_handler_param.cb = get_enc_handler;
  reg_status = ucp_worker_set_am_recv_handler(defcp->w, &recv_handler_param);
  if (reg_status != UCS_OK) {
    handleErrors("am_recv_handler 2 failed\n");
  }


  recv_handler_param.id = AM_GET_DEC_RESPONSE;
  DEBUG_SHMEM("Registering get_dec handler\n");
  recv_handler_param.cb = get_dec_resp_handler;
  reg_status = ucp_worker_set_am_recv_handler(defcp->w, &recv_handler_param);
  if (reg_status != UCS_OK) {
    handleErrors("am_recv_handler 3 failed\n");
  }
  recv_handler_param.id = AM_GET_DEC_RESPONSE_2;
  DEBUG_SHMEM("Registering get_dec_local handler\n");
  recv_handler_param.cb = get_dec_resp_handler_rem;
  reg_status = ucp_worker_set_am_recv_handler(defcp->w, &recv_handler_param);
  if (reg_status != UCS_OK) {
    handleErrors("am_recv_handler 4 failed\n");
  }


  recv_handler_param.id = AM_NBPUT_HANDLER;
  DEBUG_SHMEM("Registering nbput handler\n");
  recv_handler_param.cb = put_dec_handler;

  reg_status = ucp_worker_set_am_recv_handler(defcp->w, &recv_handler_param);
  if (reg_status != UCS_OK) {
    handleErrors("am_recv_handler 5 failed\n");
  }

  recv_handler_param.id = AM_NBGET_HANDLER;
  DEBUG_SHMEM("Registering nbget_handler\n");
  recv_handler_param.cb = nbget_handler;

  reg_status = ucp_worker_set_am_recv_handler(defcp->w, &recv_handler_param);
  if (reg_status != UCS_OK) {
    handleErrors("am_recv_handler 6 failed\n");
  }

  recv_handler_param.id = AM_NBGET_HANDLER_2;
  DEBUG_SHMEM("Registering nbget_handler_local\n");
  recv_handler_param.cb = nbget_handler_local;

  reg_status = ucp_worker_set_am_recv_handler(defcp->w, &recv_handler_param);
  if (reg_status != UCS_OK) {
    handleErrors("am_recv_handler 7 failed\n");
  }


  return;
}

#if use_gcm
int shmemx_encrypt_single_buffer_omp(unsigned char *cipherbuf,
                                     unsigned long long src, const void *sbuf,
                                     unsigned long long dest, size_t bytes,
                                     size_t *cipherlen) {

  int res = 0;
  int segment_count = 0, count = 0;
  int local_cipherlen = 0;
  int max_data = 0;
  *cipherlen = 0; /* Just in case */

  int thread_no = get_thread_count(bytes); // Starting thread point

  int data = bytes / thread_no;
  // data++;
  DEBUG_SHMEM("bytes = %d  max_thread_no %d\n", data, thread_no);

  if (bytes <= 16) {
    segment_count = 1;
    data = bytes;
  } else {
    segment_count = (bytes - 1) / data + 1;
  }

  int i = 0, quantity = 0;
//  for (i = 0; i < segment_count; i++){
//     count = (i * data);
//     quantity = data;

 //    if (i == segment_count - 1){
 //        quantity = bytes - (data * (segment_count - 1));
 //    }
 //    memcpy(plain_chunks[i], sbuf+count, quantity);
 // }

  int enc_data = data;

  int position = 0;

  int temp_cipherlen = 0;

  // unsigned char *key = &(gcm_key[0]);
  //  DEBUG_SHMEM("Segment_count %d, data = %d, bytes = %d  max_thread_no %d\n",
  //              segment_count, data, bytes, thread_no);

  //   if (segment_count == 1){
  //      return shmemx_encrypt_single_buffer(cipherbuf, src, sbuf, dest, bytes,
  //      cipherlen);
  //   }

  DEBUG_SHMEM("[START_ENCRYPTION] Starting parallel for plaintext: %s \n",
              (char *)sbuf);
#pragma omp parallel for schedule(dynamic) default(none)                       \
    private(max_data, position, res, local_cipherlen, enc_data)                \
    shared(src, dest, openmp_enc_ctx, stdout, stderr, segment_count, data,     \
               sbuf, cipherbuf, temp_cipherlen, proc, bytes, gcm_key, plain_chunks, enc_chunks)          \
    num_threads(thread_no)
  for (count = 0; count < segment_count; count++) {

    int tn = omp_get_thread_num();
    
    position = count * (data + AES_TAG_LEN + AES_RAND_BYTES);
    int pos2 = count * data;
    void *tmp_buf =  cipherbuf + position;
    void *tmp_buf2 = sbuf+pos2; //plain_chunks[count]; 

    //  RAND_bytes(tmp_buf, AES_RAND_BYTES);
    EVP_CIPHER_CTX *local_ctx = openmp_enc_ctx[tn];
    RAND_bytes(tmp_buf + src, AES_RAND_BYTES);
    enc_data = data;
    max_data = enc_data + AES_TAG_LEN;

    if ((count == segment_count - 1)) {
      enc_data = bytes - (data * (segment_count - 1));
      max_data = enc_data + AES_TAG_LEN;
      DEBUG_SHMEM("Last count enc: enc_data %d, max_data %d\n", enc_data,
                  max_data);
    }
    DEBUG_SHMEM(
        "[T_%d] local_ctx %p enc_data %d max_data %d tmp_buf: %p (cipher buf "
        "%p + count %d * (data %d + AES_TAG_LEN %d + AES_RAND_BYTES %d))\n",
        tn, local_ctx, enc_data, max_data, tmp_buf, (void *)cipherbuf + src,
        count, enc_data, AES_TAG_LEN, AES_RAND_BYTES);

    if ((res = EVP_EncryptInit_ex(local_ctx, NULL, NULL, NULL, tmp_buf)) != 1) {
      ERROR_SHMEM("[T_%d] EncryptInit_ex from error %lu: %s\n", tn,
                  ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
      shmem_global_exit(ERR_get_error());
    }

    DEBUG_SHMEM("[T_%d] EncryptInit_ex passed\n", tn);

    if ((res = EVP_EncryptUpdate(
             local_ctx, tmp_buf + src + AES_RAND_BYTES, &local_cipherlen,
             ((const unsigned char *)(tmp_buf2 + dest)), (int)enc_data)) != 1) {
      ERROR_SHMEM("[T_%d] EncryptUpdate Failed: %lu %s\n", tn, ERR_get_error(),
                  ERR_error_string(res, NULL));
      shmem_global_exit(ERR_get_error());
    }

    //      DEBUG_SHMEM("[T_%d] EncrypUpdate passed with cipherlen %d\n", tn,
    //      local_cipherlen);
    temp_cipherlen += local_cipherlen; /* Adds to the global cipherlen length */

    DEBUG_SHMEM("[T_%d] Entering final with local_ctx %p tmp_buf %p + "
                "AES_TAG_LEN %d + src %llu + local_cipherlen %d\n",
                tn, local_ctx, tmp_buf, AES_TAG_LEN, src, local_cipherlen);

    if ((res = EVP_EncryptFinal_ex(
             local_ctx, tmp_buf + AES_RAND_BYTES + src + local_cipherlen,
             &local_cipherlen)) != 1) {
      ERROR_SHMEM("[T_%d] EncryptFinal_ex failed: %lu %s\n", tn,
                  ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
      shmem_global_exit(ERR_get_error());
    }

    // #pragma omp barrier

    DEBUG_SHMEM("[T_%d] EncrypFinal_ex passed\n", tn);
    if ((res = EVP_CIPHER_CTX_ctrl(local_ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_LEN,
                                   tmp_buf + src + AES_RAND_BYTES + enc_data)) != 1) {
      ERROR_SHMEM("[T_%d]: CTX_CTRL: %s\n", tn,
                  ERR_error_string(ERR_get_error(), NULL));
      shmem_global_exit(ERR_get_error());
    }
    DEBUG_SHMEM("[T_%d] CIPHER_CTX_CTRL passed\n", tn);
  }

 // for (count = 0 ; count < segment_count; count++){
 //    enc_data = data;
 //    max_data = enc_data + AES_TAG_LEN;

  // /  int offset = (count * (data + AES_TAG_LEN + AES_RAND_BYTES));
 //    if (count == segment_count - 1){
 //       enc_data = (bytes - data * (segment_count - 1));
 //       max_data = enc_data + AES_TAG_LEN;
 //    }
 //    memcpy(cipherbuf + offset, enc_chunks[count], max_data);
 // }



  *cipherlen = temp_cipherlen;
  DEBUG_SHMEM("[END_ENCRYPTION] Final cipherlen: %lu, CIPHERTEXT: %s\n",
              *cipherlen, cipherbuf);

  return segment_count;
}

int shmemx_decrypt_single_buffer_omp(unsigned char *cipherbuf,
                                     unsigned long long src, void *rbuf,
                                     unsigned long long dest, size_t bytes,
                                     size_t cipher_len) {

  int res = 0;
  int segment_count = 0, count = 0;
  int local_cipherlen = 0;
  int max_data = 0;

  if (bytes < 0 ){
     ERROR_SHMEM("Byte count is less than 0: %lu\n", bytes);
     shmem_global_exit(-1);
  }

  int other_cipherlen = 0;
  int thread_no = get_thread_count(bytes); // Starting thread point

  int data = bytes / thread_no;
  DEBUG_SHMEM("Data: %d\n", data);
  max_data = data + AES_RAND_BYTES;
  data++;

  if (bytes <= 16) {
    segment_count = 1;
    data = bytes;
  } else {
    segment_count = (bytes - 1) / data + 1;
  }

  if (segment_count > MAX_THREAD_COUNT*2 || segment_count <= 0 ){
     ERROR_SHMEM("Segment count is out of wack!! %d\n", segment_count);
     shmem_global_exit(-1);
  }

  int position = 0;
  int enc_data = data;
  max_data = data + AES_TAG_LEN ;

//  for (count = 0; count < segment_count; count++){
//     if (count == segment_count - 1){
//        enc_data = (bytes) - (data * (segment_count - 1));
//        max_data = enc_data + AES_TAG_LEN ;
//     }
//     int offset = count * (max_data);
//     memcpy(dec_chunks[count], cipherbuf+offset, max_data);
//  }


  int temp_cipherlen = 0;

  DEBUG_SHMEM("Segment_count %d, data = %d, max_thread_no %d\n", segment_count,
              data, thread_no);


  DEBUG_SHMEM("[START_DECRYPTION] Ciphertext: %s\n", cipherbuf);
#pragma omp parallel for schedule(dynamic) default(none)                       \
    private(count, max_data, position, res, local_cipherlen, enc_data)         \
    shared(segment_count, stdout, stderr, openmp_dec_ctx, data, cipherbuf,     \
               rbuf, cipher_len, src, dest, bytes, proc, gcm_key, plain_chunks, dec_chunks)              \
    num_threads(thread_no)
  for (count = 0; count < segment_count; count++) {

    int tn = omp_get_thread_num();
    //     DEBUG_SHMEM("[T_%d] start\n", tn);
    // int cipher_temp = 0;
        
    enc_data = data;
    max_data = data + AES_RAND_BYTES;

    if ((count == segment_count - 1)) {
      enc_data = (bytes - data * (segment_count - 1));
      max_data = enc_data + AES_RAND_BYTES;
    }
    //position = count * (max_data);
    void *tmp_buf = cipherbuf+(count * (enc_data+AES_TAG_LEN+AES_RAND_BYTES));//dec_chunks[count];
    void *tmp_buf2 = rbuf+(count*enc_data);//plain_chunks[count]; //rbuf + (count * data);
  
    EVP_CIPHER_CTX *local_ctx = openmp_dec_ctx[tn];



    DEBUG_SHMEM(
        "T_%d Params: ctx %p, rbuf+(%d): %p, cipher_len ptr %p, cipherbuf %p + "
        "src %d + RAND BYTES %d, bytes %d - AES_RAND_BYTES %d\n",
        tn, local_ctx, dest, (rbuf + dest), (&cipher_len), cipherbuf, src,
        AES_RAND_BYTES, enc_data, AES_RAND_BYTES);

    if ((res = EVP_DecryptInit_ex(local_ctx, NULL, NULL, NULL, tmp_buf)) != 1) {
      ERROR_SHMEM("[T_%d] DecryptInit_ex failed: %lu %s\n", tn, ERR_get_error(),
                  ERR_error_string(ERR_get_error(), NULL));
      shmem_global_exit(ERR_get_error());
    }

    if ((res = EVP_DecryptUpdate(
             local_ctx, ((unsigned char *)(tmp_buf2 + dest)),
             (int *)(&local_cipherlen), tmp_buf + src + AES_RAND_BYTES,
             enc_data)) != 1) {
      ERROR_SHMEM("[T_%d] DecryptUpdate failed: %lu %s\n", tn, ERR_get_error(),
                  ERR_error_string(res, NULL));
      shmem_global_exit(ERR_get_error());
    }

    DEBUG_SHMEM("T_%d DecryptUpdated passed; cipherlen: %u, local_cipherlen: %u\n", tn, cipher_len, local_cipherlen);

    if ((res = EVP_CIPHER_CTX_ctrl(local_ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_LEN,
                                   tmp_buf + src + AES_RAND_BYTES + enc_data)) !=
        1) {
      ERROR_SHMEM("[T_%d] CIPHER_CTX_ctrl failed: %lu %s\n", tn,
                  ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
      shmem_global_exit(ERR_get_error());
    }

    DEBUG_SHMEM("T_%d CTX_ctrl passed \n", tn);
    if ((res =
             EVP_DecryptFinal_ex(local_ctx, (tmp_buf2 + dest + enc_data),
                                 (int *)(&local_cipherlen))) != 1) {
      /*handleErrors*/
      ERROR_SHMEM("[T_%d] Decryption Tag Verification Failed %lu %s\n", tn,
                  ERR_get_error(), ERR_error_string(ERR_get_error(), NULL));
     // shmem_global_exit(ERR_get_error());
    }

    DEBUG_SHMEM("T_%d DecryptFinal_ex passed\n", tn);
  }

//  enc_data = data;
//  int offset = 0;
//  for (count = 0; count < segment_count; count++){
//     if (count == segment_count - 1){
//        enc_data = (bytes - (data * (segment_count - 1)));
//     }
//     offset = (count * enc_data);
//     memcpy(rbuf+offset, plain_chunks[count], enc_data);
//  }


  memset(rbuf+bytes, 0, 10);

//  DEBUG_SHMEM("[END_DECRYPTION] plaintext: %s\n", (char *)rbuf);

  return 0;
}

#elif use_ctr

static void aes_ctr_dec(unsigned long long counter_val,
                           unsigned char *inbuf,
                           unsigned char *outbuf, 
                           unsigned long long len, unsigned long long src, int dest,
                           size_t *cipherlen, EVP_CIPHER_CTX *ctx) {

//   RAND_bytes(IV, AES_RAND_BYTES);
//   IV[AES_RAND_BYTES] = (counter_val >> 24) & 0xFF;
//   IV[AES_RAND_BYTES+1] = (counter_val >> 16) & 0xFF;
//   IV[AES_RAND_BYTES+2] = (counter_val >> 8 ) & 0xFF;
//   IV[AES_RAND_BYTES+3] = (counter_val) & 0xFF;
   if (EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL,gcm_key, IV) != 1){
      handleErrors("EncryptInit Failed\n");
   }

   *cipherlen = 0;

   if (EVP_EncryptUpdate(ctx, outbuf+dest, cipherlen, inbuf+src, (int) len) != 1){
      handleErrors("EncryptUpdate Failed\n");
   }

   DEBUG_SHMEM("Cipherlen 1: %lu\n", *cipherlen);

   if (EVP_EncryptFinal_ex(ctx, outbuf+(*cipherlen)+dest, cipherlen) != 1){
      handleErrors("EncryptFinal Failed\n");
   }

   DEBUG_SHMEM("Cipherlen: %lu\n", *cipherlen);

}

int shmemx_encrypt_single_buffer_omp(unsigned char *cipherbuf,
                                     unsigned long long src, const void *sbuf,
                                     unsigned long long dest, size_t bytes,
                                     size_t *cipherlen) {


  int thread_no = get_thread_count(bytes); // Starting thread point

  int data = bytes / thread_no;
  int segment_count = 0;
  int count = 0;
  DEBUG_SHMEM("bytes = %d  max_thread_no %d\n", data, thread_no);

//  thread_no = 1;
  if (bytes <= 16) {
    segment_count = 1;
    data = bytes;
  } else {
    segment_count = (bytes - 1) / data + 1;
  }

//  if (thread_no == 1){
//     segment_count = 1;
//  }

  int enc_data = data;
  int max_data = enc_data + AES_TAG_LEN;

  int local_cipherlen = 0,
      temp_cipherlen = 0;

  unsigned long long counter_val = (data*AES_TAG_LEN) % thread_no;

//   RAND_bytes(IV, AES_RAND_BYTES);
//   IV[AES_RAND_BYTES] = (counter_val >> 24) & 0xFF;
//   IV[AES_RAND_BYTES+1] = (counter_val >> 16) & 0xFF;
//   IV[AES_RAND_BYTES+2] = (counter_val >> 8 ) & 0xFF;
//   IV[AES_RAND_BYTES+3] = (counter_val) & 0xFF;
//

  for (count = 0 ; count < thread_no; count++){
      openmp_enc_ctx[count] = EVP_CIPHER_CTX_new();
      if (!(openmp_enc_ctx[count])){
          handleErrors("Can't create cipher_ctx\n");
      }
  }

  char *sbuf2 = (char *)sbuf;

   DEBUG_SHMEM("segment_count %d, enc_data %d, max_data %d\n", segment_count, enc_data, max_data);
 
  DEBUG_SHMEM("[START_ENCRYPTION] Starting parallel for plaintext: %x %x %x %x %s\n",
        sbuf2[0],  sbuf2[1],  sbuf2[2], sbuf2[3], sbuf );
//
//   //default(none)  private(local_cipherlen) shared(src, dest, openmp_enc_ctx, stdout, stderr, segment_count, data, sbuf, enc_data, cipherbuf, temp_cipherlen, bytes, gcm_key, plain_chunks, enc_chunks, thread_no, IV, proc) num_threads(thread_no)d config.log
//
#pragma omp parallel for num_threads(thread_no)
  for (count = 0; count < segment_count; count++) {

      int offset = (count * enc_data);
      unsigned char* tmp_plain = sbuf + offset;
      unsigned char* tmp_enc = cipherbuf + offset;
      int tn = omp_get_thread_num();

      DEBUG_SHMEM("Running on thread %d\n", tn);

     EVP_CIPHER_CTX *ctx = openmp_enc_ctx[count];
  //   ctx = EVP_CIPHER_CTX_new();
  //   if (!ctx) { 
  //      handleErrors("Can't create a cipher ctx\n");
  //   }

     if (EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL,gcm_key, IV) != 1){
        handleErrors("EncryptInit Failed\n");
     }

     if (EVP_EncryptUpdate(ctx, tmp_enc+src, &local_cipherlen, tmp_plain+src, (int) enc_data) != 1){
        handleErrors("EncryptUpdate Failed\n");
     }

     temp_cipherlen += local_cipherlen;

     DEBUG_SHMEM("[T_%d] Local_cipherlen 1: %lu\n", tn, local_cipherlen);


     if (EVP_EncryptFinal_ex(ctx, tmp_enc+local_cipherlen+dest, &local_cipherlen) != 1){
        handleErrors("EncryptFinal Failed\n");
     }

     DEBUG_SHMEM("[T_%d] Local_cipherlen 2: %lu\n", tn, local_cipherlen);
   //  aes_ctr_enc(counter_val, sbuf + (count*enc_data), cipherbuf + (count*enc_data), enc_data, &local_cipherlen,openmp_enc_ctx[count]);

     temp_cipherlen += local_cipherlen;

  }

  for (count = 0 ; count < thread_no; count++){
      EVP_CIPHER_CTX_free(openmp_enc_ctx[count]);
  }


  *cipherlen = temp_cipherlen;
  DEBUG_SHMEM("Cipherlen: %lu\n", temp_cipherlen);
//  memset(cipherbuf+(*cipherlen), 0,32);

  DEBUG_SHMEM("[END_ENCRYPTION] CIPHERTEXT: %x %x %x %x %s\n", cipherbuf[0], cipherbuf[1], cipherbuf[2], cipherbuf[3], cipherbuf);

  return segment_count;
}

int shmemx_decrypt_single_buffer_omp(unsigned char *cipherbuf,
                                     unsigned long long src, void *rbuf,
                                     unsigned long long dest, size_t bytes,
                                     size_t cipher_len) {

  int res = 0;
  int segment_count = 0, count = 0;
  int local_cipherlen = 0;
  int max_data = 0;

  if (bytes < 0 ){
     ERROR_SHMEM("Byte count is less than 0: %lu\n", bytes);
     shmem_global_exit(-1);
  }

  int other_cipherlen = 0;
  int thread_no = get_thread_count(bytes); // Starting thread point
//   thread_no = 1;
  int data = bytes / thread_no;
  DEBUG_SHMEM("Data: %d\n", data);
  max_data = data + AES_TAG_LEN;
 // data++;

  if (bytes <= 16) {
    segment_count = 1;
    data = bytes;
  } else {
    segment_count = (bytes - 1) / data + 1;
  }
//  if (thread_no == 1){
//     segment_count = 1;
//  }

  if (segment_count > MAX_THREAD_COUNT*2 || segment_count <= 0 ){
     ERROR_SHMEM("Segment count is out of wack!! %d\n", segment_count);
     shmem_global_exit(-1);
  }

  int position = 0;
  int enc_data = data;
  max_data = data + AES_TAG_LEN;

//  for (count = 0; count < segment_count; count++){
//     if (count == segment_count - 1){
//        enc_data = (bytes) - (data * (segment_count - 1));
//        max_data = enc_data + AES_TAG_LEN ;
//     }
//     int offset = count * (max_data);
//     memcpy(dec_chunks[count], cipherbuf+offset, max_data);
//  }


  int temp_cipherlen = 0;
 
  DEBUG_SHMEM("Segment_count %d, data = %d, max_thread_no %d\n", segment_count,
              data, thread_no);

  unsigned long long counter_val = 0;


  /*               \
    */

  DEBUG_SHMEM("[START_DECRYPTION] CIPHERTEXT: %x %x %x %x %s\n", cipherbuf[0], cipherbuf[1], cipherbuf[2], cipherbuf[3], cipherbuf);

#pragma omp parallel for default(none)                       \
  private(count, local_cipherlen)         \
  shared(segment_count, enc_data, stdout, stderr, openmp_dec_ctx, data, cipherbuf,     \
          rbuf, src, dest, proc, gcm_key, thread_no, counter_val) num_threads(thread_no) 
  for (count = 0; count < segment_count; count++) {
    // counter_val = (count * enc_data/thread_no);
     
      int tn = omp_get_thread_num();
      DEBUG_SHMEM("T_%d starting decryption\n", tn);
      aes_ctr_dec( counter_val, cipherbuf + (count * enc_data), rbuf + (count*enc_data), enc_data, src, dest, &local_cipherlen, openmp_dec_ctx[count]);

   }

  char *rbuf2 = (char *) rbuf;

  DEBUG_SHMEM("[END_DECRYPTION] plaintext: %x %x %x %x %s\n",  rbuf2[0], rbuf2[1], rbuf2[2], rbuf2[3], rbuf);

  return 0;
}

#endif /* use_ctr_or_gcm */


int shmemx_encrypt_single_buffer(unsigned char *cipherbuf,
                                 unsigned long long src, const void *sbuf,
                                 unsigned long long dest, size_t bytes,
                                 size_t *cipherlen) {

  int res = 0;
  int len = 0, temp_len = 0;

  int const_bytes = AES_RAND_BYTES;
  // DEBUG_SHMEM("Entering rand_bytes with cipherbuf+src: %p+0x%x\n",
  //       cipherbuf, src);
  RAND_bytes(cipherbuf + src, const_bytes);

  //   DEBUG_SHMEM("send_buf: %p, src %llu, dest %llu, cipherbuf: %p,
  //   defcp->enc_ctx: %p\n",
  //         sbuf, src, dest, cipherbuf, defcp->enc_ctx);

  //    DEBUG_SHMEM("Byte count :%d \n", (int)bytes);
  if ((res = EVP_EncryptInit_ex(defcp->enc_ctx, EVP_aes_256_gcm(), NULL,
                                gcm_key, cipherbuf + src)) != 1) {
    ERROR_SHMEM("EncryptInit_ex from error %lu: %s\n", ERR_get_error(),
                ERR_error_string(ERR_get_error(), NULL));
    shmem_global_exit(ERR_get_error());
  }

  DEBUG_SHMEM("EncryptInit passed\n");

  if ((res = EVP_EncryptUpdate(defcp->enc_ctx, cipherbuf + src + const_bytes,
                               &len, ((const unsigned char *)(sbuf + dest)),
                               (int)bytes)) != 1) {
    ERROR_SHMEM("Encrypt_Update failed: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
    shmem_global_exit(ERR_get_error());
  }

  DEBUG_SHMEM("EncryptUpdate passed; block_put_cipherlen: %lu\n", (size_t)len);

  shmemu_assert(len > 0, "shmemx_encrypt_single_buffer: ciphertext is 0...\n");

  temp_len = len;

  if ((res = EVP_EncryptFinal_ex(
           defcp->enc_ctx, cipherbuf + const_bytes + src + (len), &len)) != 1) {
    ERROR_SHMEM("EncryptFinal_ex failed: %s\n",
                ERR_error_string(ERR_get_error(), NULL));
    shmem_global_exit(ERR_get_error());
  }

  DEBUG_SHMEM("EncryptFinal passed. Len: %d\n", temp_len + len);

  if ((res = EVP_CIPHER_CTX_ctrl(defcp->enc_ctx, EVP_CTRL_GCM_GET_TAG,
                                 AES_TAG_LEN,
                                 cipherbuf + const_bytes + src + bytes)) != 1) {
    ERROR_SHMEM("CTX_CTRL: %s\n", ERR_error_string(ERR_get_error(), NULL));
    shmem_global_exit(ERR_get_error());
  }
  temp_len += len;

  DEBUG_SHMEM("Ciphertext %p, Cipher_len %d\n", cipherbuf, temp_len);

  *cipherlen = temp_len;

  return 0;
}


int shmemx_decrypt_single_buffer(unsigned char *cipherbuf,
                                 unsigned long long src, void *rbuf,
                                 unsigned long long dest, size_t bytes,
                                 size_t cipher_len) {

  int res = 0;

  DEBUG_SHMEM("cipherbuf %p, src %llu, rbuf %p, dest %llu, bytes %lu\n",
              cipherbuf, src, (void *)rbuf, dest, bytes);

  if ((res = EVP_DecryptInit_ex(defcp->dec_ctx, EVP_aes_256_gcm(), NULL,
                                gcm_key, cipherbuf + src)) != 1) {
    ERROR_SHMEM("DecryptInit_ex failed: %lu %s\n", ERR_get_error(),
                ERR_error_string(res, NULL));
    shmem_global_exit(ERR_get_error());
  }

  DEBUG_SHMEM("DecryptInit_ex passed \n");
  DEBUG_SHMEM("Params: ctx %p, rbuf+(%llu): %p, cipher_len ptr %p, cipherbuf "
              "%p + src %llu + RAND BYTES %d, bytes %lu - AES_RAND_BYTES %d\n",
              defcp->dec_ctx, dest, (rbuf + dest), (&cipher_len), cipherbuf,
              src, AES_RAND_BYTES, bytes, AES_RAND_BYTES);

  if ((res = EVP_DecryptUpdate(defcp->dec_ctx, ((unsigned char *)(rbuf + dest)),
                               (int *)(&cipher_len),
                               cipherbuf + src + AES_RAND_BYTES,
                               (bytes - AES_RAND_BYTES))) != 1) {
    ERROR_SHMEM("DecryptUpdate failed: %lu %s\n", ERR_get_error(),
                ERR_error_string(res, NULL));
    shmem_global_exit(ERR_get_error());
  }

  DEBUG_SHMEM("DecryptUpdated passed; cipherlen: %lu\n", cipher_len);

  if ((res = EVP_CIPHER_CTX_ctrl(defcp->dec_ctx, EVP_CTRL_GCM_SET_TAG,
                                 AES_TAG_LEN, (cipherbuf + dest + (bytes)))) !=
      1) {
    ERROR_SHMEM("CIPHER_CTX_ctrl failed: %lu %s\n", ERR_get_error(),
                ERR_error_string(res, NULL));
    shmem_global_exit(ERR_get_error());
  }

  int temp_len = cipher_len;

  DEBUG_SHMEM("CTX_ctrl passed \n");
  if ((res = EVP_DecryptFinal_ex(defcp->dec_ctx, (rbuf + dest + cipher_len),
                                 (int *)(&cipher_len))) != 1) {
    /*handleErrors*/
    ERROR_SHMEM("Decryption Tag Verification Failed\n");
  }

  DEBUG_SHMEM("DecryptFinal_ex passed\n");
  return res != 0 ? 0 : 0;
}

void shmemx_secure_put_nbi(shmem_ctx_t ctx, void *dest, const void *src,
                           size_t nbytes, int pe) {

  size_t cipherlen = 0;
  memset(nbi_put_ciphertext[nbput_count], 0,
         MAX_MSG_SIZE + OFFSET);
  int segment_count = shmemx_encrypt_single_buffer_omp(
      ((unsigned char *)(&(nbi_put_ciphertext[nbput_count][0]))), 0, src, 0,
      nbytes, ((size_t *)(&cipherlen)));

  int res_bytes = cipherlen ;//+ (segment_count *(AES_TAG_LEN + AES_RAND_BYTES));

  DEBUG_SHMEM("Encryption successful\n");
  shmemc_ctx_put_nbi(ctx, dest, (nbi_put_ciphertext[nbput_count]), res_bytes, pe);

  DEBUG_SHMEM("Non-blocking_put successful\n");

  shmemc_context_h ch = (shmemc_context_h)ctx;
  uint64_t r_dest;  /* address on other PE */
  ucp_rkey_h r_key; /* rkey for remote address */
  get_remote_key_and_addr(ch, (uint64_t)dest, pe, &r_key, &r_dest);

  uint64_t local_dest;
  ucp_rkey_h local_rkey;

  get_remote_key_and_addr(ch, (uint64_t)src, proc.li.rank, &local_rkey,
                          &local_dest);

  nb_put_ctr[nbput_count].src_pe = 0; // proc.li.rank;
  nb_put_ctr[nbput_count].dst_pe = 0;
  nb_put_ctr[nbput_count].res_pe = pe;
  nb_put_ctr[nbput_count].plaintext_size = nbytes;
  nb_put_ctr[nbput_count].encrypted_size = cipherlen;
  nb_put_ctr[nbput_count].remote_buf_addr = r_dest;
  nb_put_ctr[nbput_count].local_buf_addr = (uintptr_t)src;
  nb_put_ctr[nbput_count].local_buf = (uintptr_t)src;
#if use_ctr
//  memcpy(nb_put_ctr[nbput_count].IV, IV, AES_TAG_LEN);
#endif /*use_ctr*/

  nbput_count++;

}

void shmemx_secure_put(shmem_ctx_t ctx, void *dest, const void *src,
                       size_t nbytes, int pe) {

  size_t cipherlen = 0;

  double total_t1, total_t2, enc_t1, enc_t2, put_t1, put_t2, am_t1,
      am_t2, polling_t1, polling_t2; /* These last 2 would be for decrypting/encrypting */


  double put_set_t1, put_set_t2;
  total_t1 = shmemx_wtime();

   
  put_set_t1 = shmemx_wtime();
  shmemc_context_h ch = (shmemc_context_h)ctx;
  uint64_t r_dest;  /* address on other PE */
  ucp_rkey_h r_key; /* rkey for remote address */
  DEBUG_SHMEM("Getting rkey and addr\n");
  get_remote_key_and_addr(ch, (uint64_t)dest, pe, &r_key, &r_dest);
  ucp_ep_h peer_ep = lookup_ucp_ep(ch, pe);
  const ucp_request_param_t prm = {.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK,
      .cb.send = noop_callbackx};
  put_set_t2 = (shmemx_wtime() - put_set_t1) * 1e6;

  enc_t1 = shmemx_wtime();
  memset(blocking_put_ciphertext, 0, MAX_MSG_SIZE + OFFSET);

  DEBUG_SHMEM("bytes: %lu\n", nbytes);
  int segment_count = shmemx_encrypt_single_buffer_omp(
      &(blocking_put_ciphertext[0]), 0, src, 0, nbytes,
      ((size_t *)(&block_put_cipherlen)));
  enc_t2 = (shmemx_wtime() - enc_t1) * 1e6;

  DEBUG_SHMEM("Encryption end, ciphertext: %p, cipherlen: %d \n",
              &(blocking_put_ciphertext[0]), block_put_cipherlen);


  put_t1 = shmemx_wtime();

  ucs_status_ptr_t sp = ucp_put_nbx(peer_ep, blocking_put_ciphertext, block_put_cipherlen, r_dest, r_key, &prm);
  
  ucs_status_t st = check_wait_for_request(ch, sp);
  put_t2 = (shmemx_wtime() - put_t1) * 1e6;

  // DEBUG_SHMEM("local_buffer %p, remote_buffer %p\n", func_put->local_buffer,
  // r_dest);
  am_t1 = shmemx_wtime(); 
  
  func_args_t *func_put =
      (func_args_t *)malloc(sizeof(func_args_t));

  func_put->src_pe = proc.li.rank;
  func_put->dst_pe = pe;
  func_put->local_size = nbytes;
  int count =
      block_put_cipherlen + (segment_count * (AES_TAG_LEN + AES_RAND_BYTES));

#if use_gcm

  func_put->encrypted_size = count;
#elif use_ctr
  func_put->encrypted_size = block_put_cipherlen; //count;
//  memcpy(func_put->IV, IV, AES_TAG_LEN);

#endif /* use_gcm or use_ctr */
  func_put->remote_buffer = r_dest;
#if use_ctr
#endif /*use_ctr*/
  
  ucp_request_param_t param = {
      .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
      .cb.send = NULL,
      .datatype = ucp_dt_make_contig(sizeof(unsigned char)),
  };

  sp =
      ucp_am_send_nbx(peer_ep, AM_NBPUT_HANDLER, NULL, 0, func_put,
                      (sizeof(func_args_t)), &param);
//  shmemc_progress();
   st = check_wait_for_request(ch, sp);
  shmemu_assert(st == UCS_OK, "%s: put failed (status: %s)", __func__,
                ucs_status_string(st));

  am_t2 = (shmemx_wtime() - am_t1) * 1e6;
 
  polling_t1 = shmemx_wtime();

 // for (int k = 0; k < 30 ; k++){
 //  shmemc_progress();
 // }
  DEBUG_SHMEM("Put end\n");

  //goto fn_end;

     int k = 0;
     int magic = FOUR_M;
     int kilo = KILO;
     int magic2 = 1;
       if (nbytes < magic){
        while(k++ < magic2 * kilo )
           shmemc_progress();
     }else{
        while(k++ < magic2 * kilo * (nbytes*2/magic)) 
           shmemc_progress();
     }
   
fn_end:
       polling_t2 = (shmemx_wtime() - polling_t1) * 1e6;
       total_t2 = (shmemx_wtime() - total_t1) * 1e6;

     DEBUG_TIME("Msg sz: %d, total %.3f enc %.3f put_setup %.3f put %.3f AM %.3f polling %.3f\n",
             nbytes, total_t2, enc_t2, put_set_t2, put_t2, am_t2, polling_t2);   

}
void shmemx_secure_get_nbi(shmem_ctx_t ctx, void *dest, const void *src,
                           size_t nbytes, int pe) {

  size_t cipherlen = 0;

  shmemc_context_h ch = (shmemc_context_h)ctx;
  uint64_t r_dest;  /* address on other PE */
  ucp_rkey_h r_key; /* rkey for remote address */
  DEBUG_SHMEM("Getting rkey and addr\n");
  get_remote_key_and_addr(ch, (uint64_t)src, pe, &r_key, &r_dest);
  ucp_ep_h ep = NULL;

  func_args_t *func_get = malloc(sizeof(func_args_t));

  int thread_no = get_thread_count(nbytes); // Starting thread point
  int segment_count = 0;
  int data = nbytes / thread_no;
  // data++;
  DEBUG_SHMEM("bytes = %d  max_thread_no %d\n", data, thread_no);

  func_get->src_pe = pe;
  func_get->dst_pe = proc.li.rank;
  func_get->local_size = nbytes;
  func_get->encrypted_size = 0;
  func_get->remote_buffer = r_dest;
  func_get->get_rem_buf = src;
  func_get->local_buf = dest;
 
  unsigned long long counter_val = 0; //nbget_count % (segment_count + pe); //0xFF / (unsigned long long) nbget_count % (unsigned long long) nbget_count;



  ucp_request_param_t ack_param = {
     .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
      .cb.send = NULL,
      .datatype = ucp_dt_make_contig(sizeof(unsigned char))};

  ep = lookup_ucp_ep(defcp, pe);


  DEBUG_SHMEM("Starting Nonblocking get\n");
  ucs_status_ptr_t sp = ucp_am_send_nbx(ep, AM_GET_ENC_HANDLER, NULL, 0, func_get,
        sizeof(func_args_t), &ack_param);
  //shmemc_progress();
  ucs_status_t st = check_wait_for_request(ch, sp);
  shmemu_assert(st == UCS_OK, "%s: nb_get enc failed (status: %s)", __func__,
        ucs_status_string(st));


     int k = 0;
     int magic = FOUR_M;
     int kilo = KILO;
     int magic2 = 1;

     if (nbytes < magic){
        while(k++ < magic2 * kilo )
           shmemc_progress();
     }else{
        while(k++ < magic2 * kilo * (nbytes/magic)) 
           shmemc_progress();
     }

     shmemc_ctx_get_nbi(defcp, dest, src, nbytes, pe);
  
     DEBUG_SHMEM("NbGet end\n");

  nb_get_ctr[nbget_count].src_pe = pe;
  nb_get_ctr[nbget_count].dst_pe = proc.li.rank;
  nb_get_ctr[nbget_count].res_pe = pe;
  nb_get_ctr[nbget_count].plaintext_size = nbytes;
#if use_gcm
  nb_get_ctr[nbget_count].encrypted_size = nbytes + (segment_count * (AES_TAG_LEN + AES_RAND_BYTES)); 
#elif use_ctr
  nb_get_ctr[nbget_count].encrypted_size = nbytes;
//  memcpy(nb_get_ctr[nbget_count].IV, func_get->IV, AES_TAG_LEN);
#endif /* use_gcm ^ use_ctr */
  nb_get_ctr[nbget_count].local_buf_addr = (uintptr_t)dest;
  nb_get_ctr[nbget_count].local_buf = (uintptr_t)dest;
  nb_get_ctr[nbget_count].remote_buf_addr = r_dest;

  nbget_count++;

}

int shmemx_secure_quiet(void) {

  // int shmem_errno = 0;

  if (nbput_count > 0 || nbget_count > 0) {

     int ctr = 0;
     while (ctr < nbput_count) {
      shmem_secure_attr_t put_data = nb_put_ctr[ctr];
      func_args_t *func_put = malloc(sizeof(func_args_t));

      func_put->remote_buffer = put_data.remote_buf_addr;
      func_put->src_pe = proc.li.rank;
      func_put->dst_pe = put_data.res_pe; /* Will need this for peer EP calculation */
      func_put->encrypted_size = put_data.encrypted_size;
      func_put->local_size = put_data.plaintext_size;
#if use_ctr
//      memcpy(func_put->IV, nb_put_ctr[ctr].IV, AES_TAG_LEN);
#endif /*use_ctr*/

      ucp_request_param_t param = {
         .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
         .cb.send = NULL,
         .datatype = ucp_dt_make_contig(sizeof(unsigned char)),
      };
      ucp_ep_h peer_ep = lookup_ucp_ep(defcp, put_data.res_pe);

      ucs_status_ptr_t sp =
      ucp_am_send_nbx(peer_ep, AM_NBPUT_HANDLER, NULL, 0, func_put,
            (sizeof(func_args_t)), &param);
     
      ucs_status_t st = check_wait_for_request(defcp, sp);
      shmemu_assert(st == UCS_OK, "%s: put failed (status: %s)", __func__,
            ucs_status_string(st));
      for (int k = 0 ; k < 10; k++){
          shmemc_progress();
      }
    
      ctr++;
    }

    ctr = 0;
    nbput_count = 0;

    ucp_request_param_t ack_param = {
       .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
       .cb.send = NULL,
       .datatype = ucp_dt_make_contig(sizeof(unsigned char))};
    ucp_ep_h ep = NULL;
    func_args_t *nb_get = malloc(sizeof(func_args_t));
    ucs_status_t st;
    ucs_status_ptr_t sp;
   uint64_t r_dest;
   ucp_rkey_h r_key;

    while (ctr < nbget_count) {
      shmem_secure_attr_t get_data = nb_get_ctr[ctr];

      void *addr = get_data.local_buf;
      size_t local_size = get_data.plaintext_size;
      size_t enc_size = get_data.encrypted_size;

      DEBUG_SHMEM("Fulfilling 'non-blocking' get (iteration %d)\n", ctr);
 

      nb_get->src_pe = get_data.src_pe;
      nb_get->dst_pe = get_data.dst_pe;
      nb_get->local_size = local_size;
      nb_get->encrypted_size = enc_size;
      nb_get->remote_buffer = get_data.remote_buf_addr;
      nb_get->local_buf = addr;
#if use_ctr
//      memcpy(nb_get->IV, get_data.IV, AES_TAG_LEN);
#endif /* use_ctr */
      ep = lookup_ucp_ep(defcp, get_data.src_pe);
      DEBUG_SHMEM("Starting remote decryption via active messages\n");

      sp = ucp_am_send_nbx(ep, AM_NBPUT_HANDLER, NULL, 0, nb_get,
            sizeof(func_args_t), &ack_param);
      shmemc_progress();
      st = check_wait_for_request(defcp, sp);
      shmemu_assert(st == UCS_OK, "%s: nb_get failed (status: %s)", __func__,
            ucs_status_string(st));

      int k = 0;
      int magic = FOUR_M;
     int kilo = 512;
     int magic2 = 1;

     if (enc_size < magic){
        while(k++ < magic2 * kilo )
           shmemc_progress();
     }else{
        while(k++ < magic2 * kilo * (enc_size/magic)) 
           shmemc_progress();
     }

   
      DEBUG_SHMEM("Doing local_decryption on buffer %p with ciphertext %s\n", addr, (unsigned char*) addr);
      shmemx_decrypt_single_buffer_omp(addr, 0, addr, 0, local_size, enc_size);

      ctr++;
    }
    nbget_count = 0;
    free(nb_get);

    memset(nb_put_ctr, 0,
           (sizeof(shmem_secure_attr_t) * NON_BLOCKING_OP_COUNT * 2));
    memset(nb_get_ctr, 0,
           (sizeof(shmem_secure_attr_t) * NON_BLOCKING_OP_COUNT * 2));
  }

  return 0;
}

void shmemx_secure_get(shmem_ctx_t ctx, void *dest, const void *src,
                       size_t nbytes, int pe) {

   memset(blocking_get_ciphertext, 0, MAX_MSG_SIZE + OFFSET);
  size_t cipherlen = 0;
  double total_t1, total_t2, enc_t1, enc_t2, put_t1, put_t2, am1_t1,
      am1_t2, polling1_t1, polling1_t2, polling2_t1, polling2_t2, am2_t1, am2_t2; /* These last 2 would be for decrypting/encrypting */
  double get_set_t1, get_set_t2;

  total_t1 = shmemx_wtime();

  get_set_t1 = shmemx_wtime();
  shmemc_context_h ch = (shmemc_context_h)ctx;
  uint64_t r_src;
  ucp_rkey_h r_key;
  uint64_t l_rdest;
  ucp_rkey_h l_rkey;
  ucp_ep_h ep;
#if defined(HAVE_UCP_GET_NBX) || defined(HAVE_UCP_GET_NB)
  ucs_status_ptr_t sp;
#endif /* HAVE_UCP_GET_NBX || HAVE_UCP_GET_NB */
  ucs_status_t s;

  get_remote_key_and_addr(ch, (uint64_t)src, pe, &r_key, &r_src);
  get_remote_key_and_addr(ch, (uint64_t)dest, proc.li.rank, &l_rkey, &l_rdest);

  ep = lookup_ucp_ep(ch, pe);
  get_set_t2 = (shmemx_wtime() - get_set_t1) * 1e6;


  am1_t1 = shmemx_wtime();
  func_args_t *func_get = 
      (func_args_t *)malloc(sizeof(func_args_t));

  int max_offset = SIXTEEN_K;

  int segment_count = (nbytes/max_offset == 0 ? 1 : nbytes/max_offset);
  int seg_bytes = nbytes / segment_count;
  int seg_bytes_last = seg_bytes + (nbytes % max_offset);
  int remainder_bytes = nbytes % segment_count;

  DEBUG_SHMEM("nbytes %d Segment_count %d seg_bytes %d seg_bytes_last %d\n", nbytes, segment_count, seg_bytes, seg_bytes_last);
  func_get->segment_count = segment_count;
  func_get->remote_buffer = r_src;
  func_get->local_buf = dest;
  func_get->get_rem_buf = src;


  int i = 0;

#if use_gcm
  for (i = 0 ; i < segment_count ; i++){

      DEBUG_SHMEM("Sending segment %d\n", i);

      func_get->src_pe = pe;
      func_get->dst_pe = proc.li.rank;
      func_get->local_size = seg_bytes;
      func_get->offset_from_start = (i*seg_bytes);
      func_get->local_buf = dest + (i*seg_bytes);
      func_get->remote_buffer = r_src + (i*seg_bytes);
      // if ( i == segment_count -1){
      //    func_get->local_size = seg_bytes_last;
      // }
      func_get->encrypted_size = 0; // for now 

      ucp_request_param_t ack_param = {
          .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
          .cb.send = NULL,
          .datatype = ucp_dt_make_contig(sizeof(unsigned char))};
      DEBUG_SHMEM("Starting blocking get\n");
      sp = ucp_am_send_nbx(ep, AM_GET_ENC_HANDLER, NULL, 0, func_get,
              sizeof(func_args_t), &ack_param);
      ucs_status_t st = check_wait_for_request(ch, sp);
      shmemu_assert(st == UCS_OK, "%s: get enc failed (status: %s)", __func__,
              ucs_status_string(st));

      st = check_wait_for_request(ch, sp);
      shmemu_assert(st == UCS_OK, "%s: get enc failed (status: %s)", __func__,
              ucs_status_string(st));
      //     shmemc_progress();

      //     shmemc_quiet();
  }
#elif use_ctr
  DEBUG_SHMEM("dest ptr: %p\n", dest);
  func_get->src_pe = pe;
  func_get->dst_pe = proc.li.rank;
  func_get->local_size = nbytes;
  func_get->offset_from_start = 0;
  func_get->put_rem_buf = l_rdest;
  func_get->local_buf = dest;
  shmem_fence();
  ucp_request_param_t ack_param = {
      .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
      .cb.send = NULL,
      .datatype = ucp_dt_make_contig(sizeof(unsigned char))};
  DEBUG_SHMEM("Starting blocking get\n");
  am1_t1 = shmemx_wtime();
  sp = ucp_am_send_nbx(ep, AM_GET_ENC_HANDLER, NULL, 0, func_get,
          sizeof(func_args_t), &ack_param);
  shmemc_progress();
  ucs_status_t st = check_wait_for_request(ch, sp);
 
  shmemu_assert(st == UCS_OK, "%s: get enc failed (status: %s)", __func__,
          ucs_status_string(st));
  am1_t2 = (shmemx_wtime() - am1_t1) * 1e6;


     polling1_t1 = shmemx_wtime();
     int k = 0;
     int magic = FOUR_M;
     int kilo = KILO;
     int magic2 = 1;

     if (nbytes < magic){
        while(k++ < magic2 * kilo )
           shmemc_progress();
     }else{
        while(k++ < magic2 * kilo * (nbytes/magic)) 
           shmemc_progress();
     }
     polling1_t2 = (shmemx_wtime() - polling1_t1) * 1e6;
   //  shmem_fence();

     put_t1 = shmemx_wtime();
    shmemc_ctx_get(defcp, dest, src, nbytes, pe);
    put_t2 = (shmemx_wtime() - put_t1) * 1e6;

    enc_t1 = shmemx_wtime();
    shmemx_decrypt_single_buffer_omp((unsigned char *)dest, 0, (void *)dest, 0,
          nbytes,
          ((size_t)(func_get->encrypted_size)));
    enc_t2 = (shmemx_wtime() - enc_t1) * 1e6;

    ucp_request_param_t ack_param2 = {
       .op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE,
       .cb.send = NULL,
       .datatype = ucp_dt_make_contig(sizeof(unsigned char))};
    DEBUG_SHMEM("Starting blocking get\n");
    am2_t1 = shmemx_wtime();
    sp = ucp_am_send_nbx(ep, AM_GET_DEC_RESPONSE_2, NULL, 0, func_get,
          sizeof(func_args_t), &ack_param2);
       st = check_wait_for_request(ch, sp);
     shmemu_assert(st == UCS_OK, "%s: get enc failed (status: %s)", __func__,
           ucs_status_string(st));
     am2_t2 = (shmemx_wtime() - am2_t1) * 1e6;
     polling2_t1 = shmemx_wtime();
      for (int i = 0; i < 20; i ++){
         shmemc_progress();
      }
      polling2_t2 = (shmemx_wtime() - polling2_t1) * 1e6;
      total_t2 = (shmemx_wtime() - total_t1) *1e6;
      DEBUG_TIME("Msg sz %d total %.3f get %.3f am1 %.3f am2 %.3f polling1 %.3f polling2 %.3f dec %.3f\n",
              nbytes, total_t2, put_t2, am1_t2, am2_t2, polling1_t2, polling2_t2, enc_t2);


#endif /* use_gcm ^ use_ctr */
 
    DEBUG_SHMEM("Get end\n");
    DEBUG_SHMEM("Final plaintext: %s\n", (char *)dest);



}

#endif /* ENABLE_SHMEM_ENCRYPTION */
