#define _GNU_SOURCE
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <errno.h>
#include <sched.h>

#define MAX_THREADS 1024

static tcb_t *thread_table[MAX_THREADS];
static int next_tid = 0;

typedef struct {
    void *(*start_routine)(void *);
    void *arg;
    thread_t tid;
} thread_args_t;

static int thread_start(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    void *(*start_routine)(void *) = args->start_routine;
    void *routine_arg = args->arg;
    thread_t tid = args->tid;
    
    tcb_t *tcb = thread_table[tid];
    tcb->kernel_tid = syscall(SYS_gettid);
    
    free(args);
    
    void *retval = start_routine(routine_arg);
    thread_exit(retval);
    return 0;
}

int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg) {
    if (next_tid >= MAX_THREADS) {
        return -1;
    }
    
    tcb_t *tcb = malloc(sizeof(tcb_t));
    void *stack = mmap(NULL, THREAD_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0); 
    void *stack_top = (char *)stack + THREAD_STACK_SIZE;
    thread_args_t *args = malloc(sizeof(thread_args_t));
    
    tcb->tid = next_tid;
    tcb->state = THREAD_RUNNING;
    tcb->retval = NULL;
    tcb->kernel_tid = 0;
    tcb->stack = stack;
    
    args->start_routine = start_routine;
    args->arg = arg;
    args->tid = next_tid;
    
    thread_table[next_tid] = tcb;
    
    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM;
    
    pid_t pid = clone(thread_start, stack_top, flags, args);
    
    if (pid == -1) {
        thread_table[next_tid] = NULL;
        free(args);
        munmap(stack, THREAD_STACK_SIZE);
        free(tcb);
        return -1;
    }
    
    *thread = next_tid;
    next_tid++;
    
    return 0;
}

int thread_join(thread_t thread, void **retval) {
    if (thread >= next_tid || thread < 0 || !thread_table[thread]) {
        return -1;
    }
    
    tcb_t *tcb = thread_table[thread];
    
    while (tcb->state != THREAD_TERMINATED) {
        usleep(1000);
    }
    
    if (retval) {
        *retval = tcb->retval;
    }
    
    munmap(tcb->stack, THREAD_STACK_SIZE);
    free(tcb);
    thread_table[thread] = NULL;
    
    return 0;
}

void thread_exit(void *retval) {
    pid_t current_tid = syscall(SYS_gettid);
    
    for (int i = 0; i < next_tid; i++) {
        if (thread_table[i] && thread_table[i]->kernel_tid == current_tid) {
            thread_table[i]->retval = retval;
            thread_table[i]->state = THREAD_TERMINATED;
            break;
        }
    }
    
    syscall(SYS_exit, 0);
}


