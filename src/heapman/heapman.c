#include "heapman.h"
#include "block.h"
#include "free_list.h"
#include "mgrant.h"

#include <stdio.h>
#include <stdbool.h>

static Header* heap_tail = NULL;
static Header* heap_head = NULL;

/**
 * @brief Adjusts the global heap_head and heap_tail pointers.
 *
 * @param block pointer to a block.
 *
 * @note Head and tail (hat).
 */
void adjust_hat(Header* block) {
    if (block == NULL) return; 

    // The passed block is the first allocated
    if (heap_head == NULL) {
        heap_head = block;
        heap_head->prev_phys = NULL;

        heap_tail = heap_head;

        return;
    }

    // The passed block is the second allocated
    if (heap_head == heap_tail) {
        heap_head->next_phys = block;
    }

    // Greater than two allocations
    else if (heap_tail != NULL) {
        heap_tail->next_phys = block;
    }

    move_heap_tail_forward(block);
}

/**
 * @brief Returns the heap_tail pointer.
 *
 * @return Pointer to heap_tail.
 */
Header* get_heap_tail() {
    return heap_tail;
}

/**
 * @brief Returns the heap_head pointer.
 *
 * @return Pointer to heap_head.
 */
Header* get_heap_head() {
    return heap_head;
}

/**
 * @brief Checks if heap_head and heap_tail point to the same block.
 *
 * @return True / false.
 */
_Bool tail_is_head() {
    return get_heap_tail() 
        == get_heap_head();
}

/**
 * @brief Moves the heap_tail pointer forward to block.
 *
 * @param block pointer to a block.
 *
 * @note It should be the case that block is immediately after tail. 
 */
void move_heap_tail_forward(Header* block) {
    Header* curr_tail = heap_tail;
    heap_tail = block; 
    heap_tail->prev_phys = curr_tail;
}

/**
 * @brief Moves the heap_tail pointer backward to block.
 *
 * @param block pointer to a block.
 *
 * @note It should be the case that block is immediately before tail. 
 */
void move_heap_tail_backward(Header* block) {
    heap_tail = block;
}

/**
 * @brief A debug util that sends the contents of both the
 * heap_head pointer and heap_tail pointer to stdout,
 * along with their neighbors in physical memory.
 */
void hat_out() {
    puts("HEAD:");
    block_out(heap_head);

    if (heap_head) {
        puts("\nHEAD->NEXT");
        block_out(heap_head->next_phys);
    }

    puts("TAIL:");
    block_out(heap_tail);

    if (heap_tail) {
        puts("\nTAIL->PREV");
        block_out(heap_tail->prev_phys);

        puts("\nTAIL->NEXT");
        block_out(heap_tail->next_phys);
    }
}

/**
 * @brief A debug util that sends the contents of the heap
 * to stdout.
 */
void walk_heap() {
    Header* curr = get_heap_head();

    while (curr != NULL) {
        block_out(curr);
        curr = curr->next_phys; 
    }
}

/**
 * @brief Coalesces a block with its left side physical neighbor if that
 * neighbor is free.
 *
 * @param block address of a pointer to a block.
 * @return True if coalescing occurred, false otherwise.
 */
_Bool coalesce_left(Header** block) {
    if (block == NULL ||
        (*block) == NULL ||
        (*block)->prev_phys == NULL ||
        (*block)->prev_phys->free == false
    ) {
        return false;
    }

    // If we are still here, heap_head cannot be block,
    // but it might be heap_tail.
    
    if (get_heap_tail() == *block) {
        move_heap_tail_backward((*block)->prev_phys);
    }

    size_t new_size = (*block)->size 
        + (*block)->prev_phys->size
        + HEADER_ALIGN;

    Header* save_next_phys = (*block)->next_phys;
    (*block) = (*block)->prev_phys;
    (*block)->size = new_size;
    (*block)->next_phys = save_next_phys;

    if (save_next_phys != NULL) {
        save_next_phys->prev_phys = *block;
    }

    return true;
}

/**
 * @brief Coalesces a block with its right side physical neighbor if that
 * neighbor is free.
 *
 * @param block address of a pointer to a block.
 * @return True if coalescing occurred, false otherwise.
 */
_Bool coalesce_right(Header** block) {
    if (block == NULL ||
        (*block) == NULL ||
        (*block)->next_phys == NULL ||
        (*block)->next_phys->free == false
    ) {
        return false;
    }

    fl_remove((*block)->next_phys);

    Header* save_next_phys  = (*block)->next_phys;
    Header* after_next = save_next_phys->next_phys;

    if (save_next_phys == get_heap_tail()) {
        move_heap_tail_backward(*block);
    }

    size_t new_size = (*block)->size 
        + (*block)->next_phys->size
        + HEADER_ALIGN;

    (*block)->size = new_size;
    (*block)->next_phys = after_next;

    if (after_next != NULL) {
        after_next->prev_phys = *block;
    }

    fl_append(*block);

    return true;
}

/**
 * @brief Calls both coalescing functions described above.
 *
 * @param block address of a pointer to a block
 * @return True if coalescing occurred, false otherwise.
 */
_Bool coalesce(Header** block) {
    return coalesce_left(block) |
        coalesce_right(block);
}
