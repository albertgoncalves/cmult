#if __linux__
    #define _DEFAULT_SOURCE
#endif

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lib.h"

/* NOTE: Based on `https://nachtimwald.com/2019/04/12/thread-pool-in-c/`. */

#define N_ITEMS 10

static const uint16_t  N_THREADS = 3;
static pthread_mutex_t MUTEX;

static void worker(void* arg) {
    usleep(100000);
    uint16_t copy = *(uint16_t*)arg;
    *(uint16_t*)arg += 100;
    pthread_mutex_lock(&MUTEX);
    printf("address: %p\tin: %hu\tout: %hu\n", (void*)pthread_self(), copy,
           *(uint16_t*)arg);
    pthread_mutex_unlock(&MUTEX);
}

int main(void) {
    EXIT_IF(pthread_mutex_init(&MUTEX, NULL) != 0);
    tpool_t pool;
    EXIT_IF(!tpool_set(&pool, worker, N_THREADS));
    uint16_t data[N_ITEMS] = {0};
    for (size_t i = 0; i < N_ITEMS; ++i) {
        data[i] = (uint16_t)i;
    }
    for (size_t i = 0; i < N_ITEMS; ++i) {
        EXIT_IF(!tpool_work_enqueue(&pool, &data[i]))
    }
    tpool_wait(&pool);
    for (size_t i = 0; i < N_ITEMS; ++i) {
        printf("%hu\n", data[i]);
    }
    tpool_clear(&pool);
    return EXIT_SUCCESS;
}
