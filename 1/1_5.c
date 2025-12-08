#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#define perror2(s, e) fprintf(stderr ,"%s: %s\n", s, strerror(e))

void* threadFunc(void*);

pthread_barrier_t barrier;

int main(int argc, char* argv[]){

    if(argc != 3){
        printf("Usage: %s num_of_threads num_of_iterations\n", argv[0]);
        exit(1);
    }

    int numOfThreads = atoi(argv[1]);
    int numOfIterations = atol(argv[2]);

    pthread_barrier_init(&barrier, NULL, numOfThreads);
    
    int err, status;
    pthread_t* threads = malloc(numOfThreads * sizeof(pthread_t));
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for(int i = 0; i < numOfThreads; i++){
        if((err = pthread_create(&threads[i], NULL, threadFunc, &numOfIterations))){
            perror2("pthread_create", err);
            exit(1);
        }
    }


    for(int i = 0; i < numOfThreads; i++){
        if((err = pthread_join(threads[i], (void**)&status))){
            perror2("pthread_join", err);
            exit(1);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("====Barrier with pthread_barrier_t====\n");
    printf("Elapsed time: %f seconds\n\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9);
    
    pthread_barrier_destroy(&barrier);

    free(threads);
    exit(0);

}

void* threadFunc(void* arg){

    int numOfIterations = *(int*)arg;

    // when all threads arrive at the barrier, they are released to continue
    // they basically, synchronize in each iteration
    for(int i = 0; i < numOfIterations; i++){
        pthread_barrier_wait(&barrier);
    }
    return NULL;
}