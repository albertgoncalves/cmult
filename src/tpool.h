#ifndef __TPOOL_H__
#define __TPOOL_H__

typedef unsigned char u8;
typedef unsigned long u64;

typedef pthread_cond_t  ThreadCond;
typedef pthread_mutex_t ThreadMutex;

#define LOCK_OR_EXIT(mutex)   EXIT_IF(pthread_mutex_lock(&mutex) != 0);
#define UNLOCK_OR_EXIT(mutex) EXIT_IF(pthread_mutex_unlock(&mutex) != 0);

typedef enum {
    FALSE = 0,
    TRUE,
} Bool;

#define CAPACITY 64

typedef void (*ThreadFn)(void*);

typedef struct {
    void*       memory[CAPACITY];
    ThreadCond  enqueue_cond;
    ThreadCond  dequeue_cond;
    ThreadMutex mutex;
    ThreadFn    fn;
    u8          active_threads;
    u8          total_threads;
    u8          queue_len;
    u8          index;
    Bool        stop;
} TPool;

Bool tpool_set(TPool*, const ThreadFn, const u8);
void tpool_clear(TPool*);
Bool tpool_work_enqueue(TPool*, void*);
void tpool_wait(TPool*);

#endif
