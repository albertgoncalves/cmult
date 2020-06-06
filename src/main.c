#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "exit.h"
#include "tpool.h"

/* NOTE: Based on `https://nachtimwald.com/2019/04/12/thread-pool-in-c/`. */

#define N_PAYLOADS 25
#define N_THREADS  3

typedef struct {
    u8 value;
} Payload;

typedef struct {
    TPool   pool;
    Payload payloads[N_PAYLOADS];
} Memory;

static ThreadMutex MUTEX;

static void thread_fn(void* arg) {
    Payload* payload = arg;
    u8       value = payload->value;
    payload->value = (u8)(value + 100);
    usleep(10000);
    LOCK_OR_EXIT(MUTEX);
    printf("in : \033[1m%4hhu\033[0m\t"
           "out : \033[1m%4hhu\033[0m\n",
           value,
           payload->value);
    UNLOCK_OR_EXIT(MUTEX);
}

int main(void) {
    EXIT_IF(pthread_mutex_init(&MUTEX, NULL) != 0);
    Memory* memory = calloc(1, sizeof(Memory));
    EXIT_IF(memory == NULL);
    {
        TPool*   pool = &memory->pool;
        Payload* payloads = memory->payloads;
        EXIT_IF(tpool_set(pool, thread_fn, N_THREADS) == FALSE);
        for (u8 i = 0; i < N_PAYLOADS; ++i) {
            payloads[i].value = (u8)i;
        }
        for (u8 i = 0; i < N_PAYLOADS; ++i) {
            EXIT_IF(tpool_work_enqueue(pool, &payloads[i]) == FALSE)
        }
        tpool_wait(pool);
        for (u8 i = 0; i < N_PAYLOADS; ++i) {
            printf("%hhu\n", payloads[i].value);
        }
        tpool_clear(pool);
    }
    free(memory);
    return EXIT_SUCCESS;
}
