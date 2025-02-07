const kernel = @import("kernel");
const idt = kernel.drivers.idt;
const mem = kernel.lib.mem;
const cpu = kernel.drivers.cpu;
const smp = kernel.drivers.smp;
const timers = kernel.drivers.timers;
const std = @import("std");
const log = std.log.scoped(.sched);

const QUANTUM = 50;

pub var Task = kernel.Task{
    .name = "Scheduler",
    .init = init,
    .dependencies = &.{
        .{ .task = &kernel.lib.mem.KHeapTask },
        .{ .task = &kernel.drivers.smp.Task },
        .{ .task = &kernel.drivers.timers.Task },
    },
};

pub const Thread = struct {
    id: usize = 0,
    priority: u8,
    ef: cpu.Status,
    mapper: mem.Mapper,
    status: enum {
        ready,
        sleeping,
        dead,
    } = .ready,
};

const ThreadLinkedList = std.DoublyLinkedList(Thread);
const ThreadPriorityQueue = std.PriorityQueue(ThreadLinkedList, void, compareThreadPriorities);

var sched_vec: *idt.HandlerData = undefined;
var id_counter = std.atomic.Value(usize).init(0);
var lock = std.atomic.Value(bool).init(false);
var global_queue: ThreadPriorityQueue = undefined;
var delete_proc: ?*Thread = null;

var idle_thread: Thread = undefined;

fn init() kernel.Task.Ret {
    sched_vec = idt.allocate_handler(null);
    sched_vec.handler = schedule;
    global_queue = ThreadPriorityQueue.init(mem.kheap.allocator(), {});

    const kproc = Thread{
        .ef = undefined,
        .mapper = mem.kmapper,
        .priority = 100,
    };
    const stack = mem.kvmm.allocator().alloc(u8, 0x1000) catch return .failed;
    idle_thread = .{
        .priority = 100,
        .status = .ready,
        .mapper = mem.kmapper,
        .ef = .{ .iret_status = .{
            .cs = 0x8,
            .ss = 0x10,
            .rip = @intFromPtr(&idle_func),
            .rsp = @intFromPtr(stack.ptr) + 0x1000,
        } },
    };

    smp.cpu_info(null).cur_proc = enqueue(kproc);
    _ = enqueue(idle_thread);
    timers.callback(50_000_000, null, schedule) catch return .failed;
    return .success;
}

fn enqueue(thread: Thread) *Thread {
    const node = mem.kheap.allocator().create(ThreadLinkedList.Node) catch @panic("OOM");
    node.data = thread;
    node.data.id = id_counter.fetchAdd(1, .monotonic);
    while (lock.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    defer lock.store(false, .release);

    for (global_queue.items) |*queue| {
        if (queue.first.?.data.priority == thread.priority) {
            queue.append(node);
            return &node.data;
        }
    }
    var queue = ThreadLinkedList{};
    queue.append(node);
    global_queue.add(queue) catch @panic("OOM");

    return &node.data;
}

fn check_cur_thread(cpu_info: *smp.CPU) void {
    if (cpu_info.cur_proc) |cp| {
        if (cp.status == .dead) {
            // Handle dead thread
            delete_proc = cp;
            cpu_info.cur_proc = null;
        } else if (cp.status == .sleeping) {
            // Handle sleeping thread
            @panic("Handling sleeping threads unimplemented");
        }
    }
}

// Checks if any threads are ready to be scheduled to know whether to preempt current one
fn check_ready_threads(cpu_info: *smp.CPU) ?*Thread {
    if (cpu_info.cur_proc) |cp| {
        if (global_queue.count() > 0) {
            const pq = &global_queue.items[0];
            // SCHED_RR, account for timeslice/quantum
            if (pq.first.?.data.priority >= cp.priority) {
                return &pq.popFirst().?.data;
            }
        }
    } else {
        if (global_queue.count() == 0) return null;
        const proc = global_queue.items[0].popFirst().?;
        if (global_queue.items[0].len == 0) _ = global_queue.remove();
        return &proc.data;
    }
    return null;
}

pub fn schedule(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    _ = ctx;
    const cpu_info = smp.cpu_info(null);
    const alloc = mem.kheap.allocator();

    while (lock.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    defer lock.store(false, .release);

    if (delete_proc) |dp| {
        delete_proc = null;
        alloc.destroy(dp);
    }

    check_cur_thread(cpu_info);
    const next_thread = check_ready_threads(cpu_info);

    if (cpu_info.cur_proc != null and next_thread != null) {
        // Preempt current process and switch to next thread
        const cp = cpu_info.cur_proc.?;
        log.info("Current proc is {x}, switching to {x}", .{ @intFromPtr(cp), @intFromPtr(next_thread.?) });
        cp.ef = status.*;
        global_queue.items[0].append(@fieldParentPtr("data", cp));
    } else if (cpu_info.cur_proc != null) {
        return status;
    }

    if (next_thread) |nt| {
        cpu_info.cur_proc = nt;
        timers.callback(50_000_000, null, schedule) catch @panic("No timer available");
        cpu.set_cr3(@intFromPtr(nt.mapper.pml4));
        return &nt.ef;
    }
    @panic("idle thread not implemeneted");
    // return &idle_thread.ef;
}

fn idle_func() noreturn {
    while (true) asm volatile ("hlt");
}

fn compareThreadPriorities(_: void, a: ThreadLinkedList, b: ThreadLinkedList) std.math.Order {
    // Highest priority first, lowest priority last
    return std.math.order(b.first.?.data.priority, a.first.?.data.priority);
}
