#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define perror2(s, e) fprintf(stderr ,"%s: %s\n", s, strerror(e))

int sharedVar;
pthread_mutex_t mutex;
pthread_rwlock_t rwlock;

void* threadFunc_mutex(void*);
void* threadFunc_rwlock(void*);
void* threadFunc_atomic(void*);

int main(int argc, char* argv[]){
    
    if(argc != 4){
        printf("Usage: %s number_of_threads number_of_iterations num_of_algorithm\n", argv[0]);
        exit(1);
    }
    
    sharedVar = 0;
    
    int threadsNum = atoi(argv[1]); // number of threads
    int numOfIter = atoi(argv[2]); // number of iterations for each thread
    int algorithm = atoi(argv[3]); // 1: mutex, 2: rwlock, 3: atomic

    pthread_t* threads = malloc(threadsNum*sizeof(pthread_t));

    void* (*threadFunc)(void*); // function pointer to thread function

    switch (algorithm){
        case 1:
            printf("---Using mutex lock---\n");
            pthread_mutex_init(&mutex, NULL);
            threadFunc = threadFunc_mutex;
            break;
        case 2:
            printf("---Using rwlock---\n");
            pthread_rwlock_init(&rwlock, NULL);
            threadFunc = threadFunc_rwlock;
            break;
        case 3:
            printf("---Using atomic operations---\n");
            threadFunc = threadFunc_atomic;
            break;
        default:
            printf("Invalid algorithm number: Use 1 for mutex, 2 for rwlock, 3 for atomic.\n");
            exit(1);
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    int err, status;
    for(int i = 0; i < threadsNum; i++){
        if((err = pthread_create(&threads[i], NULL, threadFunc, (void*)&numOfIter))){
            perror2("pthread_create", err);
            exit(1);
        }
    }

    for(int i = 0; i < threadsNum; i++){
        if((err = pthread_join(threads[i], (void**)&status))){
            perror2("pthread_join", err);
            exit(1);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);
    // Calculate total time taken, take into account nanoseconds as well
    double totalTime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Final value of sharedVar: %d\n", sharedVar);
    printf("Time: %f seconds\n", totalTime);

    switch (algorithm){
        case 1:
            pthread_mutex_destroy(&mutex);
            break;
        case 2:
            pthread_rwlock_destroy(&rwlock);
            break;
        case 3:
            // no destruction needed for atomic
            break;
        default:
            break;
    }                   
    
    free(threads);
    return 0;
}


void* threadFunc_mutex(void* arg){
    int iters = *(int*)arg;

    for(int i = 0; i < iters; i++){
        pthread_mutex_lock(&mutex);
        sharedVar++;
        // printf("in thread %ld with sharedVal: %d\n", pthread_self(), sharedVar);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void* threadFunc_rwlock(void* arg){
    int iters = *(int*)arg;

    for(int i = 0; i < iters; i++){
        // write lock for incrementing sharedVar
        pthread_rwlock_wrlock(&rwlock);
        sharedVar++;
        pthread_rwlock_unlock(&rwlock);
        
        // read lock for printing only
        pthread_rwlock_rdlock(&rwlock);
        // printf("in thread %ld with sharedVal: %d\n", pthread_self(), sharedVar);
        pthread_rwlock_unlock(&rwlock);
    }
    pthread_exit(NULL);
}

void* threadFunc_atomic(void* arg){
    int iters = *(int*)arg;

    for(int i = 0; i < iters; i++){
        
        // memory order relaxed since we do not care about the order of the threads
        __atomic_add_fetch(&sharedVar, 1, __ATOMIC_RELAXED);
        // printf("in thread %ld with sharedVal: %d\n", pthread_self(), sharedVar);
    }
    pthread_exit(NULL);

}