#pragma once
#include <scheduler/thread.h>

extern thread_t *cur_thread;

void scheduler_add_thread(thread_t*);