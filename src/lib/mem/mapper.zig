const kernel = @import("kernel");
const log = @import("std").log.scoped(.mapper);
const mem = kernel.lib.mem;
const math = @import("std").math;
const Mutex = kernel.lib.Mutex;
pub const Table = *[512]u64;

const HP_SIZE = 0x20000;
const SIGN_MASK: usize = 0x000ffffffffff00;

pml4: Table,
// mutex: Mutex = Mutex{},

pub fn create(tbl: usize) @This() {
    log.debug("Creating mapper with PML4 at 0x{x}", .{tbl});
    return .{ .pml4 = @ptrFromInt(tbl) };
}

pub fn map(self: *@This(), phys: usize, virt: usize, flags: u64) void {
    const p4_ent = (virt >> 39) & 0x1FF;
    const p3_ent = (virt >> 30) & 0x1FF;
    const p2_ent = (virt >> 21) & 0x1FF;
    const p1_ent = (virt >> 12) & 0x1FF;

    // self.mutex.lock();
    // defer self.mutex.unlock();
    const p3_tbl = next_table(&self.pml4[p4_ent]);
    const p2_tbl = next_table(&p3_tbl[p3_ent]);
    const hp_works = phys % HP_SIZE == virt % HP_SIZE;
    var p1_tbl: Table = block: {
        const ent = p2_tbl[p2_ent];
        if (ent & 1 > 0) {
            if (ent & (1 << 7) > 0) {
                if ((phys / HP_SIZE * HP_SIZE == (ent & SIGN_MASK))) return;
                const frm = mem.pmm.frame();
                const table: Table = @ptrFromInt(mem.virt(frm));
                for (0..512) |i| {
                    table[i] = ((ent & SIGN_MASK) + i * 0x1000) | (ent & ~(SIGN_MASK | (1 << 7)));
                }
                p2_tbl[p2_ent] = frm | 0b11 | (1 << 63);
                break :block table;
            }
        } else if (hp_works) {
            // TODO: For ACPI, this isn't working. Figure out why, and alternative so we can use this optimization
            // p2_tbl[p2_ent] = ((math.divFloor(usize, phys, HP_SIZE) catch unreachable) * HP_SIZE) | flags | 1 | (1 << 7);
            // log.info("map: 0x{x}", .{p2_tbl[p2_ent]});
            // return;
        }
        break :block next_table(&p2_tbl[p2_ent]);
    };
    const ent = phys | flags | 1;
    if (ent != p1_tbl[p1_ent]) p1_tbl[p1_ent] = ent;
}

pub fn translate(self: *@This(), virt: usize) ?usize {
    const p4_ent = (virt >> 39) & 0x1FF;
    const p3_ent = (virt >> 30) & 0x1FF;
    const p2_ent = (virt >> 21) & 0x1FF;
    const p1_ent = (virt >> 12) & 0x1FF;
    // FIXME: This is not efficient at all - consider readers writer lock implementation
    // Technically right now it shouldn't cause a problem because it will only be called from single threaded envs
    // self.mutex.lock();
    // defer self.mutex.unlock();
    const p3_tbl: Table = if (self.pml4[p4_ent] & 1 > 0) @ptrFromInt(mem.virt(self.pml4[p4_ent] & SIGN_MASK)) else return null;
    const p2_tbl: Table = if (p3_tbl[p3_ent] & 1 > 0) @ptrFromInt(mem.virt(p3_tbl[p3_ent] & SIGN_MASK)) else return null;
    const p1_tbl: Table = block: {
        const addr = p2_tbl[p2_ent] & SIGN_MASK;
        if (p2_tbl[p2_ent] & 1 == 0) return null;
        if (p2_tbl[p2_ent] & (1 << 7) > 0) {
            return addr + virt % 0x20000;
        } else break :block @ptrFromInt(mem.virt(addr));
    };
    return if (p1_tbl[p1_ent] & 1 > 0) p1_tbl[p1_ent] & SIGN_MASK else null;
}

// TODO: Unmap function, is needed?

fn next_table(entry: *u64) Table {
    if ((entry.* & 1) > 0) {
        return @ptrFromInt(mem.virt(entry.* & SIGN_MASK));
    } else {
        const frm = mem.pmm.frame();
        entry.* = frm | 0b11 | (1 << 63); // Writable, present, no execute
        const table: Table = @ptrFromInt(mem.virt(frm));
        @memset(table, 0);
        return table;
    }
}
