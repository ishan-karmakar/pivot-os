#pragma once
#include <io/stdio.h>

class IOWriter {
public:
    virtual void operator<<(char) = 0;
    void set_global();
};
