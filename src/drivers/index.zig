pub const qemu = @import("qemu.zig");
pub const serial = @import("serial.zig");
pub const acpi = @import("acpi.zig");
pub const timers = @import("timers/index.zig");
pub const intctrl = @import("intctrl/index.zig");
pub const pci = @import("pci/index.zig");
pub const fb = @import("fb.zig");
pub const modules = @import("modules.zig");

// CPU stuff
pub const gdt = @import("gdt.zig");
pub const idt = @import("idt.zig");
pub const cpu = @import("cpu.zig");
pub const smp = @import("smp.zig");
pub const lapic = @import("lapic.zig");

comptime {
    _ = @import("uacpi.zig");
}
