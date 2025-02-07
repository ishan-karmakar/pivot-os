const kernel = @import("kernel");
const idt = kernel.drivers.idt;
const mem = kernel.lib.mem;
const cpu = kernel.drivers.cpu;
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
    ef: ?cpu.Status,
    mapper: mem.Mapper,
    stack: ?[]u8,
};

const ThreadLinkedList = std.DoublyLinkedList(Thread);
const ThreadQueue = std.meta.Tuple(&.{ u8, ThreadLinkedList });
const ThreadPriorityQueue = std.PriorityQueue(ThreadQueue, void, compareThreadPriorities);

var sched_vec: *idt.HandlerData = undefined;
var id_counter = std.atomic.Value(usize).init(0);
var lock = std.atomic.Value(bool).init(false);
var global_queue: ThreadPriorityQueue = undefined;

fn init() kernel.Task.Ret {
    sched_vec = idt.allocate_handler(null);
    sched_vec.handler = schedule;
    global_queue = ThreadPriorityQueue.init(mem.kheap.allocator(), {});

    const kproc = Thread{
        .ef = null,
        .mapper = mem.kmapper,
        .stack = null,
    };
    enqueue(100, kproc);
    kernel.drivers.timers.callback(50_000_000, null, schedule) catch return .failed;
    return .success;
}

fn enqueue(priority: u8, thread: Thread) void {
    const node = mem.kheap.allocator().create(ThreadLinkedList.Node) catch @panic("OOM");
    node.data = thread;
    node.data.id = id_counter.fetchAdd(1, .monotonic);
    while (lock.cmpxchgWeak(false, true, .acquire, .monotonic) != null) {}

    for (global_queue.items) |*queue| {
        if (queue[0] == priority) {
            queue[1].append(node);
            return;
        }
    }
    var queue = ThreadQueue{ priority, ThreadLinkedList{} };
    queue[1].append(node);
    global_queue.add(queue) catch @panic("OOM");

    lock.store(false, .release);
}

pub fn schedule(ctx: ?*anyopaque, status: *const cpu.Status) *const cpu.Status {
    log.info("schedule()", .{});
    _ = ctx;
    return status;
}

fn compareThreadPriorities(_: void, a: ThreadQueue, b: ThreadQueue) std.math.Order {
    // Highest priority first, lowest priority last
    return std.math.order(b[0], a[0]);
}
