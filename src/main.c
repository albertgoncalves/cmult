#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include "exit.h"
#include "tpool.h"

/* NOTE: Based on `https://nachtimwald.com/2019/04/12/thread-pool-in-c/`. */

#define T         uint16_t
#define N_ITEMS   25
#define N_THREADS 4

static pthread_mutex_t MUTEX;

static void worker(void* arg) {
    usleep(10000);
    const T copy = *(T*)arg;
    *(T*)arg = (T)(*(T*)arg + 100);
    pthread_mutex_lock(&MUTEX);
    const uint64_t id = pthread_self();
    printf("id: %lu\tin: %hu\tout: %hu\n", id, copy, *(T*)arg);
    pthread_mutex_unlock(&MUTEX);
}

int main(void) {
    EXIT_IF(pthread_mutex_init(&MUTEX, NULL) != 0);
    tpool_t* pool = calloc(1, sizeof(tpool_t));
    EXIT_IF(!tpool_set(pool, worker, N_THREADS));
    T* data = calloc(N_ITEMS, sizeof(T));
    for (size_t i = 0; i < N_ITEMS; ++i) {
        data[i] = (T)i;
    }
    for (size_t j = 0; j < 2; ++j) {
        for (size_t i = 0; i < N_ITEMS; ++i) {
            EXIT_IF(!tpool_work_enqueue(pool, &data[i]))
        }
        tpool_wait(pool);
    }
    for (size_t i = 0; i < N_ITEMS; ++i) {
        printf("%hu\n", data[i]);
    }
    tpool_clear(pool);
    free(pool);
    free(data);
    return EXIT_SUCCESS;
}
