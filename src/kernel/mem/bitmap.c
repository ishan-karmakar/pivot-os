#include <mem/bitmap.h>
#include <libc/string.h>
#include <util/logger.h>
#include <common.h>
#include <kernel.h>
#define BM_BLOCKS(size, bsize) DIV_CEIL(DIV_CEIL((size) / (bsize), BLOCKS_PER_INT), (bsize))
static void set_bm(uint8_t*, size_t, uint8_t);
static uint8_t get_bm(uint8_t*, size_t);
static uint8_t bm_unique_id(uint8_t a, uint8_t b);

void init_bitmap(bitmap_t *b, size_t size, size_t bsize) {
    b->bsize = bsize;
    b->size = size;

    size_t blocks = b->size / b->bsize;
    uint8_t *bm = b->bm;

    for (size_t i = 0; i < blocks; i++)
        set_bm(bm, i, 0);
    
    blocks = BM_BLOCKS(b->size, b->bsize);
    for (size_t i = 0; i < blocks; i++)
        set_bm(bm, i, 1);
    b->ffa = blocks;
    b->used = blocks;

    log(Verbose, "BITMAP", "Initialized region with %u bytes, %u block size", size, bsize);
}

void *bm_alloc(size_t size, bitmap_t *b) {
    size_t tblocks = b->size / b->bsize;
    size_t nblocks = DIV_CEIL(size, b->bsize);
    if ((tblocks - b->used) < nblocks)
        return NULL;
    uint8_t *bm = (uint8_t*) b->bm;
    for (size_t i = b->ffa; i < tblocks; i++) {
        if (get_bm(bm, i))
            continue;
        
        size_t fblocks = 0;
        for (; fblocks < nblocks && (i + fblocks) < tblocks && !get_bm(bm, i + fblocks); fblocks++);

        if (fblocks == nblocks) {
            uint8_t nid = bm_unique_id(i > 0 ? get_bm(bm, i - 1) : 0, i + fblocks < (tblocks - 1) ? get_bm(bm, i + fblocks) : 0);
            for (size_t j = 0; j < fblocks; j++)
                set_bm(bm, i + j, nid);
            
            if (i == b->ffa)
                b->ffa = i + fblocks;
            
            b->used += fblocks;
            return (void*) (bm + i * b->bsize);
        }

        i += fblocks;
    }

    return NULL;
}

size_t bm_free(void *ptr, bitmap_t *b) {
    uintptr_t addr = (uintptr_t) ptr;
    uintptr_t start = (uintptr_t) b->bm + BM_BLOCKS(b->size, b->bsize) * b->bsize;
    uintptr_t end = (uintptr_t) b->bm + b->size;
    if (start > addr || addr >= end)
        return 0;
    
    uint8_t *bm = b->bm;
    size_t sblock = ((uintptr_t) ptr - (uintptr_t) bm) / b->bsize;
    uint8_t id = get_bm(bm, sblock);
    size_t i = 0;
    for (; get_bm(bm, sblock + i) == id && (sblock + i) < (b->size / b->bsize); i++)
        set_bm(bm, sblock + i, 0);
    if (sblock < b->ffa)
        b->ffa = sblock;
    
    return i * b->bsize;
}

void *bm_realloc(void *old, size_t new_size, bitmap_t *b) {
    uintptr_t start = (uintptr_t) b->bm + BM_BLOCKS(b->size, b->bsize) * b->bsize;
    uintptr_t end = (uintptr_t) b->bm + b->size;
    if (start > (uintptr_t) old || (uintptr_t) old >= end)
        return NULL;
    void *new = bm_alloc(new_size, b);
    uint8_t *bm = b->bm;
    size_t sblock = ((uintptr_t) old - (uintptr_t) bm) / b->bsize;
    uint8_t id = get_bm(bm, sblock);
    size_t i = 0;
    for (; get_bm(bm, sblock + i) == id && (sblock + i) < (b->size / b->bsize); i++);
    memcpy(new, old, i * b->bsize);
    bm_free(old, b);
    return new;
}

static void set_bm(uint8_t *bm, size_t block, uint8_t id) {
    size_t row = block / BLOCKS_PER_INT;
    size_t col = (block % BLOCKS_PER_INT) * BITS_PER_ID;
    bm[row] |= (id & ((1 << BITS_PER_ID) - 1)) << col;
}

static uint8_t get_bm(uint8_t *bm, size_t block) {
    size_t row = block / BLOCKS_PER_INT;
    size_t col = (block % BLOCKS_PER_INT) * BITS_PER_ID;
    return bm[row] & (((1 << BITS_PER_ID) - 1) << col);
}

static uint8_t bm_unique_id(uint8_t a, uint8_t b) {
    uint8_t c;
	for (c = 1; c == a || c == b; c++);
	return c;
}