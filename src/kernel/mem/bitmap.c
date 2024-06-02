#include <mem/bitmap.h>
#include <kernel/logging.h>
#include <sys.h>
void init_bitmap(bitmap_t *b, size_t size, size_t bsize) {
    b->bsize = bsize;
    b->size = size;

    size_t blocks = b->size / b->bsize;
    uint8_t *bm = b->bm;

    for (size_t i = 0; i < blocks; i++)
        bm[i] = 0;
    
    blocks = DIV_CEIL(blocks, b->bsize);
    for (size_t i = 0; i < blocks; i++)
        bm[i] = 1;
    
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
        if (bm[i])
            continue;
        
        size_t fblocks = 0;
        for (; fblocks < nblocks && (i + fblocks) < tblocks && !bm[i + fblocks]; fblocks++);

        if (fblocks == nblocks) {
            uint8_t nid = bm_unique_id(i > 0 ? bm[i - 1] : 0, i + fblocks < (tblocks - 1) ? bm[i + fblocks] : 0);
            for (size_t j = 0; j < fblocks; j++)
                bm[i + j] = nid;
            
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
    uintptr_t start = (uintptr_t) b->bm + DIV_CEIL(b->size / b->bsize, b->bsize) * b->bsize;
    uintptr_t end = (uintptr_t) b->bm + b->size;
    if (start > addr || addr >= end)
        return 0;
    
    uint8_t *bm = b->bm;
    size_t sblock = ((uintptr_t) ptr - (uintptr_t) bm) / b->bsize;
    uint8_t id = bm[sblock];
    size_t i;
    for (i = 0; bm[sblock + i] == id && (sblock + i) < (b->size / b->bsize); i++)
        bm[sblock + i] = 0;
    if (sblock < b->ffa)
        b->ffa = sblock;
    
    return i * b->bsize;
}

uint8_t bm_unique_id(uint8_t a, uint8_t b) {
    uint8_t c;	
	for (c = 1; c == a || c == b; c++);
	return c;
}