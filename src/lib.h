#ifndef __LIB_H__
#define __LIB_H__

#include <stdbool.h>
#include <stdio.h>

#define BOLD_RED "\033[1;31m"
#define BOLD "\033[1m"
#define CLOSE "\033[0m"

#define EXIT_IF(condition)                                                   \
    if (condition) {                                                         \
        fprintf(stderr, "%sError%s: <%s%s%s>.%s%s%s(), ln.%s%d%s", BOLD_RED, \
                CLOSE, BOLD, __FILE__, CLOSE, BOLD, __func__, CLOSE, BOLD,   \
                __LINE__, CLOSE);                                            \
        exit(1);                                                             \
    }

typedef struct tpool_work tpool_work_t;
typedef struct tpool      tpool_t;

struct tpool {
    tpool_work_t*   work_first;
    tpool_work_t*   work_last;
    pthread_mutex_t mutex;
    pthread_cond_t  new_work_cond;
    pthread_cond_t  working_cond;
    size_t          working_cnt;
    size_t          thread_cnt;
    bool            stop;
};

typedef void (*thread_func_t)(void* arg);

void tpool_set(tpool_t* pool, size_t n);
void tpool_clear(tpool_t* pool);
bool tpool_work_enqueue(tpool_t* pool, thread_func_t func, void* arg);
void tpool_wait(tpool_t* pool);

#endif
