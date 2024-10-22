const limine = @import("limine");
const mem = @import("kernel").lib.mem;
pub var kmapper: mem.Mapper = undefined;

pub fn init() void {
    mem.pmm.init();
    kmapper = mem.Mapper.create(asm volatile ("mov %%cr3, %[result]"
        : [result] "=r" (-> usize),
    ) & 0xfffffffffffffffe);
    kmapper.map(0x1000, 0x1000, 0);
}
