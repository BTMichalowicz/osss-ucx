/**
 * @file internal-malloc.h
 * @brief Internal header for dlmalloc allocator
 *
 * For license: see LICENSE file at top-level
 */

#ifndef _DLMALLOC_H
#define _DLMALLOC_H 1

#include <sys/types.h>

/**
 * @brief Memory space handle type for dlmalloc allocator
 */
typedef void *mspace;

/**
 * @brief Creates a new memory space with a given base address
 * @param base Base address for the memory space
 * @param capacity Size of the memory space in bytes
 * @param locked Whether the space should be thread-safe
 * @return Handle to the created memory space
 */
extern mspace create_mspace_with_base(void *base, size_t capacity, int locked);

/**
 * @brief Destroys a memory space and frees its resources
 * @param msp Memory space handle
 * @return Size of space that was destroyed
 */
extern size_t destroy_mspace(mspace msp);

/**
 * @brief Allocates memory from a memory space
 * @param msp Memory space handle
 * @param bytes Number of bytes to allocate
 * @return Pointer to allocated memory or NULL if allocation fails
 */
extern void *mspace_malloc(mspace msp, size_t bytes);

/**
 * @brief Allocates zeroed memory from a memory space
 * @param msp Memory space handle
 * @param count Number of elements
 * @param bytes Size of each element in bytes
 * @return Pointer to allocated memory or NULL if allocation fails
 */
extern void *mspace_calloc(mspace msp, size_t count, size_t bytes);

/**
 * @brief Reallocates memory from a memory space
 * @param msp Memory space handle
 * @param mem Pointer to previously allocated memory
 * @param newsize New size in bytes
 * @return Pointer to reallocated memory or NULL if reallocation fails
 */
extern void *mspace_realloc(mspace msp, void *mem, size_t newsize);

/**
 * @brief Allocates aligned memory from a memory space
 * @param msp Memory space handle
 * @param alignment Required alignment in bytes (must be power of 2)
 * @param bytes Number of bytes to allocate
 * @return Pointer to allocated memory or NULL if allocation fails
 */
extern void *mspace_memalign(mspace msp, size_t alignment, size_t bytes);

/**
 * @brief Frees memory allocated from a memory space
 * @param msp Memory space handle
 * @param mem Pointer to memory to free
 */
extern void mspace_free(mspace msp, void *mem);

/**
 * @brief Gets the current memory footprint of a memory space
 * @param msp Memory space handle
 * @return Current size in bytes of allocated memory
 */
extern size_t mspace_footprint(mspace msp);

#endif /* ! _DLMALLOC_H */
