const std = @import("std");
const kernel = @import("kernel");
const uacpi = @import("uacpi");
const pci = kernel.drivers.pci;
const acpi = kernel.drivers.acpi;
const serial = kernel.drivers.serial;
const log = @import("std").log.scoped(.ide);
const mem = kernel.lib.mem;
const cpu = kernel.drivers.cpu;
const timers = kernel.drivers.timers;

pub var PCIVTable = pci.VTable{
    .target_codes = &.{
        .{ .class_code = 0x1, .subclass_code = 0x1 },
    },
    .target_ret = undefined,
};

pub var PCITask = kernel.Task{
    .name = "IDE PCI Driver",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.drivers.timers.Task },
    },
};

// TODO: Change into enums?
const ATA_PRIMARY = 0;
const ATA_SECONDARY = 1;

const ATA_ER_ABRT = 0x4;

const ATA_REG_ERROR = 0x1;
const ATA_REG_CONTROL = 0xC;
const ATA_REG_HDDEVSEL = 0x6;
const ATA_REG_COMMAND = 0x7;
const ATA_REG_STATUS = 0x7;
const ATA_REG_LBA1 = 4;
const ATA_REG_LBA2 = 5;
const ATA_REG_DATA = 0;

const ATA_CMD_IDENTIFY = 0xEC;
const ATA_CMD_IDENTIFY_PACKET = 0xA1;

const ATA_SR_ERR = 1;
const ATA_SR_BSY = 0x80;
const ATA_SR_DRQ = 8;

const ATA_IDENT_CAPABILITIES = 98;
const ATA_IDENT_MODEL = 54;

const Drive = struct {
    pub const Type = enum {
        ATA,
        ATAPI,
    };

    dtype: Type,
};

const Controller = struct {
    channels: [2]ChannelRegisters,
    drives: []Drive,
};

const ChannelRegisters = struct {
    base: u16,
    ctrl: u16,
    bmide: u16,
    nIEN: u8 = 0,

    pub fn read(self: @This(), reg: u8) u8 {
        var result: u8 = undefined;
        if (reg > 7 and reg < 0xC) self.write(ATA_REG_CONTROL, 0x80 | self.nIEN);
        if (reg < 8) {
            result = serial.in(self.base + reg, u8);
        } else if (reg < 0xC) {
            result = serial.in(self.base + reg - 6, u8);
        } else if (reg < 0xE) {
            result = serial.in(self.ctrl + reg - 0xA, u8);
        } else if (reg < 0x16) {
            result = serial.in(self.bmide + reg - 0xE, u8);
        }
        if (reg > 7 and reg < 0xC) self.write(ATA_REG_CONTROL, self.nIEN);

        return result;
    }

    pub fn write(self: @This(), reg: u8, data: u8) void {
        if (reg > 7 and reg < 0xC) self.write(ATA_REG_CONTROL, 0x80 | self.nIEN);
        if (reg < 8) {
            serial.out(self.base + reg, data);
        } else if (reg < 0xC) {
            serial.out(self.base + reg - 6, data);
        } else if (reg < 0xE) {
            serial.out(self.ctrl + reg - 0xA, data);
        } else if (reg < 0x16) {
            serial.out(self.bmide + reg - 0xE, data);
        }
        if (reg > 7 and reg < 0xC) self.write(ATA_REG_CONTROL, self.nIEN);
    }

    pub fn read_buf(self: @This(), reg: u8, size: usize) []u8 {
        const buf = mem.kheap.allocator().alloc(u8, size) catch @panic("OOM");
        @memset(buf, 0);
        const quads = buf.len / 4;
        if (reg > 0x7 and reg < 0xC) self.write(ATA_REG_CONTROL, 0x80 | self.nIEN);

        if (reg < 0x8) {
            cpu.insd(self.base + reg, buf.ptr, quads);
        } else if (reg < 0xC) {
            cpu.insd(self.base + reg - 0x6, buf.ptr, quads);
        } else if (reg < 0xE) {
            cpu.insd(self.base + reg - 0xA, buf.ptr, quads);
        } else if (reg < 0x16) {
            cpu.insd(self.base + reg - 0xE, buf.ptr, quads);
        }

        if (reg > 0x7 and reg < 0xC) self.write(ATA_REG_CONTROL, self.nIEN);
        return buf;
    }
};

