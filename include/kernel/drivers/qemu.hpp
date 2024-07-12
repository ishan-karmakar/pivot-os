#pragma once
#include <io/stdio.hpp>

namespace drivers {
    class QEMUWriter : public IOWriter {
    public:
        void init();
        void operator<<(char);
    };
}