#pragma once
#define SCHEDULER_THREAD_TICKS 0x200

struct cpu_status;
struct thread;
struct task;

extern struct thread *cur_thread;
extern struct thread *idle_thread;

struct cpu_status *schedule(struct cpu_status*);
void scheduler_add_thread(struct thread*);
void scheduler_add_task(struct task*);