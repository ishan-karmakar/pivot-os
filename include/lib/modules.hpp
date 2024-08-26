#pragma once
#include <frg/string.hpp>

struct limine_file;
namespace mod {
    limine_file *find(frg::string_view);
}