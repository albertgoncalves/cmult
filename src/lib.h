#ifndef __LIB_H__
#define __LIB_H__

#include <stdbool.h>

typedef void (*thread_func_t)(void* arg);

typedef struct tpool_work tpool_work_t;
typedef struct tpool      tpool_t;

tpool_t* tpool_create(size_t n);
void     tpool_destroy(tpool_t* pool);
bool     tpool_work_push(tpool_t* pool, thread_func_t func, void* arg);
void     tpool_wait(tpool_t* pool);

#endif
