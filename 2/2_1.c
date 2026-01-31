#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


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


int main(int argc, char *argv[]){

    if(argc != 3){
        fprintf(stderr, "Usage: %s <Degree of the Polynomials> <Threads number>\n", argv[0]);
        return 1;
    }

    int degree = atoi(argv[1]);
    int threads_num = atoi(argv[2]);

    
    double init_start, init_end, serial_start, serial_end;
    double parallel_start, parallel_end;
    
    // start initialization timing 
    init_start = omp_get_wtime();

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

    
    // Initialize polynomials with random coefficients
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

    init_end = omp_get_wtime();

    // start serial algorithm timing
    serial_start = omp_get_wtime();
    
    //Serial Polynomial Multiplication (same algorithm as parallel)
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
    serial_end = omp_get_wtime();



    // start parallel algorithm timing
    parallel_start = omp_get_wtime();

    #pragma omp parallel for num_threads(threads_num) schedule(guided) default(none) shared(poly1, poly2, parallel_result, degree, result_size)
    for(int k = 0; k < result_size; k++) {
        __int128_t sum = 0;
        
            int i_start = (k - degree) > 0 ? (k - degree) : 0;
            int i_end = (k < degree) ? k : degree;
            
            for (int i = i_start; i <= i_end; i++) {
                sum += (__int128_t)poly1[i] * poly2[k - i];
            }
            
            parallel_result[k] = sum;
            printf("Thread %d starting computation.\n", omp_get_thread_num());
        }


    parallel_end = omp_get_wtime();

    // Verification of results
    printf("\n=== Verification of Results ===\n");
    bool correct = verify_results(serial_result, parallel_result, result_size);
    if(correct) {
        printf("The results are CORRECT!\n");
    } else {
        printf("The results DIFFER!\n");
    }
    
    // Print timings
    double init_time = init_end - init_start;
    double serial_time = serial_end - serial_start;
    double parallel_time = parallel_end - parallel_start;
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
    return 0;
}