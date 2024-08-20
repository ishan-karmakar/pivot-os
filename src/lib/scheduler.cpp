#include <lib/scheduler.hpp>
#include <mem/pmm.hpp>
#include <lib/logger.hpp>
using namespace scheduler;

process::process(const char *name, thread_lvl lvl) :
    name{name},
    mapper{({
        auto tbl = reinterpret_cast<mapper::page_table>(virt_addr(pmm::frame()));
        const auto kpml4 = mapper::kmapper->data();
        for (int i = 256; i < 512; i++)
            tbl[i] = kpml4[i];
        tbl;
    })},
    vmm{({
        mapper.load();
        PAGE_SIZE;
    }), 64 * PAGE_SIZE, lvl == Superuser ? mapper::KERNEL_ENTRY : mapper::USER_ENTRY, mapper},
    policy{vmm},
    pool{policy}
{
    mapper::kmapper->load(); // Reset back to kernel's address space
    logger::info("SCHED[PROC]", "Created new process %s", name);
}