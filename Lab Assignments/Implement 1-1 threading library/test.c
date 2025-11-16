#include "thread.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define NUM_THREADS 5

int global_counter = 0;

void *counter_thread(void *arg) {
    int id = *(int *)arg;
    printf("Thread %d starting\n", id);
    
    for (int i = 0; i < 3; i++) {
        printf("Thread %d: count %d\n", id, i);
        sleep(1);
    }
    
    printf("Thread %d exiting\n", id);
    return (void *)(long)(id * 100);
}

void *calculator_thread(void *arg) {
    int n = *(int *)arg;
    int *result = malloc(sizeof(int));
    *result = n * n;
    printf("Calculator: %d * %d = %d\n", n, n, *result);
    return result;
}

void *early_exit_thread(void *arg) {
    printf("Early exit thread starting\n");
    sleep(1);
    printf("Early exit thread calling thread_exit\n");
    thread_exit((void *)999L);
    return NULL;
}

void *sync_thread(void *arg) {
    int id = *(int *)arg;
    printf("Sync thread %d working\n", id);
    sleep(2);
    printf("Sync thread %d done\n", id);
    return (void *)(long)id;
}

void test_basic() {
    printf("\nBasic Creation\n");
    thread_t threads[3];
    int args[3] = {1, 2, 3};
    
    for (int i = 0; i < 3; i++) {
        thread_create(&threads[i], counter_thread, &args[i]);
    }
    
    for (int i = 0; i < 3; i++) {
        void *retval;
        thread_join(threads[i], &retval);
        printf("Thread %d returned %ld\n", i, (long)retval);
    }
}

void test_return() {
    printf("\nReturn Values\n");
    thread_t thread;
    int arg = 7;
    
    thread_create(&thread, calculator_thread, &arg);
    void *retval;
    thread_join(thread, &retval);
    int *result = (int *)retval;
    printf("Got result %d\n", *result);
    free(result);
}

void test_exit() {
    printf("\nthread_exit\n");
    thread_t thread;
    
    thread_create(&thread, early_exit_thread, NULL);
    void *retval;
    thread_join(thread, &retval);
    printf("Early exit returned %ld\n", (long)retval);
}

void test_multiple() {
    printf("\nMultiple Threads\n");
    thread_t threads[NUM_THREADS];
    int args[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i] = i;
        thread_create(&threads[i], sync_thread, &args[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        void *retval;
        thread_join(threads[i], &retval);
        printf("Joined thread %d, returned %ld\n", i, (long)retval);
    }
}

void test_errors() {
    printf("\nError Cases\n");
    
    if (thread_join(999, NULL) == -1) {
        printf("Correctly failed to join invalid thread\n");
    }
    
    thread_t thread;
    int arg = 1;
    thread_create(&thread, counter_thread, &arg);
    void *retval;
    thread_join(thread, &retval);
    if (thread_join(thread, NULL) == -1) {
        printf("Correctly failed to join already joined thread\n");
    }
}

int main() {
    printf("Starting Thread Tests\n");
    
    test_basic();
    test_return();
    test_exit();
    test_multiple();
    test_errors();
    
    printf("\nAll tests completed\n");
}
