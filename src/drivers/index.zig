pub const gdt = @import("gdt.zig");
pub const idt = @import("idt.zig");
pub const serial = @import("serial.zig");
pub const smp = @import("smp.zig");
pub const term = @import("term.zig");
pub const qemu = @import("qemu.zig");
pub const pic = @import("pic.zig");
pub const timers = @import("timers/index.zig");
pub const lapic = @import("lapic.zig");
pub const ioapic = @import("ioapic.zig");
pub const cpu = @import("cpu.zig");
pub const acpi = @import("acpi.zig");

comptime {
    _ = @import("uacpi.zig");
}
