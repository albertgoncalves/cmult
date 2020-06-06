#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "exit.h"
#include "tpool.h"

Bool tpool_work_enqueue(TPool* pool, void* arg) {
    if (pool == NULL) {
        return FALSE;
    }
    LOCK_OR_EXIT(pool->mutex);
    if ((CAPACITY - pool->queue_len) == 0) {
        UNLOCK_OR_EXIT(pool->mutex);
        return FALSE;
    }
    pool->memory[pool->index] = arg;
    pool->index = (u8)((pool->index + 1) % CAPACITY);
    ++pool->queue_len;
    EXIT_IF(pthread_cond_broadcast(&pool->enqueue_cond) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    return TRUE;
}

static void* tpool_work_dequeue(TPool* pool) {
    if ((pool == NULL) || (pool->queue_len == 0)) {
        return NULL;
    }
    u8 index = (u8)(((CAPACITY - pool->queue_len) + pool->index) % CAPACITY);
    void* arg = pool->memory[index];
    --pool->queue_len;
    return arg;
}

static void* tpool_worker(void* ptr) {
    TPool* pool = ptr;
    for (;;) {
        LOCK_OR_EXIT(pool->mutex);
        while ((pool->queue_len == 0) && (pool->stop == FALSE)) {
            /* NOTE: `pthread_cond_wait` does a few things for us:
             *   1. Block process until work arrives.
             *   2. Release `pool->mutex` so other workers can acquire it.
             *   3. Once work arrives, collect re-lock `pool->mutex`.
             */
            EXIT_IF(pthread_cond_wait(&pool->enqueue_cond, &pool->mutex) != 0);
        }
        if (pool->stop == TRUE) {
            break;
        }
        ++pool->active_threads;
        void* arg = tpool_work_dequeue(pool);
        UNLOCK_OR_EXIT(pool->mutex);
        if (arg != NULL) {
            pool->fn(arg);
        }
        LOCK_OR_EXIT(pool->mutex);
        --pool->active_threads;
        if ((pool->stop == FALSE) && (pool->active_threads == 0) &&
            (pool->queue_len == 0))
        {
            /* NOTE: Send signal that given thread is no longer working. */
            EXIT_IF(pthread_cond_signal(&pool->dequeue_cond) != 0);
        }
        UNLOCK_OR_EXIT(pool->mutex);
    }
    --pool->total_threads;
    /* NOTE: Send signal that the given thread has exited its work loop. */
    EXIT_IF(pthread_cond_signal(&pool->dequeue_cond) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    return NULL;
}

Bool tpool_set(TPool* pool, const ThreadFn fn, const u8 total_threads) {
    if ((fn == NULL) || (total_threads == 0)) {
        return FALSE;
    }
    EXIT_IF(pthread_mutex_init(&pool->mutex, NULL) != 0);
    EXIT_IF(pthread_cond_init(&pool->enqueue_cond, NULL) != 0);
    EXIT_IF(pthread_cond_init(&pool->dequeue_cond, NULL) != 0);
    pool->fn = fn;
    pool->stop = FALSE;
    pool->active_threads = 0;
    pool->total_threads = total_threads;
    pool->queue_len = 0;
    pool->index = 0;
    for (u8 i = 0; i < total_threads; ++i) {
        pthread_t thread;
        EXIT_IF(pthread_create(&thread, NULL, tpool_worker, pool) != 0);
        EXIT_IF(pthread_detach(thread) != 0);
    }
    return TRUE;
}

void tpool_wait(TPool* pool) {
    if (pool == NULL) {
        return;
    }
    LOCK_OR_EXIT(pool->mutex);
    while (((!pool->stop) &&
            ((pool->queue_len != 0) || (pool->n_thread_active != 0))) ||
           ((pool->stop) && (pool->n_thread_total != 0)))
    {
        EXIT_IF(pthread_cond_wait(&pool->dequeue_cond, &pool->mutex) != 0);
    }
    UNLOCK_OR_EXIT(pool->mutex);
}

void tpool_clear(TPool* pool) {
    if (pool == NULL) {
        return;
    }
    LOCK_OR_EXIT(pool->mutex);
    pool->stop = TRUE;
    EXIT_IF(pthread_cond_broadcast(&pool->enqueue_cond) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    tpool_wait(pool);
    EXIT_IF(pthread_mutex_destroy(&pool->mutex) != 0);
    EXIT_IF(pthread_cond_destroy(&pool->enqueue_cond) != 0);
    EXIT_IF(pthread_cond_destroy(&pool->dequeue_cond) != 0);
}
