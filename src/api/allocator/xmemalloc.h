/**
 * @file xmemalloc.h
 * @brief Header for extended memory allocation functions
 *
 * For license: see LICENSE file at top-level
 */

#ifndef _SHMEMXA_MEMALLOC_H
#define _SHMEMXA_MEMALLOC_H 1

#include <sys/types.h> /* size_t */

/*
 * translate between heap names and indices
 */

/** Type representing a heap index */
typedef int shmemx_heap_index_t;

/**
 * @brief Get the index for a named heap
 * @param name Name of the heap
 * @return Index of the heap, or creates new entry if name not found
 */
shmemx_heap_index_t shmemxa_name_to_index(const char *name);

/**
 * @brief Get the name of a heap from its index
 * @param index Index of the heap
 * @return Name of the heap or NULL if not found
 */
char *shmemxa_index_to_name(shmemx_heap_index_t index);

/*
 * memory allocation
 */

/**
 * @brief Initialize the memory allocator system
 * @param numheaps Number of heaps to support
 */
void shmemxa_init(shmemx_heap_index_t numheaps);

/**
 * @brief Clean up and finalize the memory allocator system
 */
void shmemxa_finalize(void);

/**
 * @brief Initialize a specific heap by its index
 * @param index Index of the heap to initialize
 * @param base Base address for the heap
 * @param capacity Size of the heap in bytes
 */
void shmemxa_init_by_index(shmemx_heap_index_t index, void *base,
                           size_t capacity);

/**
 * @brief Finalize a specific heap
 * @param index Index of the heap to finalize
 */
void shmemxa_finalize_by_index(shmemx_heap_index_t index);

/**
 * @brief Get the base address of a specific heap
 * @param index Index of the heap
 * @return Base address of the heap
 */
void *shmemxa_base_by_index(shmemx_heap_index_t index);

/**
 * @brief Allocate memory from a specific heap
 * @param index Index of the heap
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory or NULL if allocation fails
 */
void *shmemxa_malloc_by_index(shmemx_heap_index_t index, size_t size);

/**
 * @brief Allocate and zero-initialize memory from a specific heap
 * @param index Index of the heap
 * @param count Number of elements to allocate
 * @param size Size of each element in bytes
 * @return Pointer to allocated memory or NULL if allocation fails
 */
void *shmemxa_calloc_by_index(shmemx_heap_index_t index, size_t count,
                              size_t size);

/**
 * @brief Free memory from a specific heap
 * @param index Index of the heap
 * @param addr Address of memory to free
 */
void shmemxa_free_by_index(shmemx_heap_index_t index, void *addr);

/**
 * @brief Resize memory block from a specific heap
 * @param index Index of the heap
 * @param addr Address of memory to resize
 * @param new_size New size in bytes
 * @return Pointer to resized memory or NULL if reallocation fails
 */
void *shmemxa_realloc_by_index(shmemx_heap_index_t index, void *addr,
                               size_t new_size);

/**
 * @brief Allocate aligned memory from a specific heap
 * @param index Index of the heap
 * @param alignment Required alignment in bytes (must be power of 2)
 * @param size Number of bytes to allocate
 * @return Pointer to aligned memory or NULL if allocation fails
 */
void *shmemxa_align_by_index(shmemx_heap_index_t index, size_t alignment,
                             size_t size);

#endif /* ! _SHMEMXA_MEMALLOC_H */
