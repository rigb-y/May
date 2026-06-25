#ifndef MAY_FREE_LIST_H
#define MAY_FREE_LIST_H

#include <stddef.h>

typedef struct Header Header;

typedef struct FreeList {
    Header* head;
    Header* tail;
} FreeList;

extern FreeList free_list;

_Bool fl_find(Header*);

void fl_append(Header*);
void fl_remove(Header**);

void fl_clear();

_Bool fl_empty();

Header* fl_first_fit(size_t);

#endif
