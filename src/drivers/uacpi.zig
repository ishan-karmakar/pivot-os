const uacpi = @import("uacpi");
const serial = @import("kernel").drivers.serial;
const mem = @import("kernel").lib.mem;
const timers = @import("kernel").drivers.timers;
const idt = @import("kernel").drivers.idt;
const ioapic = @import("kernel").drivers.ioapic;
const pci = @import("kernel").drivers.pci;
const math = @import("std").math;
const log = @import("std").log.scoped(.uacpi);
const Mutex = @import("std").Thread.Mutex;
const limine = @import("limine");
const std = @import("std");

const UACPI_CTX = std.meta.Tuple(&.{ uacpi.uacpi_interrupt_handler, uacpi.uacpi_handle });

var RSDP_REQUEST: limine.RsdpRequest = .{};
const ACPI_MAX_HANDLERS = 3;
var handler_data: [ACPI_MAX_HANDLERS]UACPI_CTX = .{.{ null, null }} ** 3;

export fn uacpi_kernel_initialize(lvl: uacpi.uacpi_init_level) uacpi.uacpi_status {
    switch (lvl) {
        uacpi.UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED => {
            ioapic.init();
            idt.set_ent(0x30, idt.create_irq(0, "acpi_handler"));
            idt.set_ent(0x31, idt.create_irq(1, "acpi_handler"));
            idt.set_ent(0x32, idt.create_irq(2, "acpi_handler"));
        },
        uacpi.UACPI_INIT_LEVEL_NAMESPACE_LOADED => {
            pci.init();
        },
        else => {},
    }
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_deinitialize() void {}

export fn uacpi_kernel_raw_memory_read(addr: uacpi.uacpi_phys_addr, bw: uacpi.uacpi_u8, out: [*c]uacpi.uacpi_u64) uacpi.uacpi_status {
    out.* = switch (bw) {
        1 => @intCast(@as(*const u8, @ptrFromInt(addr)).*),
        2 => @intCast(@as(*const u16, @ptrFromInt(addr)).*),
        4 => @intCast(@as(*const u32, @ptrFromInt(addr)).*),
        8 => @as(*const u64, @ptrFromInt(addr)).*,
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    };
    @panic("uacpi_kernel_raw_memory_read is unimplemented");
    // return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_raw_memory_write(addr: uacpi.uacpi_phys_addr, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    switch (bw) {
        1 => @as(*u8, @ptrFromInt(addr)).* = @truncate(val),
        2 => @as(*u16, @ptrFromInt(addr)).* = @truncate(val),
        4 => @as(*u32, @ptrFromInt(addr)).* = @truncate(val),
        8 => @as(*u64, @ptrFromInt(addr)).* = val,
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    }
    @panic("uacpi_kernel_raw_memory_write is unimplemented");
    // return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_raw_io_read(_addr: uacpi.uacpi_io_addr, bw: uacpi.uacpi_u8, out: [*c]uacpi.uacpi_u64) uacpi.uacpi_status {
    const addr: u16 = @intCast(_addr);
    out.* = switch (bw) {
        1 => serial.in(addr, u8),
        2 => serial.in(addr, u16),
        4 => serial.in(addr, u32),
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    };
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_raw_io_write(_addr: uacpi.uacpi_io_addr, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    const addr: u16 = @intCast(_addr);
    switch (bw) {
        1 => serial.out(addr, @as(u8, @truncate(val))),
        2 => serial.out(addr, @as(u16, @truncate(val))),
        4 => serial.out(addr, @as(u32, @truncate(val))),
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    }
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_map(addr: uacpi.uacpi_io_addr, _: uacpi.uacpi_size, out: [*c]uacpi.uacpi_handle) uacpi.uacpi_status {
    out.* = @ptrFromInt(addr);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_io_unmap(_: uacpi.uacpi_handle) void {}

export fn uacpi_kernel_io_read(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, bw: uacpi.uacpi_u8, val: [*c]uacpi.uacpi_u64) uacpi.uacpi_status {
    return uacpi_kernel_raw_io_read(@intFromPtr(handle) + off, bw, val);
}

export fn uacpi_kernel_io_write(handle: uacpi.uacpi_handle, off: uacpi.uacpi_size, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    return uacpi_kernel_raw_io_write(@intFromPtr(handle) + off, bw, val);
}

export fn uacpi_kernel_pci_read(addr: [*c]uacpi.uacpi_pci_address, off: uacpi.uacpi_size, bw: uacpi.uacpi_u8, val: [*c]uacpi.uacpi_u64) uacpi.uacpi_status {
    _ = addr;
    _ = off;
    _ = bw;
    _ = val;
    @panic("uacpi_kernel_pci_read is unimplemented");
    // return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_pci_write(addr: [*c]uacpi.uacpi_pci_address, off: uacpi.uacpi_size, bw: uacpi.uacpi_u8, val: uacpi.uacpi_u64) uacpi.uacpi_status {
    _ = addr;
    _ = off;
    _ = bw;
    _ = val;
    @panic("uacpi_kernel_pci_write is unimplemented");
    // return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_schedule_work(wtype: uacpi.uacpi_work_type, handler: uacpi.uacpi_work_handler, handle: uacpi.uacpi_handle) uacpi.uacpi_status {
    _ = wtype;
    _ = handler;
    _ = handle;
    @panic("uacpi_kernel_schedule_work is unimplemented");
    // return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_wait_for_work_completion() uacpi.uacpi_status {
    @panic("uacpi_kernel_wait_for_work_completion is unimplemented");
    // return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_get_thread_id() uacpi.uacpi_thread_id {
    return null; // 0
}

export fn uacpi_kernel_alloc(size: uacpi.uacpi_size) ?*anyopaque {
    const ptr = mem.kheap.allocator().alignedAlloc(u8, 8, size) catch @panic("OOM");
    return @ptrCast(ptr.ptr);
}

export fn uacpi_kernel_calloc(count: uacpi.uacpi_size, size: uacpi.uacpi_size) ?*anyopaque {
    // We are just using a alignment of 8 here as a constant. Ideally, we should test what the minimum needed is.
    const a = mem.kheap.allocator().alignedAlloc(u8, 8, count * size) catch @panic("OOM");
    for (a) |*b| b.* = 0;
    return @ptrCast(a.ptr);
}

export fn uacpi_kernel_free(_ptr: ?*anyopaque, size: uacpi.uacpi_size) void {
    const ptr: [*]const u8 = @ptrCast(_ptr orelse return);
    mem.kheap.allocator().free(ptr[0..size]);
}

export fn uacpi_kernel_map(addr: uacpi.uacpi_phys_addr, size: uacpi.uacpi_size) ?*anyopaque {
    const virt = mem.virt(addr);
    const num_pages = math.divCeil(usize, size, 0x1000) catch unreachable;
    for (0..num_pages) |p| {
        mem.kmapper.map(addr + p * 0x1000, virt + p * 0x1000, (1 << 63) | 0b11);
    }
    return @ptrFromInt(virt);
}

export fn uacpi_kernel_unmap(addr: ?*anyopaque, size: uacpi.uacpi_size) void {
    log.debug("uacpi_kernel_unmap: 0x{x}, {}", .{ @intFromPtr(addr.?), size });
}

export fn uacpi_kernel_get_rsdp(out: [*c]uacpi.uacpi_phys_addr) uacpi.uacpi_status {
    const req = RSDP_REQUEST.response orelse @panic("Limine RSDP request is null");
    out.* = @intFromPtr(req.address);
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_get_ticks() uacpi.uacpi_u64 {
    @panic("uacpi_kernel_get_ticks unimplemented");
}

export fn uacpi_kernel_uninstall_interrupt_handler(_: uacpi.uacpi_interrupt_handler, _: uacpi.uacpi_handle) uacpi.uacpi_status {
    @panic("uacpi_kernel_uninstall_interrupt_handler unimplemented");
    // return uacpi.UACPI_STATUS_UNIMPLEMENTED;
}

export fn uacpi_kernel_install_interrupt_handler(irq: uacpi.uacpi_u32, handler: uacpi.uacpi_interrupt_handler, ctx: uacpi.uacpi_handle, _: [*c]uacpi.uacpi_handle) uacpi.uacpi_status {
    log.info("uacpi_kernel_install_interrupt_handler", .{});
    for (0.., &handler_data) |i, *d| {
        if (d[0] == null) {
            d.* = .{ handler, ctx };
            ioapic.set(@intCast(0x30 + i), @intCast(irq), 0, 0);
            ioapic.mask(@intCast(irq), false);
            return uacpi.UACPI_STATUS_OK;
        }
    }
    @panic("Ran out of interrupt handlers for uACPI");
}

export fn uacpi_kernel_create_event() uacpi.uacpi_handle {
    return @ptrCast(mem.kheap.allocator().create(bool) catch @panic("OOM"));
}

export fn uacpi_kernel_create_spinlock() uacpi.uacpi_handle {
    return uacpi_kernel_create_mutex();
}

export fn uacpi_kernel_lock_spinlock(_: uacpi.uacpi_handle) uacpi.uacpi_cpu_flags {
    @panic("uacpi_kernel_lock_spinlock unimplemented");
}

export fn uacpi_kernel_signal_event(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_signal_event unimplemented");
}

export fn uacpi_kernel_unlock_spinlock(_: uacpi.uacpi_handle, _: uacpi.uacpi_cpu_flags) void {
    @panic("uacpi_kernel_unlock_spinlock unimplemented");
}

export fn uacpi_kernel_sleep(ms: uacpi.uacpi_u64) void {
    _ = ms;
    @panic("uacpi_kernel_sleep unimplemented");
    // timers.sleep(ms);
}

export fn uacpi_kernel_stall(_: uacpi.uacpi_u8) void {
    uacpi_kernel_sleep(1);
}

export fn uacpi_kernel_reset_event(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_reset_event unimplemented");
}

export fn uacpi_kernel_handle_firmware_request(request: [*c]uacpi.uacpi_firmware_request) uacpi.uacpi_status {
    switch (request.*.type) {
        uacpi.UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT => log.warn("Ignoring AML breakpoint - CTX: 0x{?}", .{request.*.unnamed_0.breakpoint.ctx}),
        uacpi.UACPI_FIRMWARE_REQUEST_TYPE_FATAL => log.err("Fatal firmware error - type: {}, code: {}, arg: {}", .{
            request.*.unnamed_0.fatal.type,
            request.*.unnamed_0.fatal.code,
            request.*.unnamed_0.fatal.arg,
        }),
        else => return uacpi.UACPI_STATUS_TYPE_MISMATCH,
    }
    return uacpi.UACPI_STATUS_OK;
}

export fn uacpi_kernel_wait_for_event(_: uacpi.uacpi_handle, _: uacpi.uacpi_u16) uacpi.uacpi_bool {
    @panic("uacpi_kernel_wait_for_event unimplemented");
}

// TODO: Handle timeout
export fn uacpi_kernel_acquire_mutex(handle: uacpi.uacpi_handle, _: uacpi.uacpi_u16) uacpi.uacpi_bool {
    const mutex: *bool = @ptrCast(handle);
    while (@cmpxchgWeak(bool, mutex, false, true, .acquire, .monotonic) != null) {}
    return true;
}

export fn uacpi_kernel_create_mutex() uacpi.uacpi_handle {
    const mutex = mem.kheap.allocator().create(bool) catch @panic("OOM");
    mutex.* = false;
    return @ptrCast(mutex);
}

export fn uacpi_kernel_free_mutex(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_free_mutex unimplemented");
}

export fn uacpi_kernel_release_mutex(handle: uacpi.uacpi_handle) void {
    const mutex: *bool = @ptrCast(handle);
    @atomicStore(bool, mutex, false, .release);
}

export fn uacpi_kernel_log(level: uacpi.uacpi_log_level, msg: [*c]const uacpi.uacpi_char) void {
    var str: []const u8 = std.mem.span(msg);
    str = str[0 .. str.len - 1];
    switch (level) {
        uacpi.UACPI_LOG_DEBUG, uacpi.UACPI_LOG_TRACE => log.debug("{s}", .{str}),
        uacpi.UACPI_LOG_INFO => log.info("{s}", .{str}),
        uacpi.UACPI_LOG_WARN => log.warn("{s}", .{str}),
        uacpi.UACPI_LOG_ERROR => log.err("{s}", .{str}),
        else => @panic("uacpi_kernel_log received invalid log level"),
    }
}

export fn uacpi_kernel_free_spinlock(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_free_spinlock unimplemented");
}

export fn uacpi_kernel_free_event(_: uacpi.uacpi_handle) void {
    @panic("uacpi_kernel_free_event unimplemented");
}

export fn acpi_handler(status: *const idt.Status, idx: u8) *const idt.Status {
    log.info("acpi_handler: handler {} triggered", .{idx});
    const handler = handler_data[idx][0] orelse @panic("Handler triggered when no uACPI handler registered");
    const ctx = handler_data[idx][1] orelse @panic("Handler triggered when no uACPI handler context available");
    if (handler(ctx) != uacpi.UACPI_INTERRUPT_HANDLED) @panic("uACPI handler did not handle interrupt");
    return status;
}
