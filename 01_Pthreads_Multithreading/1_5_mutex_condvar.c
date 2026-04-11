#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#define perror2(s, e) fprintf(stderr ,"%s: %s\n", s, strerror(e))

void* threadFunc(void*);

struct args {
    int numOfThreads;
    int numOfIterations;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condvar = PTHREAD_COND_INITIALIZER;

int count = 0; // number of threads that have reached the barrier


int main(int argc, char* argv[]){

    if(argc != 3){
        printf("Usage: %s num_of_threads num_of_iterations\n", argv[0]);
        exit(1);
    }

    int numOfThreads = atoi(argv[1]);
    int numOfIterations = atol(argv[2]);

    struct args threadArgs;
    threadArgs.numOfThreads = numOfThreads;
    threadArgs.numOfIterations = numOfIterations;

    int err, status;
    pthread_t* threads = malloc(numOfThreads * sizeof(pthread_t));
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(int i = 0; i < numOfThreads; i++){
        if((err = pthread_create(&threads[i], NULL, threadFunc, &threadArgs))){
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

    printf("====Mutex and Condition Variable Barrier====\n");
    printf("Elapsed time: %f seconds\n\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condvar);
    

    free(threads);
    exit(0);

}

void* threadFunc(void* arg){

    struct args* threadArgs = (struct args*)arg;
    int numOfIterations = threadArgs->numOfIterations;
    int numOfThreads = threadArgs->numOfThreads;


    // when all threads arrive at the barrier, they are released to continue
    // they basically, synchronize in each iteration
    for(int i = 0; i < numOfIterations; i++){

        pthread_mutex_lock(&mutex);
        count++;

        if (count == numOfThreads){
            count = 0; 
            pthread_cond_broadcast(&condvar);
        }
        else{
            while(pthread_cond_wait(&condvar, &mutex) != 0);
        }

        pthread_mutex_unlock(&mutex);       
        
    }



    return NULL;
}