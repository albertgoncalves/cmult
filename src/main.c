#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "lib.h"

#define N 3 /* NOTE: Number of threads. Be careful! */

int main(void) {
    pthread_t threads[N];
    if (pthread_mutex_init(&PRINT_LOCK, NULL) != 0) {
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
