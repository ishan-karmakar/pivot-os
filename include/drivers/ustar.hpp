#pragma once
#include <cstdint>

namespace ustar {
    class ustar_t {
    public:
	ustar_t(void*, std::size_t);

    private:
	struct [[gnu::packed]] header_t {
	    char name[100];
	    char mode[8];
	    char uid[8];
	    char gid[8];
	    char size[12];
	    char mtime[12];
	    char chksum[8];
	    uint8_t type;
	    char lname[100];
	    char magic[6];
	    char version[2];
	    char o_uname[32];
	    char o_gname[32];
	    uint64_t dmaj;
	    uint64_t dmin;
	    char prefix[155];
	};
	std::size_t calc_chksum(std::size_t) const;

	char *start;
	std::size_t size;
    };
}
