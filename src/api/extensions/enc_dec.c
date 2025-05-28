#if ENABLE_SHMEM_ENCRYPTION


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


#include "shmemc.h"
#include "shmem_enc.h"
#include "shmemx.h"
#include "shmemu.h"

const unsigned char gcm_key[GCM_KEY_SIZE] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
unsigned char send_ciphertext[MAX_MSG_SIZE+OFFSET];



static inline void handleErrors(char *message){
    ERR_print_errors_fp(stderr);
    shmemu_fatal("shmem_enc_dec: %s\n", message);
}

int shmemx_encrypt_single_buffer(int src_pe, int dst_pe, void **src, void **enc_src,
        size_t plaintext_size, unsigned char *nonce, unsigned char *key,
        shmemc_context_t *shmem_ctx){

    int res = 0;
    //unisigned char *initVector = (unsigned char*)"0"; 
    /*TODO: BEST WAY TO DO THIS FOR CONSISTENT VECTORS -- place it in the struct for each time??? */

    if (shmem_ctx->cipher_ctx == NULL){
        if (!(shmem_ctx->cipher_ctx = EVP_CIPHER_CTX_new())){
            handleErrors("cipher failed to be created");
        }
    }

    shmem_ctx->nonceCounter++;


    /* Begin using AES_256_gcm */
    res = EVP_EncryptInit_ex(shmem_ctx->cipher_ctx, EVP_aes_256_gcm(), NULL, NULL, send_ciphertext+0);
    if (res != 1){
        handleErrors("failed to begin encryption portion\n");
    }



    EVP_CIPHER_CTX_free(shmem_ctx->cipher_ctx);
    shmem_ctx->cipher_ctx = NULL;

    return 0;
}
    

#endif /* ENABLE_SHMEM_ENCRYPTION */
