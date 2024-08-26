#pragma once
#include <vector>
#include <limine_terminal/term.h>

namespace term {
    extern std::vector<term_t*> terms;

    void init();
    inline void clear() {
        for (const auto& t : terms)
            t->clear(t, true);
    }
}