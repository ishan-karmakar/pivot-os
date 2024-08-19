#pragma once
#include <mem/mapper.hpp>

namespace scheduler {
    struct process {
        const char *name;
        mapper::ptmapper mapper;
    };
}