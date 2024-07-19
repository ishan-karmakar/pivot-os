#include <io/stdio.h>
#include <io/stdio.hpp>
#include <util/logger.h>
#include <cstdlib>
#include <utility>
using namespace io;

static OWriter *writer;
OWriter io::cout;

// Wrapper to make things work between C and C++ classes
void io_char_printer(char c) {
    cout << c;
}

[[gnu::constructor]]
void set_char_printer() {
    char_printer = io_char_printer;
}

void OWriter::set_global() { writer = this; }

OWriter& OWriter::operator<<(char c) {
    if (writer)
        writer->write_char(c);
    return *this;
}

OWriter& OWriter::operator<<(const char *str) {
    while (*str)
        writer->write_char(*str++);
    return *this;
}

void OWriter::clear() {
    if (writer)
        writer->clear();
}

void OWriter::set_pos(coord_t pos) {
    if (writer)
        writer->set_pos(pos);
}

OWriter::coord_t OWriter::get_pos() {
    if (writer)
        return writer->get_pos();
    return { 0, 0 };
}

OWriter::coord_t OWriter::get_lims() {
    if (writer)
        return writer->get_lims();
    return { 0, 0 };
}
