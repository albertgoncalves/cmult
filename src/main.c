#if __linux__
#define _DEFAULT_SOURCE
#endif

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lib.h"

/* NOTE: Based on `https://nachtimwald.com/2019/04/12/thread-pool-in-c/`. */

#define N_ITEMS 30

static const uint8_t   N_THREADS = 3;
static pthread_mutex_t MUTEX;

static void worker(void* arg) {
    usleep(250000);
    uint8_t copy = *(uint8_t*)arg;
    *(uint8_t*)arg += 100;
    pthread_mutex_lock(&MUTEX);
    printf("address: %p\tin: %hhu\tout: %hhu\n", (void*)pthread_self(), copy,
           *(uint8_t*)arg);
    pthread_mutex_unlock(&MUTEX);
}

int main(void) {
    assert(pthread_mutex_init(&MUTEX, NULL) == 0);
    tpool_t pool;
    tpool_set(&pool, N_THREADS);
    uint8_t data[N_ITEMS] = {0};
    for (uint8_t i = 0; i < N_ITEMS; ++i) {
        data[i] = i;
    }
    for (uint8_t i = 0; i < N_ITEMS; ++i) {
        tpool_work_enqueue(&pool, worker, &data[i]);
    }
    usleep(10); /* NOTE: The pool sometimes doesn't kick off. This tiny pause
                 * seems to help... but, why? What is going on here?
                 */
    tpool_wait(&pool);
    for (uint8_t i = 0; i < N_ITEMS; ++i) {
        printf("%hhu\n", data[i]);
    }
    tpool_clear(&pool);
    return EXIT_SUCCESS;
}
