#pragma once
#include <io/stdio.h>

class IOWriter {
public:
    virtual void operator<<(char) = 0;
    void set_global();
};

void set_writer(IOWriter&);
void io_char_printer(char);