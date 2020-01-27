#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "lib.h"

struct tpool_work {
    thread_func_t      func;
    void*              arg;
    struct tpool_work* next;
};

struct tpool {
    tpool_work_t*   work_first;
    tpool_work_t*   work_last;
    pthread_mutex_t work_mutex;
    pthread_cond_t  work_cond;
    pthread_cond_t  working_cond;
    size_t          working_cnt;
    size_t          thread_cnt;
    bool            stop;
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

static tpool_work_t* tpool_work_get(tpool_t* pool) {
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
    tpool_work_t* work;
    tpool_t*      pool = arg;
    for (;;) {
        pthread_mutex_lock(&(pool->work_mutex));
        while ((pool->work_first == NULL) && (!pool->stop)) {
            /* NOTE: `pthread_cond_wait` does a few things for us:
             *   1. Block process until work arrives.
             *   2. Release `pool->work_mutex` so other workers can acquire it.
             *   3. Once work arrives, collect re-lock `pool->work_mutex`.
             */
            pthread_cond_wait(&(pool->work_cond), &(pool->work_mutex));
        }
        if (pool->stop) {
            /* NOTE: This provides top-level control; if this is `true` all
             * threads will `break` before beginning any new work.
             */
            break;
        }
        work = tpool_work_get(pool);
        pool->working_cnt++;
        pthread_mutex_unlock(&(pool->work_mutex));
        if (work != NULL) {
            work->func(work->arg);
            tpool_work_destroy(work);
        }
        pthread_mutex_lock(&(pool->work_mutex));
        pool->working_cnt--;
        if ((!pool->stop) && (pool->working_cnt == 0) &&
            (pool->work_first == NULL)) {
            pthread_cond_signal(&(pool->working_cond));
        }
        pthread_mutex_unlock(&(pool->work_mutex));
    }
    pool->thread_cnt--;
    pthread_cond_signal(&(pool->working_cond));
    pthread_mutex_unlock(&(pool->work_mutex));
    return NULL;
}

tpool_t* tpool_create(size_t n) {
    if (n == 0) {
        n = 2;
    }
    tpool_t* pool;
    pool             = calloc(1, sizeof(*pool));
    pool->thread_cnt = n;
    assert(pthread_mutex_init(&(pool->work_mutex), NULL) == 0);
    assert(pthread_cond_init(&(pool->work_cond), NULL) == 0);
    assert(pthread_cond_init(&(pool->working_cond), NULL) == 0);
    pool->work_first = NULL;
    pool->work_last  = NULL;
    pthread_t thread;
    for (size_t i = 0; i < n; ++i) {
        pthread_create(&thread, NULL, tpool_worker, pool);
        pthread_detach(thread);
    }
    return pool;
}

void tpool_destroy(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    tpool_work_t* work_current;
    tpool_work_t* work_next;
    pthread_mutex_lock(&(pool->work_mutex));
    work_current = pool->work_first;
    while (work_current != NULL) {
        work_next = work_current->next;
        tpool_work_destroy(work_current);
        work_current = work_next;
    }
    pool->stop = true;
    pthread_cond_broadcast(&(pool->work_cond));
    pthread_mutex_unlock(&(pool->work_mutex));
    tpool_wait(pool);
    pthread_mutex_destroy(&(pool->work_mutex));
    pthread_cond_destroy(&(pool->work_cond));
    pthread_cond_destroy(&(pool->working_cond));
    free(pool);
}

bool tpool_add_work(tpool_t* pool, thread_func_t func, void* arg) {
    if (pool == NULL) {
        return false;
    }
    tpool_work_t* work = tpool_work_create(func, arg);
    if (work == NULL) {
        return false;
    }
    pthread_mutex_lock(&(pool->work_mutex));
    if (pool->work_first == NULL) {
        pool->work_first = work;
        pool->work_last  = pool->work_first;
    } else {
        pool->work_last->next = work;
        pool->work_last       = work;
    }
    pthread_cond_broadcast(&(pool->work_cond));
    pthread_mutex_unlock(&(pool->work_mutex));
    return true;
}

void tpool_wait(tpool_t* pool) {
    if (pool == NULL) {
        return;
    }
    pthread_mutex_lock(&(pool->work_mutex));
    for (;;) {
        if (((!pool->stop) && (pool->working_cnt != 0)) ||
            ((pool->stop) && (pool->thread_cnt != 0))) {
            pthread_cond_wait(&(pool->working_cond), &(pool->work_mutex));
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&(pool->work_mutex));
}
