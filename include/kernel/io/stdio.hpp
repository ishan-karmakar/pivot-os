#pragma once
#include <cstdarg>
#include <cstddef>
#include <utility>
#include <io/stdio.h>

namespace io {
    class OWriter {
    public:
        // A typedef for a point on the coordinate plane
        typedef std::pair<size_t, size_t> coord_t;
        void set_global();
        OWriter& operator<<(char);
        OWriter& operator<<(const char*);
        virtual void clear();
        virtual void set_pos(coord_t);
        virtual coord_t get_pos();
        // Get limits (max cols, max rows)
        virtual coord_t get_lims();

    protected:
        virtual void write_char(char) {}
    };

    class IWriter {
    public:
        virtual void operator>>(char&) = 0;
    };

    class IOWriter : public IWriter, public OWriter {};

    extern OWriter cout;
}
