#pragma once
#include <string>

struct limine_file;
namespace mod {
    limine_file *find(std::string_view);
}