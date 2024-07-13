#include <mem/bitmap.hpp>
#include <util/logger.h>
#include <libc/string.h>
#include <common.h>
using namespace mem;

const uint8_t BITS_PER_ID = 2;
const uint8_t BLOCKS_PER_INT = sizeof(uint8_t) * 8 / BITS_PER_ID;

void Bitmap::init(size_t tsize, size_t bsize, uint8_t *bm) {
    this->tsize = tsize;
    this->bsize = bsize;
    this->bm = bm;
    size_t blocks = header_blocks();

    for (size_t i = 0; i < blocks; i++)
        set_id(i, 1);

    for (size_t i = blocks; i < tsize / bsize - blocks; i++)
        set_id(i, 0);

    ffa = blocks;
    used = blocks;

    log(Verbose, "BITMAP", "Initialized region with %u bytes, %u block size", tsize, bsize);
}

void *Bitmap::alloc(size_t nsize) {
    size_t tblocks = tsize / bsize;
    size_t nblocks = DIV_CEIL(nsize, bsize);

    if ((tblocks - used) < nblocks)
        return 0;

    for (size_t i = ffa; i < tblocks; i++) {
        if (get_id(i)) continue;

        size_t fblocks = 0;
        for (; fblocks < nblocks && (i + fblocks) < tblocks && !get_id(i + fblocks); fblocks++);

        if (fblocks == nblocks) {
            uint8_t nid = unique_id(i > 0 ? get_id(i - 1) : 0, i + fblocks < (tblocks - 1) ? get_id(i + fblocks) : 0);
            for (size_t j = 0; j < fblocks; j++)
                set_id(i + j, nid);

            if (i == ffa)
                ffa = i + fblocks;
            
            used += fblocks;

            void *ptr = bm + i * bsize;
            post_alloc(ptr, nblocks);
            return ptr;
        }

        i += fblocks;
    }

    return 0;
}

void Bitmap::free(void *ptr) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t bm_addr = reinterpret_cast<uintptr_t>(bm);
    uintptr_t start = bm_addr + header_blocks() * bsize;
    uintptr_t end = bm_addr + tsize;

    if (start > addr || addr >= end) return;
    
    size_t sblock = (addr - bm_addr) / bsize;
    size_t id = get_id(sblock);
    size_t i = 0;
    for (; get_id(sblock + i) == id && (sblock + i) < (tsize / bsize); i++)
        set_id(sblock + i, 0);
    if (sblock < ffa)
        ffa = sblock;

    post_free(ptr, i);
}

void *Bitmap::realloc(void *ptr, size_t new_size) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t bm_addr = reinterpret_cast<uintptr_t>(bm);
    uintptr_t start = bm_addr + header_blocks() * bsize;
    uintptr_t end = bm_addr + tsize;
    if (start > addr || addr >= end)
        return 0;
    void *n = alloc(new_size);
    size_t sblock = (addr - bm_addr) / bsize;
    uint8_t id = get_id(sblock);
    size_t i = 0;
    for (; get_id(sblock + i) == id && (sblock + i) < (tsize / bsize); i++);
    memcpy(n, ptr, i * bsize);
    free(ptr);
    return n;
}

void Bitmap::set_id(size_t block, uint8_t id) {
    size_t row = block / BLOCKS_PER_INT;
    size_t col = (block % BLOCKS_PER_INT) * BITS_PER_ID;
    bm[row] |= (id & ((1 << BITS_PER_ID) - 1)) << col;
}

uint8_t Bitmap::get_id(size_t block) {
    size_t row = block / BLOCKS_PER_INT;
    size_t col = (block % BLOCKS_PER_INT) * BITS_PER_ID;
    return bm[row] & (((1 << BITS_PER_ID) - 1) << col);
}

uint8_t Bitmap::unique_id(uint8_t a, uint8_t b) {
    uint8_t c = 1;
    for (; c == a || c == b; c++);
    return c;
}

size_t Bitmap::header_blocks() {
    return DIV_CEIL(DIV_CEIL(tsize / bsize, BLOCKS_PER_INT), bsize);
}
