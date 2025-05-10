/**
 * @file memalloc.c
 * @brief Implementation of memory allocation functions
 *
 * For license: see LICENSE file at top-level
 */

#include "internal-malloc.h"

#include "memalloc.h"

/**
 * @brief The memory area managed by this unit
 *
 * This static variable holds the memory space handle used for all allocations.
 * Not visible outside this compilation unit.
 */
static mspace myspace;

/**
 * @brief Initialize the memory pool
 *
 * @param base Base address for the memory pool
 * @param capacity Size of the memory pool in bytes
 *
 * Creates a new memory space at the specified base address with the given
 * capacity. The space is created with thread safety enabled.
 */
void shmema_init(void *base, size_t capacity) {
  myspace = create_mspace_with_base(base, capacity, 1);
}

/**
 * @brief Clean up and destroy the memory pool
 *
 * Releases all resources associated with the memory space.
 */
void shmema_finalize(void) { destroy_mspace(myspace); }

/**
 * @brief Get the base address of the memory pool
 *
 * @return Pointer to the start of the memory pool
 */
void *shmema_base(void) { return myspace; }

/**
 * @brief Allocate memory from the pool
 *
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory or NULL if allocation fails
 */
void *shmema_malloc(size_t size) { return mspace_malloc(myspace, size); }

/**
 * @brief Allocate and zero-initialize memory from the pool
 *
 * @param count Number of elements to allocate
 * @param size Size of each element in bytes
 * @return Pointer to allocated memory or NULL if allocation fails
 */
void *shmema_calloc(size_t count, size_t size) {
  return mspace_calloc(myspace, count, size);
}

/**
 * @brief Free previously allocated memory
 *
 * @param addr Address of memory to free
 */
void shmema_free(void *addr) { mspace_free(myspace, addr); }

/**
 * @brief Resize a previously allocated memory block
 *
 * @param addr Address of memory to resize
 * @param new_size New size in bytes
 * @return Pointer to resized memory or NULL if reallocation fails
 */
void *shmema_realloc(void *addr, size_t new_size) {
  return mspace_realloc(myspace, addr, new_size);
}

/**
 * @brief Allocate aligned memory from the pool
 *
 * @param alignment Required alignment in bytes (must be power of 2)
 * @param size Number of bytes to allocate
 * @return Pointer to aligned memory or NULL if allocation fails
 */
void *shmema_align(size_t alignment, size_t size) {
  return mspace_memalign(myspace, alignment, size);
}
