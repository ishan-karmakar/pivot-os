#include <mem/bitmap.hpp>
#include <util/logger.hpp>
#include <cstring>
#include <kernel.hpp>
#include <io/stdio.hpp>
using namespace mem;

const uint8_t BITS_PER_ID = 2;
const uint8_t BLOCKS_PER_INT = sizeof(uint8_t) * 8 / BITS_PER_ID;

Bitmap::Bitmap(size_t tsize, size_t bsize, uint8_t *bm) : tsize{tsize}, bsize{bsize}, bm{bm}, hblocks{div_ceil(div_ceil<size_t>(tsize / bsize, BLOCKS_PER_INT), bsize)}, used{hblocks}, ffa{used} {
    for (size_t i = 0; i < used; i++)
        set_id(i, 1);

    for (size_t i = used; i < tsize / bsize - used; i++)
        set_id(i, 0);

    log(VERBOSE, "BITMAP", "Initialized region with %lu bytes, %lu block size", tsize, bsize);
}

void *Bitmap::malloc(size_t nsize) {
    size_t tblocks = tsize / bsize;
    size_t nblocks = div_ceil(nsize, bsize);

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
            return ptr;
        }

        i += fblocks;
    }
    return 0;
}

size_t Bitmap::free(void *ptr) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t bm_addr = reinterpret_cast<uintptr_t>(bm);
    uintptr_t start = bm_addr + hblocks * bsize;
    uintptr_t end = bm_addr + tsize;

    if (start > addr || addr >= end) return 0;
    
    size_t sblock = (addr - bm_addr) / bsize;
    uint8_t id = get_id(sblock);
    size_t i = 0;
    for (; get_id(sblock + i) == id && (sblock + i) < (tsize / bsize); i++)
        set_id(sblock + i, 0);
    used -= i;

    if (sblock < ffa)
        ffa = sblock;
    return i;
}

void *Bitmap::realloc(void *ptr, size_t new_size) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t bm_addr = reinterpret_cast<uintptr_t>(bm);
    uintptr_t start = bm_addr + hblocks * bsize;
    uintptr_t end = bm_addr + tsize;
    if (start > addr || addr >= end)
        return 0;
    void *n = malloc(new_size);
    size_t sblock = (addr - bm_addr) / bsize;
    uint8_t id = get_id(sblock);
    size_t i = 0;
    for (; get_id(sblock + i) == id && (sblock + i) < (tsize / bsize); i++);
    // memcpy(n, ptr, i * bsize);
    free(ptr);
    return n;
}

void *Bitmap::calloc(size_t size) {
    void *ptr = malloc(size);
    // memset(ptr, 0, size);
    return ptr;
}

void Bitmap::set_id(size_t block, uint8_t id) {
    size_t row = block / BLOCKS_PER_INT;
    size_t col = (block % BLOCKS_PER_INT) * BITS_PER_ID;
    id &= ((1 << BITS_PER_ID) - 1);

    // Hopefully there is a better way of doing this
    // First reset bits I want to modify to 0
    // Do an OR to insert bits at col
    bm[row] &= ~(0b11 << col);
    bm[row] |= id << col;
}

uint8_t Bitmap::get_id(size_t block) const {
    size_t row = block / BLOCKS_PER_INT;
    size_t col = (block % BLOCKS_PER_INT) * BITS_PER_ID;
    return (bm[row] & (((1 << BITS_PER_ID) - 1) << col)) >> col;
}

uint8_t Bitmap::unique_id(uint8_t a, uint8_t b) const {
    uint8_t c = 1;
    for (; c == a || c == b; c++);
    return c;
}
