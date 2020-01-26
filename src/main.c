#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"

#define N 10

const unsigned short int T = 2;
const unsigned short int M = 10;

pthread_mutex_t LOCK;
int K;

void *do_process() {
    sleep(T);
    pthread_mutex_lock(&LOCK);
    int i = 0;
    while (i < M) {
        printf("%d", K);
        i++;
    }
    printf("\t... done!\n");
    K++;
    pthread_mutex_unlock(&LOCK);
    return NULL;
}

int main(void) {
    pthread_t threads[N];
    if (pthread_mutex_init(&LOCK, NULL) != 0) {
        printf("Mutex initialization failed.\n");
        return 1;
    }
    K = 0;
    for (int i = 0; i < N; ++i) {
        pthread_create(&threads[i], NULL, do_process, NULL);
    }
    for (int i = 0; i < N; ++i) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
