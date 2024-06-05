#pragma once
#define SCHEDULER_THREAD_TICKS 0x100

struct cpu_status;
struct thread;
struct task;

struct cpu_status *schedule(struct cpu_status*);
void scheduler_add_thread(struct thread*);