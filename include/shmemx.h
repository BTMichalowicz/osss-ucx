/**
 * @file shmemx.h
 * @brief OpenSHMEM experimental extensions header file
 *
 * This header file contains experimental extensions to the OpenSHMEM API that
 * are not part of the formal specification. These extensions provide additional
 * functionality that may be useful for certain applications.
 *
 * @note For license information, see LICENSE file at top-level
 */

#ifndef _SHMEMX_H
#define _SHMEMX_H 1

#include <shmem.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup shmemx_experimental OpenSHMEM Experimental Extensions
 * @brief Experimental extensions to the OpenSHMEM API
 * @{
 */

/**
 * @defgroup shmemx_wallclock Wallclock Time Functions
 * @brief Functions for measuring wallclock time
 * @{
 */

/**
 * @brief Returns the number of seconds since the program started running
 *
 * @return Returns the number of seconds since program start (epoch)
 *
 * @note shmemx_wtime does not indicate any error code; if it is unable to
 * detect the elapsed time, the return value is undefined. The time may be
 * different on each PE, but the epoch from which the time is measured will
 * not change while OpenSHMEM is active.
 */
double shmemx_wtime(void);

/** @} */

/**
 * @defgroup shmemx_addr_trans Address Translation Functions
 * @brief Functions for symmetric address translation
 * @{
 */

/**
 * @brief Returns the symmetric address on another PE corresponding to the
 * symmetric address on this PE
 *
 * @param addr Local symmetric address
 * @param pe PE number to translate address for
 * @return Returns the address corresponding to "addr" on PE "pe"
 */
void *shmemx_lookup_remote_addr(void *addr, int pe);

/** @} */

/**
 * @defgroup shmemx_fence_quiet Non-blocking Fence/Quiet Functions
 * @brief Functions for testing fence and quiet completion
 * @{
 */

/**
 * @brief Check whether all communication operations issued prior to this call
 * have satisfied the fence semantic
 *
 * @param ctx Context on which to test fence completion
 * @return Non-zero if fence semantic satisfied, 0 otherwise
 */
int shmemx_ctx_fence_test(shmem_ctx_t ctx);

/**
 * @brief Check whether all communication operations issued prior to this call
 * have satisfied the quiet semantic
 *
 * @param ctx Context on which to test quiet completion
 * @return Non-zero if quiet semantic satisfied, 0 otherwise
 */
int shmemx_ctx_quiet_test(shmem_ctx_t ctx);

/**
 * @brief Check whether all communication operations issued prior to this call
 * have satisfied the fence semantic on the default context
 *
 * @return Non-zero if fence semantic satisfied, 0 otherwise
 */
int shmemx_fence_test(void);

/**
 * @brief Check whether all communication operations issued prior to this call
 * have satisfied the quiet semantic on the default context
 *
 * @return Non-zero if quiet semantic satisfied, 0 otherwise
 */
int shmemx_quiet_test(void);

/** @} */

/**
 * @defgroup shmemx_ctx_session Context Session Management
 * @brief Functions for managing context sessions
 * @{
 */

/**
 * @brief Start a context session
 * @param ctx Context to start session for
 */
void shmemx_ctx_session_start(shmem_ctx_t ctx);

/**
 * @brief Stop a context session
 * @param ctx Context to stop session for
 */
void shmemx_ctx_session_stop(shmem_ctx_t ctx);

/** @} */

/**
 * @defgroup shmemx_heap Multiple Symmetric Heap Support
 * @brief Functions for managing multiple symmetric heaps
 * @{
 */

/** @brief Type for symmetric heap indices */
typedef int shmemx_heap_index_t;

/**
 * @brief Convert a heap name to an index
 * @param name Name of the heap
 * @return Index corresponding to the named heap
 */
shmemx_heap_index_t shmemx_name_to_index(const char *name);

/**
 * @brief Convert a heap index to a name
 * @param index Index of the heap
 * @return Name corresponding to the heap index
 */
const char *shmemx_index_to_name(shmemx_heap_index_t index);

/**
 * @brief Allocate memory from a specific heap by index
 * @param index Index of heap to allocate from
 * @param s Size in bytes to allocate
 * @return Pointer to allocated memory
 */
void *shmemx_malloc_by_index(shmemx_heap_index_t index, size_t s);

/**
 * @brief Allocate and zero memory from a specific heap by index
 * @param index Index of heap to allocate from
 * @param n Number of elements
 * @param s Size of each element
 * @return Pointer to allocated memory
 */
void *shmemx_calloc_by_index(shmemx_heap_index_t index, size_t n, size_t s);

/**
 * @brief Free memory from a specific heap by index
 * @param index Index of heap containing memory
 * @param p Pointer to memory to free
 */
void shmemx_free_by_index(shmemx_heap_index_t index, void *p);

/**
 * @brief Reallocate memory from a specific heap by index
 * @param index Index of heap containing memory
 * @param p Pointer to memory to reallocate
 * @param s New size in bytes
 * @return Pointer to reallocated memory
 */
void *shmemx_realloc_by_index(shmemx_heap_index_t index, void *p, size_t s);

/**
 * @brief Allocate aligned memory from a specific heap by index
 * @param index Index of heap to allocate from
 * @param a Alignment in bytes
 * @param s Size in bytes to allocate
 * @return Pointer to allocated memory
 */
void *shmemx_align_by_index(shmemx_heap_index_t index, size_t a, size_t s);

/**
 * @brief Allocate memory from a specific heap by name
 * @param name Name of heap to allocate from
 * @param s Size in bytes to allocate
 * @return Pointer to allocated memory
 */
void *shmemx_malloc_by_name(const char *name, size_t s);

/**
 * @brief Allocate and zero memory from a specific heap by name
 * @param name Name of heap to allocate from
 * @param n Number of elements
 * @param s Size of each element
 * @return Pointer to allocated memory
 */
void *shmemx_calloc_by_name(const char *name, size_t n, size_t s);

/**
 * @brief Free memory from a specific heap by name
 * @param name Name of heap containing memory
 * @param p Pointer to memory to free
 */
void shmemx_free_by_name(const char *name, void *p);

/**
 * @brief Reallocate memory from a specific heap by name
 * @param name Name of heap containing memory
 * @param p Pointer to memory to reallocate
 * @param s New size in bytes
 * @return Pointer to reallocated memory
 */
void *shmemx_realloc_by_name(const char *name, void *p, size_t s);

/**
 * @brief Allocate aligned memory from a specific heap by name
 * @param name Name of heap to allocate from
 * @param a Alignment in bytes
 * @param s Size in bytes to allocate
 * @return Pointer to allocated memory
 */
void *shmemx_align_by_name(const char *name, size_t a, size_t s);

#if SHMEM_HAS_C11

/**
 * @brief Generic malloc that works with both heap index and name
 */
#define shmemx_malloc(_arg1, _s)                                               \
  _Generic(_arg1,                                                              \
      shmemx_heap_index_t: shmemx_malloc_by_index,                             \
      char *: shmemx_malloc_by_name)(_arg1, _s)

/**
 * @brief Generic calloc that works with both heap index and name
 */
#define shmemx_calloc(_arg1, _n, _s)                                           \
  _Generic(_arg1,                                                              \
      shmemx_heap_index_t: shmemx_calloc_by_index,                             \
      char *: shmemx_calloc_by_name)(_arg1, _n, _s)

/**
 * @brief Generic free that works with both heap index and name
 */
#define shmemx_free(_arg1, _p)                                                 \
  _Generic(_arg1,                                                              \
      shmemx_heap_index_t: shmemx_free_by_index,                               \
      char *: shmemx_free_by_name)(_arg1, _p)

/**
 * @brief Generic realloc that works with both heap index and name
 */
#define shmemx_realloc(_arg1, _p, _s)                                          \
  _Generic(_arg1,                                                              \
      shmemx_heap_index_t: shmemx_realloc_by_index,                            \
      char *: shmemx_realloc_by_name)(_arg1, _p, _s)

/**
 * @brief Generic align that works with both heap index and name
 */
#define shmemx_align(_arg1, _a, _s)                                            \
  _Generic(_arg1,                                                              \
      shmemx_heap_index_t: shmemx_align_by_index,                              \
      char *: shmemx_align_by_name)(_arg1, _a, _s)

#endif /* SHMEM_HAS_C11 */

/** @} */

/**
 * @defgroup shmemx_interop Interoperability Support
 * @brief Functions for querying interoperability with other programming models
 * @{
 */

/**
 * @brief Interoperability properties that can be queried
 */
enum interoperability {
  UPC_THREADS_ARE_PES = 0, /**< UPC threads map to PEs */
  MPI_PROCESSES_ARE_PES,   /**< MPI processes map to PEs */
  SHMEM_INITIALIZES_MPI,   /**< SHMEM initializes MPI */
  MPI_INITIALIZES_SHMEM    /**< MPI initializes SHMEM */
};

/**
 * @brief Query interoperability properties
 * @param property Property to query from interoperability enum
 * @return Non-zero if property is supported, 0 otherwise
 */
int shmemx_query_interoperability(int property);

/** @} */

/** @} */


/* BEGINNING SHMEM ENCRYPTION ADDITIONS: BTM */
#if ENABLE_SHMEM_ENCRYPTION
#include <pmix.h>
#define KILO 1024
#define MEGA KILO*KILO
#define GIGA KILO*MGEA

/* Constant items */
#define MAX_MSG_SIZE (4*MEGA)
//const unsigned long long max_msg_size = 2<<27ul;
#define OFFSET 400
//const unsigned long long pt2pt_size = max_msg_size + OFFSET;
#define COLL_OFFSET 400
#define GCM_KEY_SIZE 32
#define AES_TAG_LEN 16
#define AES_RAND_BYTES 12
#define NON_BLOCKING_OP_COUNT 450
#define PUT_TEMP_BUF_LEN 20

#define MAX_THREAD_COUNT 16

#define THIRTY_TWO_K 32*KILO
#define SIX_FOUR_K 64 * KILO
#define ONE_TWO_EIGHT_K 128 * KILO
#define TWO_FIVE_SIX_K 256 * KILO
#define FIVE_TWELVE_K 512 * KILO
#define ONE_M MEGA
#define TWO_M 2 * MEGA
#define FOUR_M 4 * MEGA
#define PIPELINE_SIZE FIVE_TWELVE_K




/**
 * @brief Initializes the default contexts for encryption based on the default ctx
 * @return void return type, or crash
 */

void shmemx_sec_init(void);
void shmemx_sec_ctx_init(shmem_ctx_t shmem_ctx);

int shmemx_encrypt_single_buffer_omp(unsigned char *cipherbuf, unsigned long long src,
      const void *sbuf, unsigned long long dest, size_t bytes, size_t *cipherlen);
int shmemx_decrypt_single_buffer_omp(unsigned char *cipherbuf, unsigned long long src, 
        void *rbuf, unsigned long long dest, size_t bytes, size_t cipher_len);

/** @brief A structure for metadata for commanding the peer process do perform
 * encryption or decryption
 */


/* TODO: Do we send the below in the encrypted section? alongside buffer
 * addresses and the like?  */
/* ANSWER: Yes, for non-blocking put/get */

typedef struct shmem_secure_attr {
    int src_pe;
    int dst_pe;
    int res_pe; /* Hack for non-blocking operations */
    size_t plaintext_size;
    size_t encrypted_size;
    uintptr_t remote_buf_addr;
    uintptr_t local_buf_addr;
    uintptr_t local_buf;
} shmem_secure_attr_t;


#define AM_PUT_HANDLER 101
#define AM_GET_ENC_HANDLER 102
#define AM_GET_ENC_RESPONSE 103
#define AM_GET_DEC_HANDLER 104
#define AM_GET_DEC_RESPONSE 105

typedef enum PUT_GET_COLLECTIVE {
    PT2PT = 1,
    COLL = 2
} op_type_t;

typedef struct func_args {
    op_type_t optype;
    int src_pe,
        dst_pe;
    int local_size;
    int encrypted_size;
    uint64_t remote_buffer; /* For get and put operations */
    void *local_buffer; /* for get operations */
} func_args_t;


/**
 * @brief Encrypt single-pe-put/get buffers on the user side before sending them
 * across the network or through intra-node shared memory. Uses GCM
 * (galois_counter mode)
 * @param src_pe PE of source
 * @param dst_pe PE of dst
 * @param src address of src buffer
 * @param enc_src address of encrypted src buffer
 * @param key encryption key 
 * @param shmem_ctx OSHMEM ctx -- modified to contain ciphertext ctx
 * @return length of ciphertext
 */


int shmemx_encrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, const void *sbuf, unsigned long long dest, size_t bytes, size_t *cipher_len);

