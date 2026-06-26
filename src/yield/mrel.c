#include "mrel.h"
#include "block.h"
#include "free_list.h"
#include "heapman.h"
#include "mgrant.h"

/**
 * @brief Frees memory that was allocated by mgrant.
 *
 * @param mem pointer to memory.
 */
void mrel(void* mem) {
    if (mem == NULL) return;

    Header* block = (Header*)((char*)mem - HEADER_ALIGN);

    if (block->free || fl_find(block)) {
        return;
    }

    if (coalesce(&block)) {
        return;
    }

    fl_append(block);
}