fn init() kernel.Task.Ret {
    const bmide = get_bmide(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr);
    const channels = mem.kheap.allocator().alloc(ChannelRegisters, 2) catch return .failed;
    channels[ATA_PRIMARY] = .{
        .base = get_primary_channel(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr),
        .ctrl = get_primary_channel_control_port(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr),
        .bmide = bmide,
    };
    channels[ATA_SECONDARY] = .{
        .base = get_secondary_channel(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr),
        .ctrl = get_secondary_channel_control_port(PCIVTable.target_ret.prog_if, PCIVTable.target_ret.addr),
        .bmide = bmide + 8,
    };

    channels[ATA_PRIMARY].write(ATA_REG_CONTROL, 2);
    channels[ATA_SECONDARY].write(ATA_REG_CONTROL, 2);

    var drives = std.ArrayList(Drive).init(mem.kheap.allocator());

    for (channels) |channel| {
        for (0..2) |drive| {
            channel.write(ATA_REG_HDDEVSEL, @intCast(0xA0 | (drive << 4)));
            timers.sleep(1_000_000);
            channel.write(ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            timers.sleep(1_000_000);

            if (channel.read(ATA_REG_STATUS) == 0) continue;

            var status: u8 = undefined;
            var dtype = Drive.Type.ATA;
            while (true) {
                status = channel.read(ATA_REG_STATUS);
                // I don't know if this is reliable or not
                // In testing, ATA_ER_ABRT was set when ATAPI instead of ATA
                const err = channel.read(ATA_REG_ERROR);
                if (status & ATA_SR_ERR > 0 and err & ATA_ER_ABRT > 0) {
                    const cl = channel.read(ATA_REG_LBA1);
                    const ch = channel.read(ATA_REG_LBA2);

                    if ((cl == 0x14 and ch == 0xEB) or (cl == 0x69 and ch == 0x96)) {
                        dtype = .ATAPI;
                    } else @panic("Unknown drive type");
                    channel.write(ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
                    timers.sleep(1_000_000);
                    break;
                }
                if (status & ATA_SR_BSY == 0 and status & ATA_SR_DRQ > 0) break;
            }
            log.info("Drive type: {}", .{dtype});
            const ident_buf = channel.read_buf(ATA_REG_DATA, 512);

            // var model: [41]u8 = undefined;
            // for (0..20) |i| {
            //     model[i * 2] = ident_buf[ATA_IDENT_MODEL + i * 2 + 1];
            //     model[i * 2 + 1] = ident_buf[ATA_IDENT_MODEL + i * 2];
            // }

            drives.append(.{
                .dtype = dtype,
            }) catch return .failed;

            mem.kheap.allocator().free(ident_buf);

            // log.info("Capabilities: {any}", .{buf[ATA_IDENT_CAPABILITIES .. ATA_IDENT_CAPABILITIES + 2]});
            // const capabilities = buf + ATA_IDENT_CAPABILITIES;
            // _ = capabilities;
        }
    }

    return .success;
}

inline fn get_primary_channel(prog_if: u8, addr: uacpi.uacpi_pci_address) u16 {
    return if (prog_if & 1 == 0) 0x1F0 else get_bar(addr, 0x10);
    // TODO: Use bit 1 to modify pci native/compatibility
}

inline fn get_primary_channel_control_port(prog_if: u8, addr: uacpi.uacpi_pci_address) u16 {
    return if (prog_if & 1 == 0) 0x3F6 else get_bar(addr, 0x14);
    // TODO: Use bit 1 to modify pci native/compatibility
}

inline fn get_secondary_channel(prog_if: u8, addr: uacpi.uacpi_pci_address) u16 {
    return if (prog_if & (1 << 2) == 0) 0x170 else get_bar(addr, 0x18);
}

inline fn get_secondary_channel_control_port(prog_if: u8, addr: uacpi.uacpi_pci_address) u16 {
    return if (prog_if & (1 << 2) == 0) 0x376 else get_bar(addr, 0x1C);
}

inline fn get_bmide(prog_if: u8, addr: uacpi.uacpi_pci_address) u16 {
    return if (prog_if & (1 << 7) == 0) 0 else get_bar(addr, 0x20);
}

fn get_bar(addr: uacpi.uacpi_pci_address, off: u8) u16 {
    const bar = pci.read_reg(addr, off);
    if (bar & 1 == 0) @panic("IDE Bar is in physical memory");
    return @intCast(bar & 0xFFFC);
}
