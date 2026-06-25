#include "may.h"
#include "free_list.h"

#include <stdio.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    int* mem = mgrant(sizeof(int));
    mrel(mem);

    *mem = 15;
    printf("%d\n", *mem);

    mrel(mem);
    mem = NULL;

    int* mem2 = mgrant(sizeof(int));

    *mem2 = 25;
    printf("%d\n", *mem2);

    int* mem3 = mgrant(sizeof(int));

    *mem3 = 30;
    printf("%d\n", *mem3);

    double* mem4 = mgrant(sizeof(double));

    *mem4 = 30.0f;
    printf("%.2f\n", *mem4);


    return EXIT_SUCCESS;
}
