#include <drivers/initrd.hpp>
#include <drivers/ustar.hpp>
#include <lib/modules.hpp>
#include <lib/logger.hpp>
#include <limine.h>
using namespace initrd;

void initrd::init() {
    limine_file *initrd_file = mod::find("initrd");
    ustar::ustar_t archive{initrd_file->address, initrd_file->size};
}
