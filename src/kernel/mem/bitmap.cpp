#include <mem/bitmap.hpp>
#include <util/logger.h>
#include <libc/string.h>
#include <common.h>
using namespace mem;

void Bitmap::init() {
    size_t blocks = num_blocks();

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
        return nullptr;

    for (size_t i = ffa; i < tblocks; i++) {
        if (get_id(i)) continue;

        size_t fblocks = 0;
        for (; fblocks < nblocks && (i + fblocks) < tblocks && !get_id(i + fblocks); fblocks++);

        if (fblocks == nblocks) {
            bitmap::id_t nid = unique_id(i > 0 ? get_id(i - 1) : 0, i + fblocks < (tblocks - 1) ? get_id(i + fblocks) : 0);
            for (size_t j = 0; j < fblocks; j++)
                set_id(i + j, nid);
            
            if (i == ffa)
                ffa = i + fblocks;
            
            used += fblocks;
            return static_cast<void*>(bm + i * bsize);
        }

        i += fblocks;
    }

    return nullptr;
}

size_t Bitmap::free(void *ptr) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t bm_addr = reinterpret_cast<uintptr_t>(bm);
    uintptr_t start = bm_addr + num_blocks() * bsize;
    uintptr_t end = bm_addr + tsize;

    if (start > addr || addr >= end)
        return 0;
    
    size_t sblock = (addr - bm_addr) / bsize;
    size_t id = get_id(sblock);
    size_t i = 0;
    for (; get_id(sblock + i) == id && (sblock + i) < (tsize / bsize); i++)
        set_id(sblock + i, 0);
    if (sblock < ffa)
        ffa = sblock;

    return i * bsize;
}

void *Bitmap::realloc(void *old, size_t new_size) {
    uintptr_t old_addr = reinterpret_cast<uintptr_t>(old);
    uintptr_t bm_addr = reinterpret_cast<uintptr_t>(bm);
    uintptr_t start = bm_addr + num_blocks() * bsize;
    uintptr_t end = bm_addr + tsize;
    if (start > old_addr || old_addr >= end)
        return nullptr;
    void *n = alloc(new_size);
    size_t sblock = (old_addr - bm_addr) / bsize;
    uint8_t id = get_id(sblock);
    size_t i = 0;
    for (; get_id(sblock + i) == id && (sblock + i) < (tsize / bsize); i++);
    memcpy(n, old, i * bsize);
    free(old);
    return n;
}

size_t Bitmap::unique_id(size_t a, size_t b) {
}

size_t Bitmap::num_blocks() {
    return DIV_CEIL(DIV_CEIL(tsize / bsize, bitmap::BLOCKS_PER_INT), bsize);
}
