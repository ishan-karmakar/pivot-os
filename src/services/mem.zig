const limine = @import("limine");
const mem = @import("kernel").lib.mem;
pub var kmapper: mem.Mapper = undefined;

pub fn init() void {
    mem.pmm.init();
    kmapper = mem.Mapper.create(mem.pmm.virt(asm volatile ("mov %%cr3, %[result]"
        : [result] "=r" (-> usize),
    ) & 0xfffffffffffffffe));
    const vmm = mem.VMM.create(0, 134217728, 0, &kmapper);
    _ = vmm;
}
