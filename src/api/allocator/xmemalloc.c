/**
 * @file xmemalloc.c
 * @brief Implementation of memory allocation functions
 *
 * For license: see LICENSE file at top-level
 */

#include "xmemalloc.h"
#include "../klib/khash.h"

#include "internal-malloc.h"

#include <stdio.h>
#include <assert.h>

/**
 * @brief Array of memory spaces for multiple heaps
 */
static mspace *spaces;

/*
 * map named heap to its index
 */

KHASH_MAP_INIT_STR(heapnames, shmemx_heap_index_t)

/**
 * @brief Hash table mapping heap names to indices
 */
static khash_t(heapnames) * names;

/**
 * @brief Current heap index counter
 */
static shmemx_heap_index_t idx = 0;

/*
 * translate between name and index (create if needed)
 */

/**
 * @brief Look up a heap index by name
 * @param name Name of the heap to look up
 * @return Index of the heap if found, -1 otherwise
 */
inline static shmemx_heap_index_t lookup_name(const char *name) {
  const khiter_t k = kh_get(heapnames, names, name);

  if (k != kh_end(names)) {
    return kh_value(names, k);
  } else {
    return -1;
  }
}

/**
 * @brief Record a new heap name and assign it an index
 * @param name Name of the heap to record
 * @return New index assigned to the heap, or -1 if recording fails
 */
inline static shmemx_heap_index_t record_name(const char *name) {
  int there;
  khiter_t k;
  const shmemx_heap_index_t mine = idx;

  k = kh_put(heapnames, names, name, &there);
  if (there != 1) {
    return -1;
    /* NOT REACHED */
  }

  kh_value(names, k) = mine;

  ++idx;

  return mine;
}

/**
 * @brief Convert a heap name to its corresponding index
 * @param name Name of the heap
 * @return Index of the heap, creating a new entry if needed
 */
shmemx_heap_index_t shmemxa_name_to_index(const char *name) {
  const shmemx_heap_index_t i = lookup_name(name);

  if (i == -1) {
    return record_name(name);
  } else {
    return i;
  }
}

/**
 * @brief Convert a heap index to its corresponding name
 * @param index Index of the heap
 * @return Name of the heap or NULL if not found
 */
char *shmemxa_index_to_name(shmemx_heap_index_t index) {
  khiter_t k;

  for (k = kh_begin(names); k != kh_end(names); ++k) {
    if (kh_exist(names, k)) {
      if (kh_value(names, k) == index) {
        return (char *)kh_key(names, k);
        /* NOT REACHED */
      }
    }
  }

  return NULL;
}

/*
 * boot API
 */

/**
 * @brief Number of heaps in the system
 */
static shmemx_heap_index_t nheaps; /* local copy */

/**
 * @brief Initialize the heap management system
 * @param numheaps Number of heaps to initialize
 */
void shmemxa_init(shmemx_heap_index_t numheaps) {
  spaces = (mspace *)malloc(numheaps * sizeof(*spaces));

  assert(spaces != NULL);

  nheaps = numheaps;
}

/**
 * @brief Finalize and clean up the heap management system
 */
void shmemxa_finalize(void) { free(spaces); }

/*
 * manage heap
 */

/**
 * @brief Initialize a specific heap by index
 * @param index Index of the heap to initialize
 * @param base Base address for the heap
 * @param capacity Size of the heap in bytes
 */
void shmemxa_init_by_index(shmemx_heap_index_t index, void *base,
                           size_t capacity) {
  spaces[index] = create_mspace_with_base(base, capacity, 1);
}

/**
 * @brief Finalize a specific heap by index
 * @param index Index of the heap to finalize
 */
void shmemxa_finalize_by_index(shmemx_heap_index_t index) {
  destroy_mspace(spaces[index]);
}

/*
 * heap allocations
 */

/**
 * @brief Get the base address of a specific heap
 * @param index Index of the heap
 * @return Base address of the heap
 */
void *shmemxa_base_by_index(shmemx_heap_index_t index) { return spaces[index]; }

/**
 * @brief Allocate memory from a specific heap
 * @param index Index of the heap
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory or NULL if allocation fails
 */
void *shmemxa_malloc_by_index(shmemx_heap_index_t index, size_t size) {
  return mspace_malloc(spaces[index], size);
}

/**
 * @brief Allocate and zero-initialize memory from a specific heap
 * @param index Index of the heap
 * @param count Number of elements to allocate
 * @param size Size of each element in bytes
 * @return Pointer to allocated memory or NULL if allocation fails
 */
void *shmemxa_calloc_by_index(shmemx_heap_index_t index, size_t count,
                              size_t size) {
  return mspace_calloc(spaces[index], count, size);
}

/**
 * @brief Free memory from a specific heap
 * @param index Index of the heap
 * @param addr Address of memory to free
 */
void shmemxa_free_by_index(shmemx_heap_index_t index, void *addr) {
  mspace_free(spaces[index], addr);
}

/**
 * @brief Resize memory block from a specific heap
 * @param index Index of the heap
 * @param addr Address of memory to resize
 * @param new_size New size in bytes
 * @return Pointer to resized memory or NULL if reallocation fails
 */
void *shmemxa_realloc_by_index(shmemx_heap_index_t index, void *addr,
                               size_t new_size) {
  return mspace_realloc(spaces[index], addr, new_size);
}

/**
 * @brief Allocate aligned memory from a specific heap
 * @param index Index of the heap
 * @param alignment Required alignment in bytes (must be power of 2)
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory or NULL if allocation fails
 */
void *shmemxa_align_by_index(shmemx_heap_index_t index, size_t alignment,
                             size_t size) {
  return mspace_memalign(spaces[index], alignment, size);
}
