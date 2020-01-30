#include <pthread.h>
#include <stdlib.h>

#include "lib.h"

#define LOCK_OR_EXIT(mutex)   EXIT_IF(pthread_mutex_lock(&mutex) != 0);
#define UNLOCK_OR_EXIT(mutex) EXIT_IF(pthread_mutex_unlock(&mutex) != 0);

static const size_t DEFAULT_N_THREADS = 2;

struct tpool_work {
    thread_func_t      func;
    void*              arg;
    struct tpool_work* next;
};

static tpool_work_t* tpool_work_create(thread_func_t func, void* arg) {
    if (func == NULL) {
        return NULL;
    }
    tpool_work_t* work = malloc(sizeof(tpool_work_t));
    EXIT_IF(work == NULL);
    work->func = func;
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
    tpool_work_t* work = pool->work_first;
    if (work == NULL) {
        return NULL;
    }
    if (work->next == NULL) {
        pool->work_first = NULL;
        pool->work_last  = NULL;
    } else {
        pool->work_first = work->next;
    }
    return work;
}

static void* tpool_worker(void* arg) {
    tpool_t* pool = arg;
    for (;;) {
        LOCK_OR_EXIT(pool->mutex);
        while ((pool->work_first == NULL) && (!pool->stop)) {
            /* NOTE: `pthread_cond_wait` does a few things for us:
             *   1. Block process until work arrives.
             *   2. Release `pool->mutex` so other workers can acquire it.
             *   3. Once work arrives, collect re-lock `pool->mutex`.
             */
            EXIT_IF(pthread_cond_wait(&(pool->new_work_cond),
                                      &(pool->mutex)) != 0);
        }
        if (pool->stop) {
            break;
        }
        tpool_work_t* work = tpool_work_dequeue(pool);
        pool->working_cnt++;
        UNLOCK_OR_EXIT(pool->mutex);
        if (work != NULL) {
            work->func(work->arg);
            tpool_work_destroy(work);
        }
        LOCK_OR_EXIT(pool->mutex);
        pool->working_cnt--;
        if ((!pool->stop) && (pool->working_cnt == 0) &&
            (pool->work_first == NULL)) {
            /* NOTE: Send signal that given thread is no longer working. */
            EXIT_IF(pthread_cond_signal(&(pool->working_cond)) != 0);
        }
        UNLOCK_OR_EXIT(pool->mutex);
    }
    pool->thread_cnt--;
    /* NOTE: Send signal that the given thread has exited its work loop. */
    EXIT_IF(pthread_cond_signal(&(pool->working_cond)) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    return NULL;
}

void tpool_set(tpool_t* pool, size_t n) {
    if (n == 0) {
        n = DEFAULT_N_THREADS;
    }
    pool->work_first = NULL;
    pool->work_last  = NULL;
    EXIT_IF(pthread_mutex_init(&(pool->mutex), NULL) != 0);
    EXIT_IF(pthread_cond_init(&(pool->new_work_cond), NULL) != 0);
    EXIT_IF(pthread_cond_init(&(pool->working_cond), NULL) != 0);
    pool->working_cnt = 0;
    pool->thread_cnt  = n;
    pool->stop        = false;
    for (size_t i = 0; i < n; ++i) {
        pthread_t thread;
        EXIT_IF(pthread_create(&thread, NULL, tpool_worker, pool) != 0);
        EXIT_IF(pthread_detach(thread) != 0);
    }
}

void tpool_clear(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    LOCK_OR_EXIT(pool->mutex);
    tpool_work_t* work_current = pool->work_first;
    while (work_current != NULL) {
        tpool_work_t* work_next = work_current->next;
        tpool_work_destroy(work_current);
        work_current = work_next;
    }
    pool->stop = true;
    EXIT_IF(pthread_cond_broadcast(&(pool->new_work_cond)) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    tpool_wait(pool);
    EXIT_IF(pthread_mutex_destroy(&(pool->mutex)) != 0);
    EXIT_IF(pthread_cond_destroy(&(pool->new_work_cond)) != 0);
    EXIT_IF(pthread_cond_destroy(&(pool->working_cond)) != 0);
}

bool tpool_work_enqueue(tpool_t* pool, thread_func_t func, void* arg) {
    if (pool == NULL) {
        return false;
    }
    tpool_work_t* work = tpool_work_create(func, arg);
    if (work == NULL) {
        return false;
    }
    LOCK_OR_EXIT(pool->mutex);
    if (pool->work_first == NULL) {
        pool->work_first = work;
        pool->work_last  = pool->work_first;
    } else {
        pool->work_last->next = work;
        pool->work_last       = pool->work_last->next;
    }
    EXIT_IF(pthread_cond_broadcast(&(pool->new_work_cond)) != 0);
    UNLOCK_OR_EXIT(pool->mutex);
    return true;
}

void tpool_wait(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    LOCK_OR_EXIT(pool->mutex);
    while (((!pool->stop) && (pool->working_cnt != 0)) ||
           ((pool->stop) && (pool->thread_cnt != 0))) {
        /* NOTE: Either at least one thread is still working, or at least one
         * thread has yet to return from its work loop.
         */
        EXIT_IF(pthread_cond_wait(&(pool->working_cond), &(pool->mutex)) != 0);
    }
    UNLOCK_OR_EXIT(pool->mutex);
}
