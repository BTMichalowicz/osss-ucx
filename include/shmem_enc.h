#ifndef _SHMEM_ENC_H
#define _SHMEM_ENC_H_ 1

#if ENABLE_SHMEM_ENCRYPTION
#include<openssl/ssl.h>
#include<openssl/sha.h>
#include<openssl/conf.h>
#include<openssl/evp.h>
#include<openssl/err.h>
#include<openssl/rand.h>
#include<openssl/modes.h>
#include<openssl/core_names.h>
#include<openssl/provider.h>

#endif /* ENABLE_SHMEM_ENCRYPTION */
#endif /* _SHMEM_ENC_H_ */