int shmemx_decrypt_single_buffer(unsigned char *cipherbuf, unsigned long long src, void *rbuf, unsigned long long dest, size_t bytes, size_t cipher_len);


int shmemx_secure_put_omp_threaded(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe);

int shmemx_secure_quiet(void);

void shmemx_secure_put(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe);

void shmemx_secure_get(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe);

void shmemx_secure_put_nbi(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe);

void shmemx_secure_get_nbi(shmem_ctx_t ctx, void *dest, const void *src,
        size_t nbytes, int pe);

extern pmix_proc_t *my_second_pmix;

#define PROC_ENC_DEC_FENCE_COUNT 2

#endif /* ENABLE_SHMEM_ENCRYPTION */
#if 0
#define DEBUG_SHMEM(fmt, args...)                       \
   do {                                                 \
      fflush(stdout);                                   \
      fflush(stderr);                                   \
      fprintf(stdout, "[rank_%d][%s:%d][%s] "fmt,       \
            proc.li.rank, __FILE__, __LINE__, __func__, \
            ##args);                                    \
      fflush(stdout);                                   \
      fflush(stderr);                                   \
   } while(0);
#else
#define DEBUG_SHMEM(...)
#endif /* 1/0 for DEBUG PRINTS */
#define ERROR_SHMEM(fmt, args...)                       \
   do {                                                 \
     /* fflush(stdout);                                   \
      fflush(stderr);                               */    \
      fprintf(stderr, "[rank_%d][%s:%d][%s][ERROR] "fmt,       \
            proc.li.rank, __FILE__, __LINE__, __func__, \
            ##args);                                    \
      /*fflush(stderr);                                   \
      fflush(stdout);                              */     \
   } while(0);




/* ENDING SHMEM ENCRYPTION ADDITIONS: BTM */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SHMEMX_H */
