#include "heapman.h"
#include "block.h"
#include "free_list.h"
#include "mgrant.h"

#include <stdio.h>
#include <stdbool.h>

static Header* heap_tail = NULL;
static Header* heap_head = NULL;

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

Header* get_heap_tail() {
    return heap_tail;
}

Header* get_heap_head() {
    return heap_head;
}

_Bool tail_is_head() {
    return get_heap_tail() 
        == get_heap_head();
}

void move_heap_tail_forward(Header* block) {
    Header* curr_tail = heap_tail;
    heap_tail = block; 
    heap_tail->prev_phys = curr_tail;
}

void move_heap_tail_backward(Header* block) {
    heap_tail = block;
}

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

void walk_heap() {
    Header* curr = get_heap_head();

    while (curr != NULL) {
        block_out(curr);
        curr = curr->next_phys; 
    }
}

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

_Bool coalesce(Header** block) {
    return coalesce_left(block) |
        coalesce_right(block);
}
