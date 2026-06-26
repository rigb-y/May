#define _GNU_SOURCE

#include "mgrant.h"
#include "free_list.h"
#include "block.h"
#include "heapman.h"

#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include <stdio.h>

/**
 * @brief Rounds a size upward such that it is properly aligned
 * according to ALIGNMENT (_Alignof(max_align_t)).
 *
 * @param x a size to be aligned.
 * @return The aligned size.
 */
size_t alignup(size_t x) {
    return (x + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

/**
 * @brief Returns heap allocated memory to the caller.
 *
 * @param size the size of the request.
 * @return Pointer to allocated memory or NULL if the 
 * allocation was not successful.
 */
void* mgrant(size_t size) {
    size_t aligned_size = alignup(size);
    size_t total_size = HEADER_ALIGN + aligned_size;

    Header* block = fl_first_fit(aligned_size);

    // Block found
    if (block != NULL) {
        return (void*)((char*)block + HEADER_ALIGN);
    }

    void* raw_memory = sbrk((intptr_t)(total_size));

    // sbrk failure
    if (raw_memory == (void*)-1) {
        return NULL; 
    }

    block = raw_memory;
    init_header(block, aligned_size);
    block->prev_phys = get_heap_tail();

    adjust_hat(block);

    return (void*)((char*)block + HEADER_ALIGN);
}
