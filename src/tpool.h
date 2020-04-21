#ifndef __TPOOL_H__
#define __TPOOL_H__

#include <stdbool.h>

#define CAPACITY 33

typedef void (*thread_func_t)(void* arg);

typedef struct {
    pthread_cond_t  enqueue_cond;
    pthread_cond_t  dequeue_cond;
    pthread_mutex_t mutex;
    thread_func_t   func;
    size_t          n_thread_active;
    size_t          n_thread_total;
    size_t          queue_len;
    size_t          index;
    bool            stop;
    char            _[7];
    void*           memory[CAPACITY];
} tpool_t;

bool tpool_set(tpool_t* pool, const thread_func_t func, const size_t n);
void tpool_clear(tpool_t* pool);
bool tpool_work_enqueue(tpool_t* pool, void* arg);
void tpool_wait(tpool_t* pool);

#endif
