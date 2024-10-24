const log = @import("std").log.scoped(.mapper);
const pmm = @import("kernel").lib.mem.pmm;
const Self = @This();
const Table = *[512]u64;

const HP_SIZE = 0x20000;
const SIGN_MASK: usize = 0x000ffffffffff00;

pml4: Table,

pub fn create(tbl: usize) Self {
    log.debug("Creating mapper with PML4 at 0x{x}", .{tbl});
    return Self{ .pml4 = @ptrFromInt(tbl) };
}

pub fn map(self: Self, phys: usize, virt: usize, flags: u64) void {
    const p4_ent = (virt >> 39) & 0x1FF;
    const p3_ent = (virt >> 30) & 0x1FF;
    const p2_ent = (virt >> 21) & 0x1FF;
    const p1_ent = (virt >> 12) & 0x1FF;

    const p3_tbl = next_table(&self.pml4[p4_ent]);
    const p2_tbl = next_table(&p3_tbl[p3_ent]);
    const hp_works = phys % HP_SIZE == virt % HP_SIZE;
    var p1_tbl: Table = block: {
        if ((p2_tbl[p2_ent] & (1 << 8)) > 0) {
            if ((phys / HP_SIZE * HP_SIZE) == (p2_ent & SIGN_MASK)) {
                return;
            } else {
                const frm = pmm.frame();
                const table: Table = @ptrFromInt(pmm.virt(frm));
                for (0..512) |i| {
                    table[i] = ((p2_tbl[p2_ent] & SIGN_MASK) + i * 0x1000) | (p2_tbl[p2_ent] & ~(SIGN_MASK | (1 << 8)));
                }
                p2_tbl[p2_ent] = frm | 0b11 | (1 << 63);
                break :block table;
            }
        } else {
            if ((p2_tbl[p2_ent] & 1) > 0 and hp_works) {
                p2_tbl[p2_ent] = (phys / HP_SIZE * HP_SIZE) | 0b11 | (1 << 63) | (1 << 8);
                return;
            } else {
                break :block next_table(&p2_tbl[p2_ent]);
            }
        }
    };
    p1_tbl[p1_ent] = phys | flags | 1;
}

fn next_table(entry: *u64) Table {
    if ((entry.* & 1) > 0) {
        return @ptrFromInt(pmm.virt(entry.* & SIGN_MASK));
    } else {
        const frm = pmm.frame();
        entry.* = frm | 0b11 | (1 << 63); // Writable, present, no execute
        const table: Table = @ptrFromInt(pmm.virt(frm));
        @memset(table, 0);
        return table;
    }
}
