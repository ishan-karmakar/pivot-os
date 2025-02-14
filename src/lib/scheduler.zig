const kernel = @import("kernel");
const idt = kernel.drivers.idt;
const mem = kernel.lib.mem;
const cpu = kernel.drivers.cpu;
const smp = kernel.lib.smp;
const timers = kernel.drivers.timers;
const syscalls = kernel.lib.syscalls;
const std = @import("std");
const log = std.log.scoped(.sched);

const QUANTUM = 50_000_000;

pub var Task = kernel.Task{
    .name = "Scheduler",
    .init = init,
    .dependencies = &.{
        .{ .task = &mem.KHeapTask },
        .{ .task = &smp.Task },
        .{ .task = &timers.Task },
        .{ .task = &syscalls.Task },
    },
};

pub const Thread = struct {
    id: usize = 0,
    priority: u8,
    ef: cpu.Status,
    mapper: mem.Mapper,
    quantum_end: usize = 0, // Absolute time of when thread should end
    status: enum {
        ready,
        sleeping,
        dead,
    } = .ready,
};

const SleepThread = struct {
    thread: *Thread,
    wakeup: usize,
};

const ThreadLinkedList = std.DoublyLinkedList(Thread);
const ThreadPriorityQueue = std.PriorityQueue(ThreadLinkedList, void, compareThreadPriorities);

const SleepPriorityQueue = std.PriorityQueue(SleepThread, void, compareSleepThreads);

var sched_vec: *idt.HandlerData = undefined;
var id_counter = std.atomic.Value(usize).init(0);
var lock = std.atomic.Value(bool).init(false);
var global_queue: ThreadPriorityQueue = undefined;
var sleep_queue: SleepPriorityQueue = undefined;
var delete_proc: ?*Thread = null;

var idle_thread_ef: cpu.Status = .{
    .iret_status = .{
        .cs = 0x8,
        .ss = 0x10,
        .rip = undefined,
        .rsp = undefined,
    },
};

fn init() kernel.Task.Ret {
    syscalls.register_syscall(syscalls.SYSCALLS.EXIT, syscall_exit);
    syscalls.register_syscall(syscalls.SYSCALLS.SLEEP, syscall_sleep);
    sched_vec = idt.allocate_handler(null);
    sched_vec.handler = schedule;
    global_queue = ThreadPriorityQueue.init(mem.kheap.allocator(), {});
    sleep_queue = SleepPriorityQueue.init(mem.kheap.allocator(), {});

    const kproc = Thread{
        .ef = undefined,
        .mapper = mem.kmapper,
        .priority = 100,
        .quantum_end = timers.time() + QUANTUM,
    };

    const stack = mem.kvmm.allocator().alloc(u8, 0x1000) catch return .failed;
    idle_thread_ef.iret_status.rsp = @intFromPtr(stack.ptr) + 0x1000;
    idle_thread_ef.iret_status.rip = @intFromPtr(&idle_func);

    smp.cpu_info(null).cur_proc = create_thread(kproc);
    return .success;
}

fn enqueue_no_preempt(thread: *Thread) void {
    const node: *ThreadLinkedList.Node = @fieldParentPtr("data", thread);

    for (global_queue.items) |*queue| {
        if (queue.first.?.data.priority == thread.priority) {
            queue.append(node);
            return;
        }
    }
    var queue = ThreadLinkedList{};
    queue.append(node);
    global_queue.add(queue) catch @panic("OOM");
}

pub fn enqueue(thread: *Thread) void {
    enqueue_no_preempt(thread);
    for (0..smp.cpu_count()) |i| {
        const cproc = smp.cpu_info(i).cur_proc;
        if (cproc == null) return kernel.drivers.lapic.ipi(@intCast(i), idt.handler2vec(sched_vec));
    }
    for (0..smp.cpu_count()) |i| {
        // If it made it here, then all threads have a process running
        const cproc = smp.cpu_info(i).cur_proc.?;
        if (thread.priority > cproc.priority) return kernel.drivers.lapic.ipi(@intCast(i), idt.handler2vec(sched_vec));
    }
}

pub fn create_thread(thread: Thread) *Thread {
    const node = mem.kheap.allocator().create(ThreadLinkedList.Node) catch @panic("OOM");
    node.data = thread;
    node.data.id = id_counter.fetchAdd(1, .monotonic);

    return &node.data;
}

