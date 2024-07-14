#pragma once
#include <io/stdio.hpp>

namespace drivers {
    class QEMUWriter : public IOWriter {
    public:
        QEMUWriter();
        void operator<<(char) override;
    };
}