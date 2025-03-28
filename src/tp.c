#include "tp.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void *thread_pool_thread(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;
    tasks_t task;

    while (true) {
        pthread_mutex_lock(&pool->queue_mutex);

        while (pool->queue_count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_not_empty, &pool->queue_mutex);
        }

        if (pool->shutdown && pool->queue_count == 0) {
            pthread_mutex_unlock(&pool->queue_mutex);
            pthread_exit(NULL);
        }

        task.fn = pool->queue[pool->queue_front].fn;
        task.arg = pool->queue[pool->queue_front].arg;

        pool->queue_front = (pool->queue_front + 1) % pool->queue_size;
        pool->queue_count--;

        pthread_cond_signal(&pool->queue_not_full);
        pthread_mutex_unlock(&pool->queue_mutex);

        (*(task.fn))(task.arg);
    }
}

thread_pool_t *thread_pool_create(int max_thread, int max_queue) {
    thread_pool_t *pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    if (pool == NULL) {
        printf("Failed to allocate memory for thread pool\n");
        return NULL;
    }

    pool->thread_count = max_thread;
    pool->queue_size = max_queue;
    pool->queue_front = 0;
    pool->queue_rear = -1;
    pool->queue_count = 0;
    pool->shutdown = 0;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * max_thread);
    pool->queue = (tasks_t *)malloc(sizeof(tasks_t) * max_queue);

    if (pool->threads == NULL || pool->queue == NULL) {
        printf("Failed to allocate memory for threads or queue\n");
        if (pool->threads) free(pool->threads);
        if (pool->queue) free(pool->queue);
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0 || pthread_cond_init(&pool->queue_not_empty, NULL) != 0 ||
        pthread_cond_init(&pool->queue_not_full, NULL) != 0) {
        printf("Failed to initialize mutex or condition variables\n");
        if (pool->threads) free(pool->threads);
        if (pool->queue) free(pool->queue);
        free(pool);
        return NULL;
    }

    for (int i = 0; i < max_thread; i++) {
        if (pthread_create(&pool->threads[i], NULL, thread_pool_thread, (void *)pool) != 0) {
            printf("Failed to create thread\n");
            pool->thread_count = i;
            // Destroy already created threads, mutex, and condition variables, and free allocated memory
            thread_pool_destroy(pool);
            return NULL;
        }
    }

    return pool;
}

int thread_pool_add_task(thread_pool_t *pool, void (*fn)(void *), void *arg) {
    if (pool == NULL || fn == NULL) {
        fprintf(stderr, "Thread pool or function is NULL\n");
        return -1;
    }

    pthread_mutex_lock(&pool->queue_mutex);

    // Wait if queue is full, and not shutting down
    while (pool->queue_count == pool->queue_size && !pool->shutdown) {
        pthread_cond_wait(&pool->queue_not_full, &pool->queue_mutex);
    }

    // If shutting down and queue is full, don't add task
    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->queue_mutex);
        return -1;
    }

    // Add task to queue
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_size;
    pool->queue[pool->queue_rear].fn = fn;
    pool->queue[pool->queue_rear].arg = arg;
    pool->queue_count++;

    pthread_cond_signal(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->queue_mutex);

    return 0;
}

void thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL) return;

    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = 1;

    // Wake up all threads (broadcast) so they can check the shutdown flag
    pthread_cond_broadcast(&pool->queue_not_empty);
    pthread_cond_broadcast(&pool->queue_not_full);
    pthread_mutex_unlock(&pool->queue_mutex);

    // Wait for all threads to finish
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);
    free(pool->queue);
    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_not_empty);
    pthread_cond_destroy(&pool->queue_not_full);
    free(pool);
}

unsigned int thread_count() {
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    if (count == -1) {
        return 1;
    }
    return (unsigned int)count;
}
