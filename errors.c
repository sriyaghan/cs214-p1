/*
 * errors.c -- triggers one detectable error, chosen by the first argument.
 *
 *   ./errors 1   free() an address not obtained from malloc()
 *   ./errors 2   free() an address not at the start of a chunk
 *   ./errors 3   free() the same pointer twice
 *   ./errors 4   leak two objects (leak report at exit, exit status 0)
 *   ./errors 5   free(NULL), which is a no-op (exit status 0, no output)
 *
 * Cases 1-3 should print "free: Inappropriate pointer" and exit with status 2.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s 1|2|3|4|5\n", argv[0]);
        return EXIT_FAILURE;
    }

    switch (atoi(argv[1])) {
    case 1: {
        int x;
        free(&x);
        break;
    }
    case 2: {
        int *p = malloc(sizeof(int) * 2);
        free(p + 1);
        break;
    }
    case 3: {
        int *p = malloc(sizeof(int) * 100);
        int *q = p;
        free(p);
        free(q);
        break;
    }
    case 4: {
        malloc(20);   /* 24 bytes after rounding */
        malloc(100);  /* 104 bytes after rounding */
        return EXIT_SUCCESS;
    }
    case 5:
        free(NULL);
        return EXIT_SUCCESS;
    default:
        fprintf(stderr, "usage: %s 1|2|3|4|5\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("error was NOT detected\n");
    return EXIT_FAILURE;
}
