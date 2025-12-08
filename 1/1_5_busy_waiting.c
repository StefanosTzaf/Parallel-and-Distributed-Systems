#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#define perror2(s, e) fprintf(stderr ,"%s: %s\n", s, strerror(e))

void* threadFunc(void*);


struct threadBarrier
{
    int thread_barrier_number; // number of threads required to pass the barrier
    int total_thread; // number of threads that have reached the barrier
    pthread_mutex_t lock; // mutex lock for total_thread and flag
    bool flag; // indicates whether thread_barrier_number threads have reached the barrier
} thread_barrier; 

// forward declarations
void thread_barrier_init(struct threadBarrier*, pthread_mutexattr_t*, int);
void thread_barrier_wait(struct threadBarrier*);
void* threadFunc(void* arg);


int main(int argc, char* argv[]){

    if(argc != 3){
        printf("Usage: %s num_of_threads num_of_iterations\n", argv[0]);
        exit(1);
    }

    int numOfThreads = atoi(argv[1]);
    int numOfIterations = atol(argv[2]);
    
    int err, status;
    pthread_t* threads = malloc(numOfThreads * sizeof(pthread_t));

    thread_barrier_init(&thread_barrier, NULL, numOfThreads);

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

    printf("====Busy Waiting Barrier====\n");
    printf("Elapsed time: %f seconds\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9);


    pthread_mutex_destroy(&thread_barrier.lock);

    free(threads);
    exit(0);

}

void* threadFunc(void* arg){

    int numOfIterations = *(int*)arg;

    for(int i = 0; i < numOfIterations; i++){
        thread_barrier_wait(&thread_barrier);

    }



    return NULL;
}

void thread_barrier_init(struct threadBarrier* barrier, pthread_mutexattr_t *mutex_attr, int thread_barrier_number){
    pthread_mutex_init(&(barrier->lock), mutex_attr);

    barrier->total_thread = 0;
    barrier->thread_barrier_number = thread_barrier_number;
    barrier->flag = false;
}


void thread_barrier_wait(struct threadBarrier* barrier){
   
    bool local_sense = barrier->flag;
    
    if(pthread_mutex_lock(&(barrier->lock)) == 0){
        
        barrier->total_thread += 1;
        local_sense = !local_sense;
        
        // last thread to reach the barrier: reset count and toggle flag
        if (barrier->total_thread == barrier->thread_barrier_number){
         
            barrier->total_thread = 0;
            barrier->flag = local_sense;
            pthread_mutex_unlock(&(barrier->lock));
        } 
        // not the last thread: wait for the flag to change
        else {
            pthread_mutex_unlock(&(barrier->lock));
            while (barrier->flag != local_sense); // busy waiting for the flag to change
        }
    }
}