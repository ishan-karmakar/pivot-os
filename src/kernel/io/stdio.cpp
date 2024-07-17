#include <io/stdio.h>
#include <io/stdio.hpp>
#include <util/logger.h>
using namespace io;

static OWriter *writer;

// Wrapper to make things work between C and C++ classes
void io_char_printer(unsigned char c) {
    *writer << c;
}

void OWriter::set_global() { writer = this; }

void clear_screen() { writer->clear(); }
