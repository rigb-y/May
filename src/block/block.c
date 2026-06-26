#include "block.h"

/**
 * @brief Initializes the contents of a block.
 *
 * @param header pointer to a block
 * @param size the size of the block 
 */
void init_header(Header* header, size_t size) {
    header->size = size;
    header->free = false;
    header->next = NULL;
    header->next_phys = NULL;
    header->prev_phys = NULL;
}
