#pragma once
#include <stdint.h>
#include <stddef.h>

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
uint8_t bm_unique_id(uint8_t a, uint8_t b);
