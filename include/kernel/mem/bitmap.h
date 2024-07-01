#pragma once
#include <stdint.h>
#include <stddef.h>
#define BITS_PER_ID 2
#define BLOCKS_PER_INT (8 / BITS_PER_ID)

typedef struct {
    size_t size;
    size_t bsize;
    size_t used; // Used blocks
    size_t ffa;
    uint8_t *bm;
} bitmap_t;

void init_bitmap(bitmap_t*, size_t size, size_t bsize);
void *bm_alloc(size_t size, bitmap_t*);
size_t bm_free(void *ptr, bitmap_t*);
void *bm_realloc(void*, size_t, bitmap_t*);
