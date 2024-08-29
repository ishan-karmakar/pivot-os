process::process(void (*addr)(), bool superuser, std::size_t stack_size) :
process{
    addr,
    superuser,
    *({
        auto tbl = reinterpret_cast<mapper::page_table>(virt_addr(pmm::frame()));
        for (int i = 256; i < 512; i++) tbl[i] = mapper::kmapper->data()[i];
        auto m = new mapper::ptmapper{tbl};
        m->load();
        new vmm::vmm{PAGE_SIZE, stack_size + heap::policy_t::slabsize, superuser ? mapper::KERNEL_ENTRY : mapper::USER_ENTRY, *m};
    }),
    *new heap::pool_t{*new heap::policy_t{vmm}},
    stack_size
} {}

process::process(void (*addr)(), bool superuser, vmm::vmm& vmm, heap::pool_t &pool, std::size_t stack_size) :
    pid{++pid},
    vmm{vmm},
    pool{pool},
    fpu_data{operator new(cpu::fpu_size)}
{
    ef.rsp = reinterpret_cast<uintptr_t>(vmm.malloc(stack_size)) + stack_size;
    ef.rip = reinterpret_cast<uintptr_t>(proc_wrapper);
    ef.rdi = reinterpret_cast<uintptr_t>(addr);
    ef.rflags = 0x202;
    if (superuser) {
        ef.cs = gdt::KCODE;
        ef.ss = gdt::KDATA;
    } else {
        ef.cs = gdt::UCODE | 3;
        ef.ss = gdt::UDATA | 3;
    }

    mapper::kmapper->load();
}

void process::enqueue() {
    ready_lock.lock();
    ready_proc.push(this);
    ready_lock.unlock();
}

void proc_wrapper(void (*fn)()) {
    fn();
    syscall(SYS_exit);
}

cpu::status *scheduler::sys_exit(cpu::status *status) {
    smp::this_cpu()->cur_proc->status = Delete;
    return schedule(status);
}

cpu::status *scheduler::sys_nanosleep(cpu::status *status) {
    process*& cur_proc = smp::this_cpu()->cur_proc;
    cur_proc->status = Sleep;
    cur_proc->wakeup = timer::time() + (status->rsi / 1'000'000);
    return schedule(status);
}

void proc::sleep(std::size_t ms) {
    syscall(SYS_nanosleep, ms * 1'000'000);
}