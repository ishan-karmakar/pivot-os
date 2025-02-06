const kernel = @import("kernel");
const std = @import("std");
const log = std.log.scoped(.elf);
const mem = kernel.lib.mem;

const ELFHeader = extern struct {
    ident: [4]u8,
    bit: u8,
    endian: u8,
    hdr_version: u8,
    abi: u8,
    rsv: u64,
    type: u16,
    iset: u16,
    elf_version: u32,
    pentry_off: u64,
    phdr_offset: u64,
    shdr_offset: u64,
    flags: u32,
    hdr_size: u16,
    ent_size_phdr: u16,
    num_ent_phdr: u16,
    ent_size_shdr: u16,
    num_ent_shdr: u16,
    sec_idx_hdr_string: u16,
};

const ELFProgramHeader = extern struct {
    type: u32,
    flags: u32,
    p_offset: usize,
    p_vaddr: usize,
    p_paddr: usize,
    p_filesz: usize,
    p_memsz: usize,
    p_align: usize,
};

const ELFSectionHeader = extern struct {
    sh_name: u32,
    sh_type: u32,
    sh_flags: u64,
    sh_addr: u64,
    sh_offset: u64,
    sh_size: u64,
    sh_link: u32,
    sh_info: u32,
    sh_addralign: u64,
    sh_entsize: u64,
};

pub var Task = kernel.Task{
    .name = "ELF Loader",
    .init = null,
    .dependencies = &.{
        .{ .task = &mem.KMapperTask },
        .{ .task = &kernel.drivers.modules.Task },
    },
};

pub fn load(addr: usize) !void {
    const elf: *const ELFHeader = @ptrFromInt(addr);
    if (!std.mem.eql(u8, &elf.ident, &.{ 0x7F, 'E', 'L', 'F' })) return;
    log.info("ELF is valid", .{});

    if (elf.bit != 2) return error.Bit32;

    if (elf.abi != 0) return error.NotSYSV;

    if (elf.iset != 0x3E) return error.NotX86_64;

    var phdr: *const ELFProgramHeader = @ptrFromInt(addr + elf.phdr_offset);
    for (0..elf.num_ent_phdr) |_| {
        if (phdr.type == 1) {
            if (phdr.p_align != 0x1000) @panic("p_align != 0x1000");

            // If the flags are set to non writable we still of course want to write the data to it
            // First we map with a known flags, present + writable, then set remap with flags after
            // Not the most efficient but gets the job done
            const num_pages = try std.math.divCeil(usize, phdr.p_memsz, 0x1000);
            const vaddr_page = (phdr.p_vaddr / 0x1000) * 0x1000;
            for (0..num_pages) |i| {
                mem.kmapper.map(mem.pmm.frame(), vaddr_page + i * 0x1000, 0b11);
            }

            // TODO: Make more efficient, only need to zero last part of memory (total - filesz)
            @memset(@as([*]u8, @ptrFromInt(phdr.p_vaddr))[0..phdr.p_memsz], 0);
            @memcpy(@as([*]u8, @ptrFromInt(phdr.p_vaddr))[0..phdr.p_filesz], @as([*]u8, @ptrFromInt(addr + phdr.p_offset))[0..phdr.p_filesz]);

            var flags: usize = 0;
            if (phdr.flags & 0x1 == 0) flags |= (1 << 63);
            if (phdr.flags & 0x2 > 0) flags |= (1 << 1);
            if (phdr.flags & 0x4 > 0) flags |= 0b1; // If not readable we are treating as essentially not present
            for (0..num_pages) |i| {
                mem.kmapper.map(mem.pmm.frame(), vaddr_page + i * 0x1000, flags);
            }
        }

        phdr = @ptrFromInt(@intFromPtr(phdr) + elf.ent_size_phdr);
    }
    const func: *const fn () noreturn = @ptrFromInt(elf.pentry_off);
    func();
}
