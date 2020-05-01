#include <pthread.h>

#include "exit.h"
#include "tpool.h"

#define LOCK_OR_EXIT(mutex)   EXIT_IF(pthread_mutex_lock(&mutex) != 0);
#define UNLOCK_OR_EXIT(mutex) EXIT_IF(pthread_mutex_unlock(&mutex) != 0);

bool tpool_work_enqueue(tpool_t* pool, void* arg) {
    if (pool == NULL) {
        return false;
    }
    LOCK_OR_EXIT(pool->mutex);
    if ((CAPACITY - pool->queue_len) == 0) {
        UNLOCK_OR_EXIT(pool->mutex);
        return false;
    }
    pool->memory[pool->index] = arg;
    pool->index = (pool->index + 1) % CAPACITY;
    ++pool->queue_len;
    EXIT_IF(pthread_cond_broadcast(&pool->enqueue_cond) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    return true;
}

static void* tpool_work_dequeue(tpool_t* pool) {
    if ((pool == NULL) || (pool->queue_len == 0)) {
        return NULL;
    }
    size_t index = ((CAPACITY - pool->queue_len) + pool->index) % CAPACITY;
    void*  arg = pool->memory[index];
    --pool->queue_len;
    return arg;
}

static void* tpool_worker(void* ptr) {
    tpool_t* pool = ptr;
    for (;;) {
        LOCK_OR_EXIT(pool->mutex);
        while ((pool->queue_len == 0) && (!pool->stop)) {
            /* NOTE: `pthread_cond_wait` does a few things for us:
             *   1. Block process until work arrives.
             *   2. Release `pool->mutex` so other workers can acquire it.
             *   3. Once work arrives, collect re-lock `pool->mutex`.
             */
            EXIT_IF(pthread_cond_wait(&pool->enqueue_cond, &pool->mutex) != 0);
        }
        if (pool->stop) {
            break;
        }
        ++pool->n_thread_active;
        void* arg = tpool_work_dequeue(pool);
        UNLOCK_OR_EXIT(pool->mutex);
        if (arg != NULL) {
            pool->func(arg);
        }
        LOCK_OR_EXIT(pool->mutex);
        --pool->n_thread_active;
        if ((!pool->stop) && (pool->n_thread_active == 0) &&
            (pool->queue_len == 0)) {
            /* NOTE: Send signal that given thread is no longer working. */
            EXIT_IF(pthread_cond_signal(&pool->dequeue_cond) != 0);
        }
        UNLOCK_OR_EXIT(pool->mutex);
    }
    --pool->n_thread_total;
    /* NOTE: Send signal that the given thread has exited its work loop. */
    EXIT_IF(pthread_cond_signal(&pool->dequeue_cond) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    return NULL;
}

bool tpool_set(tpool_t* pool, const thread_func_t func, const size_t n) {
    if ((func == NULL) || (n == 0)) {
        return false;
    }
    EXIT_IF(pthread_mutex_init(&pool->mutex, NULL) != 0);
    EXIT_IF(pthread_cond_init(&pool->enqueue_cond, NULL) != 0);
    EXIT_IF(pthread_cond_init(&pool->dequeue_cond, NULL) != 0);
    pool->func = func;
    pool->stop = false;
    pool->n_thread_active = 0;
    pool->n_thread_total = n;
    pool->queue_len = 0;
    pool->index = 0;
    for (size_t i = 0; i < n; ++i) {
        pthread_t thread;
        EXIT_IF(pthread_create(&thread, NULL, tpool_worker, pool) != 0);
        EXIT_IF(pthread_detach(thread) != 0);
    }
    return true;
}

void tpool_wait(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    LOCK_OR_EXIT(pool->mutex);
    while (((!pool->stop) && (pool->queue_len != 0)) ||
           ((!pool->stop) && (pool->n_thread_active != 0)) ||
           ((pool->stop) && (pool->n_thread_total != 0)))
    {
        EXIT_IF(pthread_cond_wait(&pool->dequeue_cond, &pool->mutex) != 0);
    }
    UNLOCK_OR_EXIT(pool->mutex);
}

void tpool_clear(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    LOCK_OR_EXIT(pool->mutex);
    pool->stop = true;
    EXIT_IF(pthread_cond_broadcast(&pool->enqueue_cond) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    tpool_wait(pool);
    EXIT_IF(pthread_mutex_destroy(&pool->mutex) != 0);
    EXIT_IF(pthread_cond_destroy(&pool->enqueue_cond) != 0);
    EXIT_IF(pthread_cond_destroy(&pool->dequeue_cond) != 0);
}
