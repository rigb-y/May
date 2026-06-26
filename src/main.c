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

    int* mem = mgrant(sizeof(int) + 120);
    mrel(mem);
    mem = mgrant(sizeof(int));

    hat_out();

    return EXIT_SUCCESS;
}
