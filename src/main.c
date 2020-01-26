#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"

#define N 3 /* NOTE: Number of threads. Be careful! */

const uint8_t T = (uint8_t)1;
const uint8_t M = (uint8_t)10;

pthread_mutex_t LOCK;

void *do_process(void *arg) {
    sleep(T);
    pthread_mutex_lock(&LOCK);
    uint8_t k = *(uint8_t *)arg;
    for (uint8_t i = 0; i < M; ++i) {
        printf("%d", k);
    }
    printf("\t... done!\n");
    pthread_mutex_unlock(&LOCK);
    return NULL;
}

int main(void) {
    pthread_t threads[N];
    if (pthread_mutex_init(&LOCK, NULL) != 0) {
        printf("Mutex initialization failed.\n");
        return 1;
    }
    uint8_t memory[N];
    for (uint8_t i = 0; i < N; ++i) {
        memory[i] = i;
    }
    for (uint8_t i = 0; i < N; ++i) {
        pthread_create(&threads[i], NULL, do_process, &memory[i]);
    }
    for (uint8_t i = 0; i < N; ++i) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
