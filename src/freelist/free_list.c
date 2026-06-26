#include "free_list.h"
#include "block.h"
#include "mgrant.h"
#include "heapman.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

FreeList free_list = {0};

void block_out(Header* block) {
    printf("Block: \n\tSize: %zu\n\tFree: %d\n\n",
        block->size,
        block->free
    );
}

void fl_append(Header* block) {
    if (block == NULL) return;

    block->next = NULL;
    block->free = true;

    if (free_list.head == NULL) {
        free_list.head = block;
        free_list.tail = block;
        return;
    }    

    free_list.tail->next = block;
    free_list.tail = block;

}

_Bool fl_find(Header* block) {
    if (block == NULL) return false;

    Header* curr = free_list.head;
    while (curr != NULL) {
        if (curr == block) return true;
        curr = curr->next;
    }
    return false;
}

void fl_remove(Header* block) {
    if (block == NULL) return;

    if (!fl_find(block)) return; 

    block->free = false;

    // Single element
    if (free_list.head == free_list.tail) {
        free_list.head = NULL;
        free_list.tail = NULL;
        block->next = NULL;
        return;
    }

    // If the block we want to remove is the head
    if (free_list.head == block) {
        free_list.head = free_list.head->next; 
        block->next = NULL;
        return;
    }

    // General case
    Header* curr = free_list.head;
    Header* prev = free_list.head;
    while (curr != NULL) {
        if (curr == block) {
            curr = curr->next;
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    // If the block we want to remove is the tail
    if (block == free_list.tail) {
        free_list.tail = prev;
    }

    prev->next = curr;
    block->next = NULL;
}

_Bool fl_empty() {
    return (free_list.head == NULL && free_list.tail == NULL);
}

Header* fl_first_fit(size_t size) {
    if (fl_empty()) return NULL;

    Header* curr = free_list.head; 
    while (curr != NULL) {
        if (size > curr->size) {
            curr = curr->next;
            continue;
        }

        if (fl_should_split(size, curr)) {
            fl_split(size, curr);
        }

        fl_remove(curr);
        return curr; 
    }

    return NULL;
}

// Impose a minimum size (M_s) on requestable space, let's call it ALIGNMENT.
// Call the rounded size of a block (alignup(sizeof(Header))) A_B,
// block->size u, and rounded requested size alignup(request) Rr. 
// If Rr + A_B + M_s <= u, then we split the block such that the 
// first chunk fulfills our rounded request exactly. Then, the second 
// chunk is a new header + alignment padding + remaining space.
_Bool fl_should_split(size_t request, Header* block) {
    // Ideally this branch will never see daylight
    if (block == NULL) return false;

    size_t Rr = alignup(request);
    return (
        block->size >= Rr + HEADER_ALIGN + MIN_REQUEST_SIZE
    );
}

// If the header H starts at address \ell with size field u, then one past the header
// sits at \ell + A_B, where A_B = alignup(sizeof(Header)). So, the
// new header H' sits at \ell + A_B + R(r), where R(r) = alignup(request).
// The size field for H' should be set as H'->size = u - R(r) - A_B.
//
// Note these cases: 
//      (1) If we split the head of the heap and the head is the only block,
//          we must move the tail. 
//      (2) If we split the tail of the heap, we must move the tail.
void fl_split(size_t request, Header* block) {
    // Branch should never see daylight (BSNSD).
    if (block == NULL) return;

    size_t Rr = alignup(request);
    size_t remaining_space = block->size 
        - Rr
        - HEADER_ALIGN;

    block->size = Rr;

    Header* Hp = (Header*)((char*)block + (HEADER_ALIGN + Rr));
    init_header(Hp, remaining_space);
    
    fl_append(Hp);

    if (tail_is_head()) {
        get_heap_head()->next_phys = Hp; 
    }

    // We just split the heaps tail. This actually accounts
    // for both cases since the tail will be the head if there is only one block.
    if (block == get_heap_tail()) {
        move_heap_tail_forward(Hp);
    }
}

void fl_dump() {
    Header* curr = free_list.head;
    while (curr != NULL) {
        block_out(curr); 
        curr=curr->next;
    }
}
