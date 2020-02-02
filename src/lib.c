#include <pthread.h>
#include <stdlib.h>

#include "lib.h"

#define LOCK_OR_EXIT(mutex)   EXIT_IF(pthread_mutex_lock(&mutex) != 0);
#define UNLOCK_OR_EXIT(mutex) EXIT_IF(pthread_mutex_unlock(&mutex) != 0);

static const size_t DEFAULT_N_THREADS = 2;

struct tpool_work {
    void*              arg;
    struct tpool_work* next;
};

static tpool_work_t* tpool_work_create(void* arg) {
    tpool_work_t* work = malloc(sizeof(tpool_work_t));
    EXIT_IF(work == NULL);
    work->arg  = arg;
    work->next = NULL;
    return work;
}

static void tpool_work_destroy(tpool_work_t* work) {
    if (work == NULL) {
        return;
    }
    free(work);
}

static tpool_work_t* tpool_work_dequeue(tpool_t* pool) {
    if (pool == NULL) {
        return NULL;
    }
    tpool_work_t* work = pool->queue_first;
    if (work == NULL) {
        return NULL;
    }
    if (work->next == NULL) {
        pool->queue_first = NULL;
        pool->queue_last  = NULL;
    } else {
        pool->queue_first = work->next;
    }
    return work;
}

static void* tpool_worker(void* arg) {
    tpool_t* pool = arg;
    for (;;) {
        LOCK_OR_EXIT(pool->mutex);
        while ((pool->queue_first == NULL) && (!pool->stop)) {
            /* NOTE: `pthread_cond_wait` does a few things for us:
             *   1. Block process until work arrives.
             *   2. Release `pool->mutex` so other workers can acquire it.
             *   3. Once work arrives, collect re-lock `pool->mutex`.
             */
            EXIT_IF(pthread_cond_wait(&(pool->enqueue_cond), &(pool->mutex)) !=
                    0);
        }
        if (pool->stop) {
            break;
        }
        ++pool->n_thread_active;
        tpool_work_t* work = tpool_work_dequeue(pool);
        if (work != NULL) {
            --pool->queue_len;
        }
        UNLOCK_OR_EXIT(pool->mutex);
        if (work != NULL) {
            pool->func(work->arg);
            tpool_work_destroy(work);
        }
        LOCK_OR_EXIT(pool->mutex);
        --pool->n_thread_active;
        if ((!pool->stop) && (pool->n_thread_active == 0) &&
            (pool->queue_first == NULL)) {
            /* NOTE: Send signal that given thread is no longer working. */
            EXIT_IF(pthread_cond_signal(&(pool->dequeue_cond)) != 0);
        }
        UNLOCK_OR_EXIT(pool->mutex);
    }
    --pool->n_thread_total;
    /* NOTE: Send signal that the given thread has exited its work loop. */
    EXIT_IF(pthread_cond_signal(&(pool->dequeue_cond)) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    return NULL;
}

bool tpool_set(tpool_t* pool, thread_func_t func, size_t n) {
    if (func == NULL) {
        return false;
    }
    if (n == 0) {
        n = DEFAULT_N_THREADS;
    }
    EXIT_IF(pthread_mutex_init(&(pool->mutex), NULL) != 0);
    EXIT_IF(pthread_cond_init(&(pool->enqueue_cond), NULL) != 0);
    EXIT_IF(pthread_cond_init(&(pool->dequeue_cond), NULL) != 0);
    pool->func            = func;
    pool->queue_first     = NULL;
    pool->queue_last      = NULL;
    pool->queue_len       = 0;
    pool->n_thread_active = 0;
    pool->n_thread_total  = n;
    pool->stop            = false;
    for (size_t i = 0; i < n; ++i) {
        pthread_t thread;
        EXIT_IF(pthread_create(&thread, NULL, tpool_worker, pool) != 0);
        EXIT_IF(pthread_detach(thread) != 0);
    }
    return true;
}

void tpool_clear(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    LOCK_OR_EXIT(pool->mutex);
    tpool_work_t* work_current = pool->queue_first;
    while (work_current != NULL) {
        tpool_work_t* work_next = work_current->next;
        tpool_work_destroy(work_current);
        work_current = work_next;
    }
    pool->stop = true;
    EXIT_IF(pthread_cond_broadcast(&(pool->enqueue_cond)) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    tpool_wait(pool);
    EXIT_IF(pthread_mutex_destroy(&(pool->mutex)) != 0);
    EXIT_IF(pthread_cond_destroy(&(pool->enqueue_cond)) != 0);
    EXIT_IF(pthread_cond_destroy(&(pool->dequeue_cond)) != 0);
}

bool tpool_work_enqueue(tpool_t* pool, void* arg) {
    if (pool == NULL) {
        return false;
    }
    tpool_work_t* work = tpool_work_create(arg);
    if (work == NULL) {
        return false;
    }
    LOCK_OR_EXIT(pool->mutex);
    ++pool->queue_len;
    if (pool->queue_first == NULL) {
        pool->queue_first = work;
        pool->queue_last  = pool->queue_first;
    } else {
        pool->queue_last->next = work;
        pool->queue_last       = pool->queue_last->next;
    }
    EXIT_IF(pthread_cond_broadcast(&(pool->enqueue_cond)) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    return true;
}

void tpool_wait(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    LOCK_OR_EXIT(pool->mutex);
    while (((!pool->stop) && (pool->queue_len != 0)) ||
           ((!pool->stop) && (pool->n_thread_active != 0)) ||
           ((pool->stop) && (pool->n_thread_total != 0))) {
        EXIT_IF(pthread_cond_wait(&(pool->dequeue_cond), &(pool->mutex)) != 0);
    }
    UNLOCK_OR_EXIT(pool->mutex);
}
