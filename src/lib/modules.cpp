#include <cstddef>
#include <lib/modules.hpp>
#include <limine.h>
using namespace mod;

__attribute__((section(".requests")))
static limine_module_request mod_request = { LIMINE_MODULE_REQUEST, 2, nullptr, 0, nullptr };

limine_file *mod::find(std::string_view name) {
    limine_module_response *mod_response = mod_request.response;
    for (std::size_t i = 0; i < mod_response->module_count; i++)
        if (name == mod_response->modules[i]->cmdline)
            return mod_response->modules[i];
    return nullptr;
}