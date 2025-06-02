#if ENABLE_SHMEM_ENCRYPTION


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


#include "shmemc.h"
#include "shmem_enc.h"
#include "shmemx.h"
#include "shmemu.h"
#include <string.h>

const unsigned char gcm_key[GCM_KEY_SIZE] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
/* unsigned char blocking_put_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char recv_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char nbi_put_ciphertext[NON_BLOCKING_OP_COUNT][NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbput_count = 0;*/


/*unsigned char blocking_get_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char recv_ciphertext[MAX_MSG_SIZE+OFFSET] = {'\0'};
unsigned char nbi_get_ciphertext[NON_BLOCKING_OP_COUNT][NON_BLOCKING_OP_COUNT][MAX_MSG_SIZE+OFFSET];
unsigned long long nbget_count = 0;*/




static inline void handleErrors(char *message){
    ERR_print_errors_fp(stderr);
    shmemu_fatal("shmem_enc_dec: %s\n", message);
}

void shmemx_sec_init(){
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
        res = EVP_DencryptInit_ex2(defcp->dec_ctx, EVP_aes_256_gcm(), NULL, gcm_key, NULL);
        if (res != 1){
            handleErrors("failed to begin Decryption portion");
        }

        res = EVP_CIPHER_CTX_ctrl(defcp->dec_ctx, EVP_CTRL_GCM_SET_IVLEN, (int)AES_RAND_BYTES, NULL);
        if (res != 1){
            handleErrors("Failed to set up the Initialization Vector Length");
        }
    }
    return;
}

int shmemx_encrypt_single_buffer(unsigned char *cipherbuf unsigned long long src, 
        const void *sbuf, unsigned long long dest, size_t bytes,
        shmem_ctx_t shmem_ctx, size_t *cipher_len){

    int const_bytes = AES_RAND_BYTES;
    int res = 0;
    RAND_bytes(cipherbuf+src, const_bytes);
    EVP_EncryptInit_ex2(shmem_ctx->enc_ctx, NULL, NULL, NULL, cipherbuf+src);

    EVP_EncryptUpdate(shmem_ctx->enc_ctx,cipherbuf+src+const_bytes,cipher_len, sbuf+dest, bytes);

    EVP_EncryptFinal_ex2(shmem_ctx->enc_ctx, cipherbuf+const_bytes+src+(*cipher_len), cipher_len);

    EVP_CIPHER_CTX_ctrl(shmem_ctx->ctx_enc, EVL_CTRL_GCM_GET_TAG, AES_TAG_LEN, cipherbuf+const_bytes+src+bytes);

    return res;
}

int shmemx_decrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, 
        const void *rbuf, unsigned long long dest, size_t bytes, 
        shmem_ctx_t shmem_ctx, size_t  *cipher_len){

    EVP_DecryptInit_ex2(shmem_ctx->dec_ctx, NULL, NULL, NULL, cipherbuf+src);
    EVP_DecryptUpdate(shmem_ctx->dec_ctx, rbuf+dest, cipher_len, cipherbuf+src+AES_RAND_BYTES, (bytes-AES_RAND_BYTES));
    EVP_CIPHER_CTX_ctrl(shmem_ctx->dec_ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_LEN, (rbuf+dest+(*cipher_len)));
    if (!(EVP_DecryptFinal_ex2(shmem_ctx->dec_ctx, (rbuf+dest+(*cipher_len)), cipher_len) > 0)){
        handleErrors("Decryption Tag Verification Failed\n");
    }
    return 0;
}

#endif /* ENABLE_SHMEM_ENCRYPTION */
