#ifndef MAY_HEAPMAN_H
#define MAY_HEAPMAN_H

typedef struct Header Header;

void adjust_hat(Header*);

Header* get_heap_tail();
Header* get_heap_head();

_Bool tail_is_head();

void move_heap_tail(Header*);

void hat_out();

void walk_heap();

void coalesce(Header*);

#endif
