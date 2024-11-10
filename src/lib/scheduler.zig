const Process = @import("kernel").lib.Process;

// TODO: Separate into kernel and user quantums (50ms, 25 ms)
const QUANTUM = 50;

fn idle_thread() void {
    while (true) {}
}
