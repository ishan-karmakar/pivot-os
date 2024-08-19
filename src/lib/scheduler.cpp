#include <lib/scheduler.hpp>
#include <mem/pmm.hpp>
#include <lib/logger.hpp>
using namespace scheduler;

process::process(const char *name, thread_lvl lvl) :
    name{name},
    mapper{({
        auto tbl = reinterpret_cast<mapper::page_table>(virt_addr(pmm::frame()));
        mapper::ptmapper::clean_table(tbl);
        tbl;
    })},
    vmm{PAGE_SIZE, 16 * PAGE_SIZE, lvl == Superuser ? mapper::KERNEL_ENTRY : mapper::USER_ENTRY, mapper}
    // policy{vmm},
    // pool{policy}
{
    logger::info("SCHED[PROC]", "Created new process %s", name);
}