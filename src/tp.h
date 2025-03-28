#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

// Define maximum capacity of task in queue in thread pool.
// This queue temporarily stores tasks before they are processed by worker threads.
//
// If the queue is full, thread_pool_add_task() in thread pool will block (wait) until space becomes available.
#define MAX_QUEUE 256

typedef struct {
    void (*fn)(void *);
    void *arg;
} tasks_t;

typedef struct {
    pthread_t *threads;
    tasks_t *queue;
    int queue_size;
    int queue_front;
    int queue_rear;
    int queue_count;
    int thread_count;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_not_full;
    pthread_cond_t queue_not_empty;
    int shutdown;
} thread_pool_t;

thread_pool_t *thread_pool_create(int max_thread, int max_queue);
int thread_pool_add_task(thread_pool_t *pool, void (*fn)(void *), void *arg);
void thread_pool_destroy(thread_pool_t *pool);
unsigned int thread_count();

#endif  // THREAD_POOL_H
