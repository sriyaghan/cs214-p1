/*
 * correct.c -- correctness tests for mymalloc/myfree.
 *
 * Each test checks one requirement and prints PASS or FAIL.
 * Exit status is 0 if all tests pass, 1 otherwise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "mymalloc.h"

#define MEMLENGTH 4096
#define HEADERSIZE 8

static int failures = 0;

static void report(const char *name, int ok)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) failures++;
}

/* Requirement: allocated objects do not overlap. */
static void test_no_overlap(void)
{
    enum { N = 16, SZ = 200 };
    unsigned char *p[N];
    int ok = 1;

    for (int i = 0; i < N; i++) {
        p[i] = malloc(SZ);
        if (p[i] == NULL) ok = 0;
    }
    if (ok) {
        for (int i = 0; i < N; i++) memset(p[i], i + 1, SZ);
        for (int i = 0; i < N; i++)
            for (int j = 0; j < SZ; j++)
                if (p[i][j] != i + 1) ok = 0;
    }
    for (int i = 0; i < N; i++) if (p[i]) free(p[i]);
    report("allocated objects do not overlap", ok);
}

/* Requirement: returned pointers are 8-byte aligned, for odd sizes too. */
static void test_alignment(void)
{
    void *p[10];
    int ok = 1;

    for (int i = 0; i < 10; i++) {
        p[i] = malloc(i * 3 + 1);
        if (p[i] == NULL || (uintptr_t) p[i] % 8 != 0) ok = 0;
    }
    for (int i = 0; i < 10; i++) if (p[i]) free(p[i]);
    report("payloads are 8-byte aligned", ok);
}

/* Requirement: free() makes memory available again. */
static void test_free_reuses(void)
{
    int ok = 1;

    /* The whole heap as one object, repeated: only works if free works. */
    for (int i = 0; i < 100 && ok; i++) {
        char *p = malloc(MEMLENGTH - HEADERSIZE);
        if (p == NULL) ok = 0;
        else free(p);
    }
    report("free() deallocates memory for reuse", ok);
}

/* Requirement: adjacent free chunks are coalesced. */
static void test_coalesce(void)
{
    enum { N = 64 };
    char *p[N];
    int ok = 1;

    /* Fill the heap exactly with 64 chunks of 64 bytes. */
    for (int i = 0; i < N; i++) {
        p[i] = malloc(64 - HEADERSIZE);
        if (p[i] == NULL) ok = 0;
    }

    /* Free in a scrambled order: odds, then evens descending. */
    for (int i = 1; i < N; i += 2) if (p[i]) free(p[i]);
    for (int i = N - 2; i >= 0; i -= 2) if (p[i]) free(p[i]);

    /* Only possible if all 64 chunks merged back into one. */
    char *big = malloc(MEMLENGTH - HEADERSIZE);
    if (big == NULL) ok = 0;
    else free(big);

    report("adjacent free chunks are coalesced", ok);
}

/* Requirement: malloc() returns NULL when it cannot satisfy a request. */
static void test_out_of_memory(void)
{
    int ok = 1;

    printf("  (expect 2 'Unable to allocate' messages below)\n");
    fflush(stdout);

    if (malloc(MEMLENGTH) != NULL) ok = 0;   /* header won't fit */

    char *p = malloc(MEMLENGTH / 2);
    if (p == NULL) ok = 0;
    if (malloc(MEMLENGTH / 2) != NULL) ok = 0;  /* only half remains, minus headers */
    if (p) free(p);

    report("malloc() returns NULL when out of memory", ok);
}

/* Requirement: data in an object is unchanged by other malloc/free calls. */
static void test_data_preserved(void)
{
    int ok = 1;
    char *keep = malloc(100);
    memset(keep, 'K', 100);

    for (int i = 0; i < 50; i++) {
        char *a = malloc(i + 1), *b = malloc(2 * i + 1);
        free(a);
        free(b);
    }
    for (int i = 0; i < 100; i++) if (keep[i] != 'K') ok = 0;
    free(keep);
    report("object data unchanged by other allocations", ok);
}

int main(void)
{
    test_no_overlap();
    test_alignment();
    test_free_reuses();
    test_coalesce();
    test_out_of_memory();
    test_data_preserved();

    printf("%d test(s) failed\n", failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
