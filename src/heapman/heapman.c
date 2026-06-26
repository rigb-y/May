#include "heapman.h"
#include "block.h"
#include "free_list.h"

#include <stdio.h>

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

    move_heap_tail(block);
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

void move_heap_tail(Header* block) {
    Header* curr_tail = heap_tail;
    heap_tail = block; 
    heap_tail->prev_phys = curr_tail;
}

void hat_out() {
    puts("HEAD:");
    block_out(heap_head);

    puts("\nHEAD->NEXT");
    if (heap_head->next_phys) {
        block_out(heap_head->next_phys);
    }

    puts("TAIL:");
    block_out(heap_tail);

    puts("\nTAIL->PREV");
    if (heap_tail->prev_phys) {
        block_out(heap_tail->prev_phys);
    }

    puts("\nTAIL->NEXT");
    if (heap_tail->next_phys) {
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

void coalesce(Header* block) {

}