fn check_cur_thread(cpu_info: *smp.CPU) void {
    if (cpu_info.cur_proc) |cp| {
        if (cp.status == .dead) {
            // Handle dead thread
            delete_proc = cp;
            cpu_info.cur_proc = null;
        } else if (cp.status == .sleeping) {}
    }
}

// Checks if any threads are ready to be scheduled to know whether to preempt current one
fn check_ready_threads(cpu_info: *smp.CPU) ?*Thread {
    defer {
        if (global_queue.count() > 0 and global_queue.peek().?.len == 0) _ = global_queue.remove();
    }
    if (cpu_info.cur_proc) |cp| {
        if (global_queue.count() > 0) {
            const pq = &global_queue.items[0];
            // SCHED_RR, account for timeslice/quantum
            const pq_prio = pq.first.?.data.priority;
            if (pq_prio > cp.priority or (pq_prio == cp.priority and cp.quantum_end <= timers.time())) {
                return &pq.popFirst().?.data;
            }
        }
    } else {
        if (global_queue.count() == 0) return null;
        const proc = global_queue.items[0].popFirst().?;
        return &proc.data;
    }
    return null;
}

fn check_sleep_threads() void {
    const time = timers.time();
    while (true) {
        const t = sleep_queue.peek() orelse return;
        if (t.wakeup <= time) enqueue(sleep_queue.remove().thread) else return;
    }
}

// TODO: Create separate method to get delta for next call
pub fn schedule(_: ?*anyopaque, status: *cpu.Status) *const cpu.Status {
    const cpu_info = smp.cpu_info(null);

    while (lock.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    defer lock.store(false, .release);

    // We have to do EOI here because we call this interrupt handler with the IPI and IPI expects an EOI
    // This results in normal timer handler calling EOI twice, but its probably fine
    // Scheduler is pretty tightly knit with LAPIC
    defer kernel.drivers.intctrl.eoi(0);

    check_sleep_threads();
    check_cur_thread(cpu_info);
    const next_thread = check_ready_threads(cpu_info);

    if (cpu_info.cur_proc != null and next_thread != null) {
        // Preempt current process and switch to next thread
        const cp = cpu_info.cur_proc.?;
        cp.ef = status.*;
        enqueue(cp);
    } else if (cpu_info.cur_proc != null) {
        return status;
    }

    if (next_thread) |nt| {
        nt.quantum_end = timers.time() + QUANTUM;
        cpu_info.cur_proc = nt;
        cpu.set_cr3(mem.phys(@intFromPtr(nt.mapper.pml4)));
        timers.callback(QUANTUM, null, schedule);
        return &nt.ef;
    }

    // If the sleep queue is not empty, we set a callback for when the first thread will wake up
    if (sleep_queue.peek()) |t| {
        timers.callback(t.wakeup - timers.time(), null, schedule);
    }
    return &idle_thread_ef;
}

fn syscall_exit(status: *cpu.Status) *const cpu.Status {
    const cpu_info = smp.cpu_info(null);
    while (lock.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    if (delete_proc) |dp| {
        delete_proc = null;
        mem.kheap.allocator().destroy(@as(*ThreadLinkedList.Node, @fieldParentPtr("data", dp)));
    }
    delete_proc = cpu_info.cur_proc;
    cpu_info.cur_proc = null;
    lock.store(false, .release);

    return schedule(undefined, status);
}

fn syscall_sleep(status: *cpu.Status) *const cpu.Status {
    const cpu_info = smp.cpu_info(null);
    while (lock.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}
    const cp = cpu_info.cur_proc.?;
    cp.ef = status.*;
    sleep_queue.add(.{
        .thread = cp,
        .wakeup = timers.time() + status.rsi,
    }) catch @panic("OOM");
    cpu_info.cur_proc = null;
    lock.store(false, .release);

    return schedule(undefined, status);
}

fn idle_func() noreturn {
    while (true) asm volatile ("hlt");
}

fn compareThreadPriorities(_: void, a: ThreadLinkedList, b: ThreadLinkedList) std.math.Order {
    // Highest priority first, lowest priority last
    return std.math.order(b.first.?.data.priority, a.first.?.data.priority);
}

fn compareSleepThreads(_: void, a: SleepThread, b: SleepThread) std.math.Order {
    return std.math.order(a.wakeup, b.wakeup);
}
