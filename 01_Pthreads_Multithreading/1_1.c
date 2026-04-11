#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

// struct for the thread data
typedef struct {
    int* poly1;
    int* poly2;
    __int128_t* result;   // Result array (shared)
    int degree;
    int start_coeff;      // Starting coefficient this thread computes
    int end_coeff;        // Ending coefficient this thread computes
} ThreadData;

// Function executed by each thread
void* parallel_poly_mult(void* arg) {
    
    ThreadData* data = (ThreadData*)arg;
    
    // Each thread computes coefficients from start_coeff to end_coeff
    for(int k = data->start_coeff; k <= data->end_coeff; k++) {
        __int128_t sum = 0;

        // result has degree 2*degree so if k>degree, i starts from k-degree
        // because less degree coefficients can't result in k (etc if k=5 and degree=3
        // x cannot produce polynomial terms of degree >3 even with multiplication with
        // the larger degree of the 2nd polynomial
        int i_start = (k - data->degree) > 0 ? (k - data->degree) : 0;
        int i_end = (k < data->degree) ? k : data->degree;

        // For each coefficient k of the result:
        // We need to add all products poly1[i] * poly2[j] where i+j=k
        for (int i = i_start; i <= i_end; i++) {
            sum += (__int128_t)data->poly1[i] * data->poly2[k - i];
        }

        data->result[k] = sum;
    }
    
    return NULL;
}

// Function to verify results
bool verify_results(__int128_t* serial, __int128_t* parallel, int size) {
    for(int i = 0; i < size; i++) {
        if(serial[i] != parallel[i]) {
            printf("Mismatch at coefficient %d:\n", i);
            return false;
        }
    }
    return true;
}

// function to get time difference in seconds
double get_time_sec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + 
           (end.tv_nsec - start.tv_nsec) / 1000000000.0;
}



int main(int argc, char *argv[]){

    if(argc != 3){
        fprintf(stderr, "Usage: %s <Degree of the Polynomials> <Threads number>\n", argv[0]);
        return 1;
    }

    int degree = atoi(argv[1]);
    int threads_num = atoi(argv[2]);

    
    struct timespec init_start, init_end, serial_start, serial_end;
    struct timespec parallel_start, parallel_end;
    
    // start initialization timing 
    clock_gettime(CLOCK_MONOTONIC, &init_start);

    int* poly1 = (int*)malloc((degree + 1) * sizeof(int));
    if(poly1 == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    int* poly2 = (int*)malloc((degree + 1) * sizeof(int));
    if(poly2 == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    
    // Initialize polynomials with random coefficients between
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned int seed = ts.tv_nsec ^ ts.tv_sec;

    for(int i = 0; i <= degree; i++){
        //rand_r is thread-safe version of rand
        // Ensure non-zero coefficients by using (rand % max) + 1
        int value = (rand_r(&seed) % RAND_MAX) + 1;
        bool sign = rand_r(&seed) % 2;
        poly1[i] = sign ? -value : value;
    }
    for(int i = 0; i <= degree; i++){
        int value = (rand_r(&seed) % RAND_MAX) + 1;
        bool sign = rand_r(&seed) % 2;
        poly2[i] = sign ? -value : value;
    }

    int result_size = degree * 2 + 1;
    // Allocate result arrays with calloc to initialize to zero
    // result is int128 to avoid overflow for large degrees (rand returns numbers till 2^31)
    __int128_t* serial_result = calloc(result_size, sizeof(__int128_t));
    if(serial_result == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    __int128_t* parallel_result = calloc(result_size, sizeof(__int128_t));
    if(parallel_result == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    pthread_t* threads = (pthread_t*)malloc(threads_num * sizeof(pthread_t));
    if(threads == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    ThreadData* thread_data = (ThreadData*)malloc(threads_num * sizeof(ThreadData));
    if(thread_data == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // distribute work among threads
    // Each thread will compute a range of coefficients of the result
    int coeffs_per_thread = result_size / threads_num;
    int remaining = result_size % threads_num;
        
    int current_start = 0;
    for(int i = 0; i < threads_num; i++){
        thread_data[i].poly1 = poly1;
        thread_data[i].poly2 = poly2;
        thread_data[i].result = parallel_result;
        thread_data[i].degree = degree;
        thread_data[i].start_coeff = current_start;
        
        // the first 'remaining' threads get an extra coefficient to compute
        // so as to have the best distribution possible
        int my_coeffs = coeffs_per_thread + (i < remaining ? 1 : 0);
        thread_data[i].end_coeff = current_start + my_coeffs - 1;
        
        current_start = thread_data[i].end_coeff + 1;
    }

    //stop initialization timing
    clock_gettime(CLOCK_MONOTONIC, &init_end);
    
    // start serial algorithm timing
    clock_gettime(CLOCK_MONOTONIC, &serial_start);
    
    // Serial Polynomial Multiplication (same algorithm as parallel)
    for(int k = 0; k < result_size; k++) {
        __int128_t sum = 0;
        
        int i_start = (k - degree) > 0 ? (k - degree) : 0;
        int i_end = (k < degree) ? k : degree;
        
        // For each coefficient k of the result:
        // We need to add all products poly1[i] * poly2[j] where i+j=k
        for (int i = i_start; i <= i_end; i++) {
            sum += (__int128_t)poly1[i] * poly2[k - i];
        }
        
        serial_result[k] = sum;
    }
    
    // stop serial algorithm timing
    clock_gettime(CLOCK_MONOTONIC, &serial_end);
    
    // start parallel algorithm timing
    clock_gettime(CLOCK_MONOTONIC, &parallel_start);
    
    
    for(int i = 0; i < threads_num; i++){
        pthread_create(&threads[i], NULL, parallel_poly_mult, &thread_data[i]);
    }

    for(int i = 0; i < threads_num; i++){
        pthread_join(threads[i], NULL);
    }
    
    // end parallel algorithm timing
    clock_gettime(CLOCK_MONOTONIC, &parallel_end);
    
    // Verification of results
    printf("\n=== Verification of Results ===\n");
    bool correct = verify_results(serial_result, parallel_result, result_size);
    if(correct) {
        printf("The results are CORRECT!\n");
    } else {
        printf("The results DIFFER!\n");
    }
    
    // Print timings
    double init_time = get_time_sec(init_start, init_end);
    double serial_time = get_time_sec(serial_start, serial_end);
    double parallel_time = get_time_sec(parallel_start, parallel_end);
    double speedup = serial_time / parallel_time;
    
    printf("\n=== Times ===\n");
    printf("Initialization time: %.5f s\n", init_time);
    printf("Serial algorithm time: %.5f s\n", serial_time);
    printf("Parallel algorithm time: %.5f s\n", parallel_time);
    printf("Speedup: %.2f x\n", speedup);
    printf("Efficiency: %.2f %%\n", (speedup / threads_num) * 100);
    
    free(poly1);
    free(poly2);
    free(serial_result);
    free(parallel_result);
    free(threads);
    free(thread_data);

    return 0;
}