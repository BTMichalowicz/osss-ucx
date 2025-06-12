#if ENABLE_SHMEM_ENCRYPTION


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


#include "shmem_enc.h"
#include "shmemx.h"
#include "shmemu.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


const unsigned char gcm_key[GCM_KEY_SIZE] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
unsigned char blocking_put_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char recv_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char nbi_put_ciphertext[NON_BLOCKING_OP_COUNT][NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbput_count = 0;

unsigned char blocking_get_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char nbi_get_ciphertext[NON_BLOCKING_OP_COUNT][NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbget_count = 0;

static inline void handleErrors(char *message){
    ERR_print_errors_fp(stderr);
    shmemu_fatal("shmem_enc_dec: %s\n", message);
}

void shmemx_sec_init(){
    char *enc_dec = NULL;
    int res = 0;
    if (defcp->enc_ctx == NULL){
        if (!(defcp->enc_ctx = EVP_CIPHER_CTX_new())){
            handleErrors("cipher failed to be created");
        }
        /* Begin using AES_256_gcm */
        res = EVP_EncryptInit_ex2(defcp->enc_ctx, EVP_aes_256_gcm(), NULL, gcm_key, NULL);
        if (res != 1){
            handleErrors("failed to begin encryption portion");
        }

        res = EVP_CIPHER_CTX_ctrl(defcp->enc_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
        if (res != 1){
            handleErrors("Failed to set up the Initialization Vector Length");
        }
    }

    if (defcp->dec_ctx == NULL){
        if (!(defcp->dec_ctx = EVP_CIPHER_CTX_new())){
            handleErrors("cipher failed to be created");
        }
        /* Begin using AES_256_gcm */
        res = EVP_DecryptInit_ex2(defcp->dec_ctx, EVP_aes_256_gcm(), NULL, gcm_key, NULL);
        if (res != 1){
            handleErrors("failed to begin Decryption portion");
        }

        res = EVP_CIPHER_CTX_ctrl(defcp->dec_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
        if (res != 1){
            handleErrors("Failed to set up the Initialization Vector Length");
        }
    }

    if ((enc_dec = getenv("SHMEM_ENABLE_ENCRYPTION")) != NULL){
        proc.env.shmem_encryption = atoi(enc_dec);
        assert(proc.env.shmem_encryption == 0 || proc.env.shmem_encryption == 1);
    }


    return;
}

int shmemx_encrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, 
        const void *sbuf, unsigned long long dest, size_t bytes,
        shmem_ctx_t shmem_ctx, size_t *cipher_len){
    
    shmemc_context_h c2 = (shmemc_context_h)shmem_ctx;
    int const_bytes = AES_RAND_BYTES;
    RAND_bytes(cipherbuf+src, const_bytes);
    EVP_EncryptInit_ex2(c2->enc_ctx, NULL, NULL, NULL, cipherbuf+src);

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
    shmemx_encrypt_single_buffer(
            ((unsigned char *)&(nbi_put_ciphertext[nbput_count][0])),
            0, src, 0, nbytes, ctx, &cipherlen);

  //  shmem_secure_attr_t attr = {}
  //  attr.src_pe = proc.li.rank;
  //  attr.dst_pe = pe;
  //  attr.plaintext_size = nbytes;
  // attr.encrypted_size = cipherlen;
  //  attr.plaintext_buf_addr = (uintptr_t) &src;
  //  attr.encrypted_buf_addr = (uintptr_t) (&(nbi_put_ciphertext[nbput_count][0]));
  //  attr.encrypted_op = NB_PUT;

    shmemc_ctx_put_nbi(ctx, dest, 
            ((unsigned char*)(&(nbi_put_ciphertext[nbput_count][0]))),
            cipherlen, pe);

    /* TODO: SIGNAL DECRYPTION IN WAIT!! */
}

void shmemx_secure_put(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

    size_t cipherlen = 0;
    int res = shmemx_encrypt_single_buffer(
            (blocking_put_ciphertext),
            0, src, 0, nbytes, ctx, &cipherlen);


 //   shmem_secure_attr_t attr = {}
 //   attr.src_pe = proc.li.rank;
 //   attr.dst_pe = pe;
 //   attr.plaintext_size = nbytes;
 //   attr.encrypted_size = cipherlen;
 //   attr.plaintext_buf_addr = (uintptr_t) &src;
 //   attr.encrypted_buf_addr = (uintptr_t) (&blocking_put_ciphertext);
 //   attr.encrypted_op = PUT;


    shmemc_ctx_put(ctx, dest, 
            blocking_put_ciphertext,
            cipherlen, pe);

// TODO    ucp_tag_send_sync_nbx(...);

    /*TODO: SIGNAL DECRYPTION here for the blocking_put_ciphertext... but
     * how?!?! 
     * Look at shmem context?
     * See address pointers and extra functions... though we'd need transport
     * layer items there to catch messages to say
     * "Pick this path to decrypt?" 
     *
     */ 
}



void shmemx_secure_get_nbi(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

    size_t cipherlen = 0;

    /* TODO: HOW TO GET BUFFER APPROPRIATELY FROM DEST SIDE FIXME  */
    /* signal_dest_encrypt ();  */

   // int res = shmemx_encrypt_single_buffer(
   //         ((unsigned char *)&(nbi_get_ciphertext[nbget_count][0])),
  //          0, src, 0, nbytes, ctx, &cipherlen);

    shmemc_ctx_get_nbi(ctx, dest, 
            ((unsigned char *)&(nbi_get_ciphertext[nbget_count++][0])),
            cipherlen, pe);

    /*TODO: SIGNAL DECRYPTION IN WAIT!! */

}


void shmemx_secure_get(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe){

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

}


#endif /* ENABLE_SHMEM_ENCRYPTION */
