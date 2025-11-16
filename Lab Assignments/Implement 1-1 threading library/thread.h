#ifndef THREAD_H
#define THREAD_H

#include <sys/types.h>

#define THREAD_STACK_SIZE (1024 * 1024)

typedef int thread_t;

typedef enum {
    THREAD_RUNNING = 0,
    THREAD_TERMINATED = 1
} thread_state_t;

typedef struct {
    thread_t tid;
    thread_state_t state;
    void *retval;
    pid_t kernel_tid;
    void *stack;
} tcb_t;

int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg);
int thread_join(thread_t thread, void **retval);
void thread_exit(void *retval);

#endif

