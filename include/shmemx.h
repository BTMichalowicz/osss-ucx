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

int shmemx_encrypt_single_buffer(int src_pe, int dst_pe, void **src, void **enc_src, unsigned char *nonce, unsigned char *key,  shmemc_ctx_t *shmem_ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SHMEMX_H */
