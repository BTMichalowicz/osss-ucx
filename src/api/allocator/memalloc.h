/**
 * @file memalloc.h
 * @brief Header for memory allocation functions
 *
 * For license: see LICENSE file at top-level
 */

#ifndef _SHMEMA_MEMALLOC_H
#define _SHMEMA_MEMALLOC_H 1

#include <sys/types.h> /* size_t */

/*
 * memory allocation
 */

/**
 * @brief Initialize the memory allocator with a base address and capacity
 * @param base Base address for the memory pool
 * @param capacity Size of the memory pool in bytes
 */
void shmema_init(void *base, size_t capacity);

/**
 * @brief Clean up and finalize the memory allocator
 */
void shmema_finalize(void);

/**
 * @brief Get the base address of the memory pool
 * @return Base address of the memory pool
 */
void *shmema_base(void);

/**
 * @brief Allocate memory from the pool
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory or NULL if allocation fails
 */
void *shmema_malloc(size_t size);

/**
 * @brief Allocate and zero-initialize memory from the pool
 * @param count Number of elements to allocate
 * @param size Size of each element in bytes
 * @return Pointer to allocated memory or NULL if allocation fails
 */
void *shmema_calloc(size_t count, size_t size);

/**
 * @brief Free previously allocated memory
 * @param addr Address of memory to free
 */
void shmema_free(void *addr);

/**
 * @brief Resize a previously allocated memory block
 * @param addr Address of memory to resize
 * @param new_size New size in bytes
 * @return Pointer to resized memory or NULL if reallocation fails
 */
void *shmema_realloc(void *addr, size_t new_size);

/**
 * @brief Allocate aligned memory from the pool
 * @param alignment Required alignment in bytes (must be power of 2)
 * @param size Number of bytes to allocate
 * @return Pointer to aligned memory or NULL if allocation fails
 */
void *shmema_align(size_t alignment, size_t size);

#endif /* ! _SHMEMA_MEMALLOC_H */
