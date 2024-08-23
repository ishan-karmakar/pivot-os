#pragma once
#define SCHEDULER_THREAD_TICKS 0x100

struct status;
struct thread;
struct task;

struct status *schedule(struct status*);
void scheduler_add_thread(struct thread*);