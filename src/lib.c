#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "lib.h"

const uint8_t T = (uint8_t)1;
const uint8_t M = (uint8_t)10;

void *do_process(void *arg) {
    sleep(T);
    pthread_mutex_lock(&PRINT_LOCK);
    uint8_t k = *(uint8_t *)arg;
    for (uint8_t i = 0; i < M; ++i) {
        printf("%d", k);
    }
    printf("\t... done!\n");
    pthread_mutex_unlock(&PRINT_LOCK);
    return NULL;
}
