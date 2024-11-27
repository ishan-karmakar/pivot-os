const kernel = @import("kernel");
const Process = kernel.lib.Process;
const AutoHashMap = @import("std").AutoHashMap;
const Mutex = kernel.lib.Mutex;
const mem = kernel.lib.mem;
const smp = kernel.drivers.smp;
const cpu = kernel.drivers.cpu;
const lapic = kernel.drivers.lapic;
const idt = kernel.drivers.idt;
const log = @import("std").log.scoped(.sched);
const std = @import("std");

const ReadyQueue = AutoHashMap(u8, *Process);

// TODO: Separate into kernel and user quantums (50ms, 25 ms)
const QUANTUM = 50;
pub const SCHED_VEC = 0x30;
var ready_queue: ?*Process = null;
var ready_queue_end: ?*Process = null; // End of MAX priority processes in queue
var idle_proc: Process = undefined;
pub var lock: Mutex = .{};

pub fn init() void {
    // FIXME: Idle proc should have no extra rules or edge cases. Change to simply add to ready queue w/ lowest priority

    const kproc = mem.kheap.allocator().create(Process) catch @panic("OOM");
    kproc.* = .{
        .ef = undefined,
        .mapper = mem.kmapper,
        .priority = 100,
        .vmm = null,
        .stack = null,
    };

    const stack = mem.kheap.allocator().alloc(u8, 1024) catch @panic("OOM");
    idle_proc = .{
        .ef = .{ .iret_status = .{
            .cs = 0x8,
            .ss = 0x10,
            .rip = @intFromPtr(&idle),
            .rsp = @intFromPtr(stack.ptr) + 1024,
        } },
        .stack = stack,
        .save_ef = false,
        .priority = 0,
        .vmm = null,
        .mapper = mem.kmapper,
    };
    idt.set_ent(SCHED_VEC, idt.create_irq(SCHED_VEC, "schedule"));
    for (0..smp.cpu_count()) |i| smp.cpu_info(i).cur_proc = &idle_proc;
    smp.cpu_info(null).cur_proc = kproc;
}

pub fn queue(proc: *Process) void {
    lock.lock();
    add_to_queue(proc);
    lock.unlock();
}

fn add_to_queue(proc: *Process) void {
    if (ready_queue) |rq| {
        if (proc.priority == rq.priority) {
            ready_queue_end.?.next = proc;
            ready_queue_end = proc;
        } else {
            var prev = ready_queue_end.?;
            while (prev.next != null and prev.next.?.priority >= proc.priority) prev = prev.next.?;
            proc.next = prev.next;
            prev.next = proc;
        }
    } else {
        proc.next = null;
        ready_queue = proc;
        ready_queue_end = proc;
    }
}

fn load_proc(proc: *Process, cpu_info: *smp.CPU) *const cpu.Status {
    cpu_info.cur_proc = proc;
    cpu_info.timeslice = 0;
    cpu.set_cr3(mem.phys(@intFromPtr(proc.mapper.pml4)));
    return &proc.ef;
}

fn check_rq(cpu_info: *smp.CPU, status: *const cpu.Status) ?*const cpu.Status {
    if (ready_queue) |rq| {
        if (rq.priority < cpu_info.cur_proc.priority or (rq.priority == cpu_info.cur_proc.priority and @atomicLoad(usize, &cpu_info.timeslice, .unordered) < QUANTUM)) return null;
        ready_queue = rq.next;
        if (cpu_info.cur_proc.save_ef) cpu_info.cur_proc.ef = status.*;
        add_to_queue(cpu_info.cur_proc);
        return load_proc(rq, cpu_info);
    }
    return null;
}

fn check_delete_proc(cpu_info: *smp.CPU) ?*const cpu.Status {
    if (cpu_info.delete_proc) |dp| {
        cpu_info.delete_proc = null;
        const alloc = mem.kheap.allocator();
        if (dp.stack) |stack| alloc.free(stack);
        alloc.destroy(dp);
    }
    if (cpu_info.cur_proc.status == .dead) {
        cpu_info.delete_proc = cpu_info.cur_proc;
        if (ready_queue) |rq| {
            ready_queue = rq.next;
            return load_proc(rq, cpu_info);
        } else return load_proc(&idle_proc, cpu_info);
    }
    return null;
}

pub export fn schedule(status: *const cpu.Status, vec: usize) *const cpu.Status {
    const cpu_info = smp.cpu_info(null);
    if (vec == kernel.drivers.timers.lapic.TIMER_VEC) _ = @atomicRmw(usize, &cpu_info.timeslice, .Add, 1, .monotonic);
    if (@cmpxchgStrong(bool, &lock.locked, false, true, .acquire, .monotonic) == true) return status;
    defer lock.unlock();
    if (vec == kernel.drivers.timers.lapic.TIMER_VEC) {
        if (check_rq(cpu_info, status)) |s| return s;
        return status;
    } else {
        // If manually called, some event happened
        if (check_delete_proc(cpu_info)) |s| return s;
        if (check_rq(cpu_info, status)) |s| return s;
        return load_proc(&idle_proc, cpu_info);
    }
}

pub inline fn yield() void {
    asm volatile ("int %[vec]"
        :
        : [vec] "i" (SCHED_VEC),
    );
}

fn idle() noreturn {
    while (true) {}
}
