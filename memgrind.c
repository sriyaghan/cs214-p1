/*
 * memgrind.c -- stress test for mymalloc/myfree.
 *
 * Runs a workload of five tasks 50 times and reports the average time
 * per workload. Every task frees all objects it allocates.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mymalloc.h"

#define RUNS 50
#define NOBJ 120

/* Task 1: allocate objects of increasing size, free them in reverse order. */
static void task1(void)
{
    static const int sizes[] = {8, 16, 32, 64, 128, 512, 1024};
    const int n = sizeof(sizes) / sizeof(sizes[0]);
    char *p[sizeof(sizes) / sizeof(sizes[0])];

    for (int i = 0; i < n; i++) p[i] = malloc(sizes[i]);
    for (int i = n - 1; i >= 0; i--) free(p[i]);
}

/* Task 2: allocate 120 small objects, free them in allocation order. */
static void task2(void)
{
    char *p[NOBJ];

    for (int i = 0; i < NOBJ; i++) p[i] = malloc(1);
    for (int i = 0; i < NOBJ; i++) free(p[i]);
}

/*
 * Task 3: randomly interleave 1-byte allocations and frees of random
 * allocated objects until 120 allocations have been made, then free the rest.
 */
static void task3(void)
{
    char *p[NOBJ];
    int live = 0, allocs = 0;

    while (allocs < NOBJ) {
        if (live == 0 || rand() % 2 == 0) {
            p[live++] = malloc(1);
            allocs++;
        } else {
            int k = rand() % live;
            free(p[k]);
            p[k] = p[--live];   /* keep live pointers packed */
        }
    }
    while (live > 0) free(p[--live]);
}

/*
 * Task 4: checkerboard fragmentation. Allocate 120 objects, free every
 * odd-indexed object, then every even one. Freeing the evens must coalesce
 * each one with its free neighbors, so a single large allocation spanning
 * most of the heap succeeds afterward.
 */
static void task4(void)
{
    char *p[NOBJ];

    for (int i = 0; i < NOBJ; i++) p[i] = malloc(8);
    for (int i = 1; i < NOBJ; i += 2) free(p[i]);
    for (int i = 0; i < NOBJ; i += 2) free(p[i]);

    char *big = malloc(2048);
    free(big);
}

/*
 * Task 5: build a singly linked list of 60 nodes, delete every other node,
 * refill the holes with nodes of varying sizes, then tear the list down
 * from the front.
 */
struct node {
    struct node *next;
    int value;
};

static void task5(void)
{
    struct node *head = NULL;

    for (int i = 0; i < 60; i++) {
        struct node *n = malloc(sizeof(struct node));
        n->value = i;
        n->next = head;
        head = n;
    }

    /* delete every second node */
    for (struct node *n = head; n != NULL && n->next != NULL; n = n->next) {
        struct node *victim = n->next;
        n->next = victim->next;
        free(victim);
    }

    /* insert new, slightly larger nodes after each surviving node */
    for (struct node *n = head; n != NULL; n = n->next->next) {
        struct node *m = malloc(sizeof(struct node) + (n->value % 4) * 8);
        m->value = -n->value;
        m->next = n->next;
        n->next = m;
    }

    while (head != NULL) {
        struct node *next = head->next;
        free(head);
        head = next;
    }
}

int main(void)
{
    struct timeval start, end;

    srand(214);

    gettimeofday(&start, NULL);
    for (int r = 0; r < RUNS; r++) {
        task1();
        task2();
        task3();
        task4();
        task5();
    }
    gettimeofday(&end, NULL);

    double elapsed = (end.tv_sec - start.tv_sec) * 1e6
                   + (end.tv_usec - start.tv_usec);
    printf("Average time per workload: %.2f microseconds (%d runs)\n",
           elapsed / RUNS, RUNS);

    return EXIT_SUCCESS;
}
