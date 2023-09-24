#include <stdint.h>
#include <stddef.h>
#include <cpu/scheduler.h>
#include <kernel/logging.h>

cpu_status_t *tasks[10];
size_t num_tasks;
size_t active_task = 0;

void print_task(size_t id) {
    cpu_status_t *task = tasks[id];
    log(Verbose, true, "SCHEDULER", "%u", task->ds);
}
