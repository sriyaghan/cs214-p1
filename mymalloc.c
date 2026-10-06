/*
 * mymalloc.c -- a simple error-detecting heap allocator.
 *
 * The heap is a static 4096-byte array divided into contiguous chunks.
 * Each chunk begins with an 8-byte header holding the chunk's total size
 * (header + payload) and an allocated flag. Because chunks are contiguous,
 * the next chunk is found by adding the current chunk's size to its address,
 * so the chunks form an implicit linked list starting at heap.bytes[0].
 *
 * All chunk sizes are multiples of 8 and the heap is 8-byte aligned, so every
 * payload returned to the client is 8-byte aligned. The smallest chunk is
 * 16 bytes (8-byte header + 8-byte payload).
 *
 * myfree() coalesces the freed chunk with free neighbors on either side, so
 * the heap never contains two adjacent free chunks.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

#ifndef MEMLENGTH
#define MEMLENGTH 4096
#endif

#define HEADERSIZE ((int) sizeof(header_t))
#define MINCHUNK   (HEADERSIZE + 8)

/* Round n up to the nearest multiple of 8. */
#define ALIGN8(n) (((n) + 7) & ~((size_t) 7))

static union {
    char bytes[MEMLENGTH];
    double not_used;
} heap;

static int initialized = 0;

/* Chunk header: exactly 8 bytes, so payloads stay 8-byte aligned. */
typedef struct {
    int size;       /* total chunk size in bytes, including this header */
    int allocated;  /* nonzero if the chunk is in use */
} header_t;

/* ---- chunk navigation helpers ---- */

static header_t *first_chunk(void)
{
    return (header_t *) heap.bytes;
}

/* Returns the chunk following h, or NULL if h is the last chunk. */
static header_t *next_chunk(header_t *h)
{
    char *next = (char *) h + h->size;
    if (next >= heap.bytes + MEMLENGTH) return NULL;
    return (header_t *) next;
}

static void *payload_of(header_t *h)
{
    return (char *) h + HEADERSIZE;
}

/* ---- leak detection ---- */

static void leak_check(void)
{
    size_t bytes = 0;
    int objects = 0;

    for (header_t *h = first_chunk(); h != NULL; h = next_chunk(h)) {
        if (h->allocated) {
            bytes += h->size - HEADERSIZE;
            objects++;
        }
    }

    if (objects > 0) {
        fprintf(stderr, "mymalloc: %zu bytes leaked in %d objects.\n",
                bytes, objects);
    }
}

/* ---- initialization ---- */

static void init_heap(void)
{
    header_t *h = first_chunk();
    h->size = MEMLENGTH;
    h->allocated = 0;
    initialized = 1;
    atexit(leak_check);
}

/* ---- public interface ---- */

void *mymalloc(size_t size, char *file, int line)
{
    if (!initialized) init_heap();

    /* Reject requests that cannot possibly fit (also guards ALIGN8 overflow). */
    if (size == 0 || size > MEMLENGTH - HEADERSIZE) {
        fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n",
                size, file, line);
        return NULL;
    }

    int needed = (int) ALIGN8(size) + HEADERSIZE;

    /* First fit: take the first free chunk that is large enough. */
    for (header_t *h = first_chunk(); h != NULL; h = next_chunk(h)) {
        if (h->allocated || h->size < needed) continue;

        /* Split only if the remainder can form a valid chunk. */
        if (h->size - needed >= MINCHUNK) {
            header_t *rest = (header_t *) ((char *) h + needed);
            rest->size = h->size - needed;
            rest->allocated = 0;
            h->size = needed;
        }
        h->allocated = 1;
        return payload_of(h);
    }

    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n",
            size, file, line);
    return NULL;
}

void myfree(void *ptr, char *file, int line)
{
    if (!initialized) init_heap();

    /* Like the standard free(), free(NULL) does nothing. */
    if (ptr == NULL) return;

    /*
     * Walk the chunk list looking for a chunk whose payload is exactly ptr.
     * This catches pointers outside the heap, pointers into the middle of a
     * chunk, and pointers to chunks that are already free. Only equality
     * comparisons are used, so pointers outside the heap are handled safely.
     */
    header_t *prev = NULL;
    for (header_t *h = first_chunk(); h != NULL; prev = h, h = next_chunk(h)) {
        if (payload_of(h) != ptr) continue;

        if (!h->allocated) break;   /* double free */

        h->allocated = 0;

        /* Coalesce with the following chunk if it is free. */
        header_t *next = next_chunk(h);
        if (next != NULL && !next->allocated) {
            h->size += next->size;
        }

        /* Coalesce with the preceding chunk if it is free. */
        if (prev != NULL && !prev->allocated) {
            prev->size += h->size;
        }
        return;
    }

    fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);
    exit(2);
}
