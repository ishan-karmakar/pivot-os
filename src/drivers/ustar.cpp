#include <drivers/ustar.hpp>
#include <lib/logger.hpp>
#include <kernel.hpp>
#include <cstring>
using namespace ustar;

std::size_t oct2bin(const char *str, uint32_t size) {
    std::size_t n = 0;
    for (std::size_t i = 0; i < size; i++) {
	n *= 8;
	n += *str++ - '0';
    }
    return n;
}

ustar_t::ustar_t(void *_start, std::size_t _size) : start{static_cast<char*>(_start)}, size{_size} {
    std::size_t num_blocks = size / 512;
    logger::info("USTAR", "Num blocks: %lu", num_blocks);
    for (std::size_t i = 0; i < num_blocks; i++) {
	auto header = reinterpret_cast<header_t*>(start + i * 512);
	if (!header->type) continue; // Making sure block isn't zero filled (for any valid block, type has to be a integer)
	std::size_t checksum = oct2bin(header->chksum, 6);
	logger::info("USTAR", "%lu, %lu", checksum, calc_chksum(i));
	if (header->type == '5') {
	    logger::info("USTAR", "DIR: %s", header->name);
	} else if (header->type == '0') {
	    std::size_t size = oct2bin(header->size, 12);
	    logger::info("USTAR", "FILE: %s, %lu", header->name, size);
	    i += div_ceil(size, 512);
	} else {
	    logger::info("USTAR", "%c, %s, %lu", header->type, header->name, oct2bin(header->size, 12));
	}
    }
}

std::size_t ustar_t::calc_chksum(std::size_t block) const {
    char *bstart = start + block * 512;
    std::size_t sum = 0;
    std::size_t name_start = offsetof(header_t, chksum);
    for (std::size_t i = 0; i < sizeof(header_t); i++)
	sum += i >= name_start && i < (name_start + 8) ? ' ' : bstart[i];
    return sum;
}
