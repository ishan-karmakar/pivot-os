#pragma once
#include <io/stdio.h>

namespace io {
    class OWriter {
    public:
        virtual void operator<<(char) = 0;
        virtual void clear() {};
        void set_global();
    };

    class IWriter {
    public:
        virtual void operator>>(char&) = 0;
    };

    class IOWriter : public IWriter, public OWriter {};
}
void clear_screen();
