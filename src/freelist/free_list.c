#include "free_list.h"
#include "block.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

FreeList free_list = {0};

void fl_append(Header* block) {
    if (block == NULL) return;

    block->next = NULL;

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

void fl_remove(Header** block) {
    if (block == NULL || *block == NULL) return;

    if (!fl_find(*block)) return; 

    // Single element
    if (free_list.head == free_list.tail) {
        free_list.head = NULL;
        free_list.tail = NULL;
        (*block)->next = NULL;
        return;
    }

    // If the block we want to remove is the head
    if (free_list.head == *block) {
        free_list.head = free_list.head->next; 
        (*block)->next = NULL;
        return;
    }

    // General case
    Header* curr = free_list.head;
    Header* prev = free_list.head;
    while (curr != NULL) {
        if (curr == *block) {
            curr = curr->next;
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    // If the block we want to remove is the tail
    if (*block == free_list.tail) {
        free_list.tail = prev;
    }

    prev->next = curr;
    (*block)->next = NULL;
}

void fl_clear() {
}

_Bool fl_empty() {
    return (free_list.head == NULL && free_list.tail == NULL);
}

Header* fl_first_fit(size_t size) {
    if (fl_empty()) return NULL;

    Header* curr = free_list.head; 
    while (curr != NULL) {
        if (size <= curr->size) {
            curr->free = false;
            fl_remove(&curr);
            return curr; 
        }
        curr = curr->next;
    }

    return NULL;
}
