#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "lib.h"

#define ASSERT_LOCK(mutex) assert(pthread_mutex_lock(&mutex) == 0);
#define ASSERT_UNLOCK(mutex) assert(pthread_mutex_unlock(&mutex) == 0);

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
    assert(work != NULL);
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

static tpool_work_t* tpool_work_pop(tpool_t* pool) {
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
        ASSERT_LOCK(pool->work_mutex);
        while ((pool->work_first == NULL) && (!pool->stop)) {
            /* NOTE: `pthread_cond_wait` does a few things for us:
             *   1. Block process until work arrives.
             *   2. Release `pool->work_mutex` so other workers can acquire it.
             *   3. Once work arrives, collect re-lock `pool->work_mutex`.
             */
            assert(pthread_cond_wait(&(pool->work_cond),
                                     &(pool->work_mutex)) == 0);
        }
        if (pool->stop) {
            /* NOTE: This provides top-level control; if this is `true` all
             * threads will `break` before beginning any new work.
             */
            break;
        }
        tpool_work_t* work = tpool_work_pop(pool);
        pool->working_cnt++;
        ASSERT_UNLOCK(pool->work_mutex);
        if (work != NULL) {
            work->func(work->arg);
            tpool_work_destroy(work);
        }
        ASSERT_LOCK(pool->work_mutex);
        pool->working_cnt--;
        if ((!pool->stop) && (pool->working_cnt == 0) &&
            (pool->work_first == NULL)) {
            assert(pthread_cond_signal(&(pool->working_cond)) == 0);
        }
        ASSERT_UNLOCK(pool->work_mutex);
    }
    pool->thread_cnt--;
    assert(pthread_cond_signal(&(pool->working_cond)) == 0);
    ASSERT_UNLOCK(pool->work_mutex);
    return NULL;
}

void tpool_set(tpool_t* pool, size_t n) {
    if (n == 0) {
        n = DEFAULT_N_THREADS;
    }
    pool->work_first = NULL;
    pool->work_last  = NULL;
    assert(pthread_mutex_init(&(pool->work_mutex), NULL) == 0);
    assert(pthread_cond_init(&(pool->work_cond), NULL) == 0);
    pool->working_cnt = 0;
    pool->thread_cnt  = n;
    assert(pthread_cond_init(&(pool->working_cond), NULL) == 0);
    pool->stop = false;
    for (size_t i = 0; i < n; ++i) {
        pthread_t thread;
        assert(pthread_create(&thread, NULL, tpool_worker, pool) == 0);
        assert(pthread_detach(thread) == 0);
    }
}

void tpool_clear(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    ASSERT_LOCK(pool->work_mutex);
    tpool_work_t* work_current = pool->work_first;
    while (work_current != NULL) {
        tpool_work_t* work_next = work_current->next;
        tpool_work_destroy(work_current);
        work_current = work_next;
    }
    pool->stop = true;
    assert(pthread_cond_broadcast(&(pool->work_cond)) == 0);
    ASSERT_UNLOCK(pool->work_mutex);
    tpool_wait(pool);
    assert(pthread_mutex_destroy(&(pool->work_mutex)) == 0);
    assert(pthread_cond_destroy(&(pool->work_cond)) == 0);
    assert(pthread_cond_destroy(&(pool->working_cond)) == 0);
}

bool tpool_work_push(tpool_t* pool, thread_func_t func, void* arg) {
    if (pool == NULL) {
        return false;
    }
    tpool_work_t* work = tpool_work_create(func, arg);
    if (work == NULL) {
        return false;
    }
    ASSERT_LOCK(pool->work_mutex);
    if (pool->work_first == NULL) {
        pool->work_first = work;
        pool->work_last  = pool->work_first;
    } else {
        pool->work_last->next = work;
        pool->work_last       = pool->work_last->next;
    }
    assert(pthread_cond_broadcast(&(pool->work_cond)) == 0);
    ASSERT_UNLOCK(pool->work_mutex);
    return true;
}

void tpool_wait(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    ASSERT_LOCK(pool->work_mutex);
    for (;;) {
        if (((!pool->stop) && (pool->working_cnt != 0)) ||
            ((pool->stop) && (pool->thread_cnt != 0))) {
            assert(pthread_cond_wait(&(pool->working_cond),
                                     &(pool->work_mutex)) == 0);
        } else {
            break;
        }
    }
    ASSERT_UNLOCK(pool->work_mutex);
}
