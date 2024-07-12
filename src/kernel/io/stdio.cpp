#include <io/stdio.h>
#include <io/stdio.hpp>

static IOWriter *writer;

void IOWriter::set_global() { writer = this; }

// Wrapper to make things work between C and C++ classes
void io_char_printer(char c) {
    *writer << c;
}