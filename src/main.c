#include "may.h"
#include "free_list.h"
#include "heapman.h"

#include <stdio.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char** argv) {

    int* mem1 = mgrant(sizeof(int) + 120);
    int* mem2 = mgrant(sizeof(int) + 250);
    mrel(mem2);

    int* mem3 = mgrant(sizeof(int) + 250);

    int* mem4 = mgrant(sizeof(int) + 450);
    mrel(mem4);

    mrel(mem3);

    walk_heap();

    return EXIT_SUCCESS;
}
