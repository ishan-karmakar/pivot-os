#include <io/stdio.h>
#include <io/stdio.hpp>
#include <util/logger.h>

static IOWriter *writer;

// Wrapper to make things work between C and C++ classes
void io_char_printer(char c) {
    *writer << c;
}

// Since this file is listed first in compilation order, this will be the first function run from call_constructors
// and will only be run ONCE
[[gnu::constructor]]
void set_char_printer() {
    char_printer = io_char_printer;
}

void IOWriter::set_global() { writer = this; }
