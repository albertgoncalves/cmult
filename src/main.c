#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lib.h"

/* NOTE: Based on `https://nachtimwald.com/2019/04/12/thread-pool-in-c/`. */

#define N_ITEMS 10

static const uint8_t   N_THREADS = 3;
static pthread_mutex_t MUTEX;

void worker(void*);
void worker(void* arg) {
    sleep(1);
    uint8_t copy = *(uint8_t*)arg;
    *(uint8_t*)arg += 100;
    pthread_mutex_lock(&MUTEX);
    printf("address: %p | in: %hhu | out: %hhu\n", pthread_self(), copy,
           *(uint8_t*)arg);
    pthread_mutex_unlock(&MUTEX);
}

int main(void) {
    assert(pthread_mutex_init(&MUTEX, NULL) == 0);
    tpool_t* pool          = tpool_create(N_THREADS);
    uint8_t  data[N_ITEMS] = {0};
    for (uint8_t i = 0; i < N_ITEMS; ++i) {
        data[i] = i;
    }
    for (uint8_t i = 0; i < N_ITEMS; ++i) {
        tpool_add_work(pool, worker, &data[i]);
    }
    tpool_wait(pool);
    for (uint8_t i = 0; i < N_ITEMS; ++i) {
        printf("%hhu\n", data[i]);
    }
    tpool_destroy(pool);
    return 0;
}
