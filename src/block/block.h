#ifndef MAY_BLOCK_H
#define MAY_BLOCK_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Header {
    size_t size;
    _Bool free;
    struct Header* next;
} Header;

void init_header(Header*, size_t);

#endif
