#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>  
#include <string.h>

pthread_mutex_t coarse_mutex;

typedef struct {
    int transactions_read;
    int transactions_write;
    int number_of_accounts;
    unsigned long* account_balances;
    int type_of_sync;
    pthread_mutex_t* fine_mutexes;
} ThreadData;


void* thread_func(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned int seed = ts.tv_nsec ^ ts.tv_sec;
    unsigned long sum_of_balances_read = 0;


    // COARSE GRAINED MUTEX
    if(data->type_of_sync == 1){  // if is not placed inside the loop it will checked only once for all read transactions
        for(int i = 0; i < data->transactions_read; i++){
            int account = rand_r(&seed) % data->number_of_accounts;
            pthread_mutex_lock(&coarse_mutex);
                sum_of_balances_read += data->account_balances[account];
            pthread_mutex_unlock(&coarse_mutex);
        }
            
        // Perform write transactions
        for(int i = 0; i < data->transactions_write; i++){
            int account_1 = rand_r(&seed) % data->number_of_accounts;
            int account_2 = rand_r(&seed) % data->number_of_accounts;
            if(account_1 == account_2){
                account_2 = (account_2 + 1) % data->number_of_accounts;
            }
            unsigned int amount = rand_r(&seed);
            // we do not accept negative balances
            
            pthread_mutex_lock(&coarse_mutex);
                if(amount > data->account_balances[account_1]){
                    amount = data->account_balances[account_1];
                }
                data->account_balances[account_1] -= amount;
                data->account_balances[account_2] += amount;
            pthread_mutex_unlock(&coarse_mutex);
        }
    }

    // FINE GRAINED MUTEX
    else if(data->type_of_sync == 2){
        for(int i = 0; i < data->transactions_read; i++){
            int account = rand_r(&seed) % data->number_of_accounts;
            pthread_mutex_lock(&data->fine_mutexes[account]);
                sum_of_balances_read += data->account_balances[account];
            pthread_mutex_unlock(&data->fine_mutexes[account]);
        }
            
        // Perform write transactions
        for(int i = 0; i < data->transactions_write; i++){
            int account_1 = rand_r(&seed) % data->number_of_accounts;
            int account_2 = rand_r(&seed) % data->number_of_accounts;
            if(account_1 == account_2){
                account_2 = (account_2 + 1) % data->number_of_accounts;
            }
            unsigned int amount = rand_r(&seed);
            // we do not accept negative balances
            
            int first = account_1 < account_2 ? account_1 : account_2;
            int second = account_1 < account_2 ? account_2 : account_1;
                        
            // Lock in a consistent order to prevent deadlocks
            pthread_mutex_lock(&data->fine_mutexes[first]);
            pthread_mutex_lock(&data->fine_mutexes[second]);
            
                if(amount > data->account_balances[account_1]){
                    amount = data->account_balances[account_1];
                }
                data->account_balances[account_1] -= amount;
                data->account_balances[account_2] += amount;
            
            pthread_mutex_unlock(&data->fine_mutexes[second]);
            pthread_mutex_unlock(&data->fine_mutexes[first]);
        }

    }
    else if(data->type_of_sync == 3){

    }
    else if(data->type_of_sync == 4){

    }
    else{
        fprintf(stderr, "Invalid synchronization type\n");
        exit(1);
    }
    return NULL;
}



int main(int argc, char* argv[]) {
    if(argc != 6){
        fprintf(stderr, "Usage: %s <number_of_accounts> <transactions_per_thread> <percentage_of_read_transactions> <type_of_sync> <number_of_threads>\n", argv[0]);
        return 1;
    }
  
    int n = atoi(argv[1]);

    int transactions_per_thread = atoi(argv[2]);

    int percentage_of_read_transactions = atoi(argv[3]);
    if(percentage_of_read_transactions < 0 || percentage_of_read_transactions > 100){
        fprintf(stderr, "Percentage of read transactions must be between 0 and 100\n");
        return 1;
    }

    pthread_mutex_t* fine_mutexes = NULL;
    char type_of_sync[128];
    for (int i = 0; argv[4][i]; i++) {
        type_of_sync[i] = tolower(argv[4][i]);
    }
    type_of_sync[strlen(argv[4])] = '\0';
    int type_of_sync_flag;
    if(!strcmp(type_of_sync, "mutex_coarse") || !strcmp(type_of_sync, "coarse_mutex")){
        type_of_sync_flag = 1;
        pthread_mutex_init(&coarse_mutex, NULL);
    } 
    else if(!strcmp(type_of_sync, "mutex_fine") || !strcmp(type_of_sync, "fine_mutex")){
        type_of_sync_flag = 2;
        fine_mutexes = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * n);
        if(fine_mutexes == NULL){
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }
        for(int i = 0; i < n; i++){
            pthread_mutex_init(&fine_mutexes[i], NULL);
        }
    }
    else if(!strcmp(type_of_sync, "rwlock_coarse") || !strcmp(type_of_sync, "coarse_rwlock")){
        type_of_sync_flag = 3;
    }
    else if(!strcmp(type_of_sync, "rwlock_fine") || !strcmp(type_of_sync, "fine_rwlock")){
        type_of_sync_flag = 4;
    }
    else{
        fprintf(stderr, "Invalid type of synchronization method: %s\n", argv[4]);
        return 1;
    }
    
    snprintf(type_of_sync, sizeof(type_of_sync), "%s", argv[4]);
    int number_of_threads = atoi(argv[5]);

    unsigned long* account_balances = (unsigned long*)malloc(sizeof(unsigned long) * n);
    if(account_balances == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned int seed = ts.tv_nsec ^ ts.tv_sec;

    unsigned long sum_of_balances_initially = 0;
    for(int i = 0; i < n; i++){
        account_balances[i] = rand_r(&seed);
        sum_of_balances_initially += account_balances[i];
        

    }

    int transactions_read_per_thread = transactions_per_thread * percentage_of_read_transactions / 100;
    int transactions_write_per_thread = transactions_per_thread - transactions_read_per_thread;




    pthread_t* threads = (pthread_t*)malloc(number_of_threads * sizeof(pthread_t));
    if(threads == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    
    for(int i = 0; i < number_of_threads; i++){
        ThreadData* thread_data = (ThreadData*)malloc(sizeof(ThreadData));
        if(thread_data == NULL){
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }
        thread_data->transactions_read = transactions_read_per_thread;
        thread_data->transactions_write = transactions_write_per_thread;
        thread_data->number_of_accounts = n;
        thread_data->account_balances = account_balances;
        thread_data->type_of_sync = type_of_sync_flag;
        thread_data->fine_mutexes = fine_mutexes;

        if(pthread_create(&threads[i], NULL, thread_func, (void*)thread_data) != 0){
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }
    
    
    for(int i = 0; i < number_of_threads; i++){
        pthread_join(threads[i], NULL);
    }
    
    unsigned long sum_of_balances_finally = 0;
    for(int i = 0; i < n; i++){
        sum_of_balances_finally += account_balances[i];
    }

    if(sum_of_balances_initially != sum_of_balances_finally){
        fprintf(stderr, "Error: Sum of balances changed! Initial: %lu, Final: %lu\n", sum_of_balances_initially, sum_of_balances_finally);
    }
      

    if(type_of_sync_flag == 1){
        pthread_mutex_destroy(&coarse_mutex);
    }

    free(account_balances);
    free(threads);

    return 0;
}