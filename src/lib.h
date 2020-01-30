#ifndef __LIB_H__
#define __LIB_H__

#include <stdbool.h>
#include <stdio.h>

#define BOLD_PURPLE    "\033[1;35m"
#define BOLD           "\033[1m"
#define BOLD_UNDERLINE "\033[1;4m"
#define CLOSE          "\033[0m"

#define EXIT_IF(condition)                                                \
    if (condition) {                                                      \
        fprintf(stderr, "<%s%s%s>.%s%s%s() @%sline %d%s: %sExit%s", BOLD, \
                __FILE__, CLOSE, BOLD, __func__, CLOSE, BOLD_UNDERLINE,   \
                __LINE__, CLOSE, BOLD_PURPLE, CLOSE);                     \
        exit(1);                                                          \
    }

typedef struct tpool_work tpool_work_t;
typedef struct tpool      tpool_t;

typedef void (*thread_func_t)(void* arg);

struct tpool {
    tpool_work_t*   work_first;
    tpool_work_t*   work_last;
    pthread_mutex_t mutex;
    pthread_cond_t  new_work_cond;
    pthread_cond_t  working_cond;
    size_t          working_cnt;
    size_t          thread_cnt;
    thread_func_t   func;
    bool            stop;
};

bool tpool_set(tpool_t* pool, thread_func_t func, size_t n);
void tpool_clear(tpool_t* pool);
bool tpool_work_enqueue(tpool_t* pool, void* arg);
void tpool_wait(tpool_t* pool);

#endif
