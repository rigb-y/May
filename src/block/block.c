#include "block.h"

void init_header(Header* header, size_t size) {
    header->size = size;
    header->free = false;
    header->next = NULL;
}
