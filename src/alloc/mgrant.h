#ifndef MAY_MGRANT_H
#define MAY_MGRANT_H

#include <stddef.h>

typedef struct Header Header;

#define ALIGNMENT _Alignof(max_align_t)

#define MIN_REQUEST_SIZE ALIGNMENT

size_t alignup(size_t);

#define HEADER_ALIGN alignup(sizeof(Header))

void* mgrant(size_t); 

#endif
