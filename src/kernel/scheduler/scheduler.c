#include <stddef.h>
#include <scheduler/thread.h>
#include <scheduler/scheduler.h>

size_t scheduler_ticks = 0;
size_t next_thread_id;
thread_t *thread_list = NULL;
thread_t *cur_thread = NULL;
thread_t *idle_thread = NULL;

size_t thread_list_size = 0;

void init_scheduler(void) {
}
