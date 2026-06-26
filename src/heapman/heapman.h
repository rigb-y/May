#ifndef MAY_HEAPMAN_H
#define MAY_HEAPMAN_H

typedef struct Header Header;

void adjust_hat(Header*);

Header* get_heap_tail();
Header* get_heap_head();

_Bool tail_is_head();

void move_heap_tail_forward(Header*);
void move_heap_tail_backward(Header*);

void hat_out();

void walk_heap();

_Bool coalesce_left(Header**);
_Bool coalesce_right(Header**);
_Bool coalesce(Header**);

#endif
