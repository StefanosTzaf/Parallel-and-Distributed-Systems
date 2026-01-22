#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <immintrin.h>

// Function to verify results
bool verify_results(int64_t* serial, int64_t* parallel, int size) {
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

    if(argc != 2){
        fprintf(stderr, "Usage: %s <Degree of the Polynomials>\n", argv[0]);
        return 1;
    }

    int degree = atoi(argv[1]);


    
    struct timespec init_start, init_end, serial_start, serial_end;
    struct timespec simd_start, simd_end;
    
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

    
    // Initialize polynomials with random coefficients
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    srand(time(NULL));

    for(int i = 0; i <= degree; i++){
        // Ensure non-zero coefficients by using (rand % max) + 1
        int value = (rand() % RAND_MAX) + 1;
        bool sign = rand() % 2;
        poly1[i] = sign ? -value : value;
    }
    for(int i = 0; i <= degree; i++){
        int value = (rand() % RAND_MAX) + 1;
        bool sign = rand() % 2;
        poly2[i] = sign ? -value : value;
    }

    int result_size = degree * 2 + 1;
    // Allocate result arrays with calloc to initialize to zero
    // result is int128 to avoid overflow for large degrees (rand returns numbers till 2^31)
    int64_t* serial_result = calloc(result_size, sizeof(int64_t));
    if(serial_result == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    int64_t* simd_result = calloc(result_size, sizeof(int64_t));
    if(simd_result == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    //stop initialization timing
    clock_gettime(CLOCK_MONOTONIC, &init_end);
    
    // start serial algorithm timing
    clock_gettime(CLOCK_MONOTONIC, &serial_start);
    
    // Serial Polynomial Multiplication
    for(int k = 0; k < result_size; k++) {
        int64_t sum = 0;
        
        int i_start = (k - degree) > 0 ? (k - degree) : 0;
        int i_end = (k < degree) ? k : degree;
        
        // For each coefficient k of the result:
        // We need to add all products poly1[i] * poly2[j] where i+j=k
        for (int i = i_start; i <= i_end; i++) {
            sum += (int64_t)poly1[i] * poly2[k - i];
        }
        
        serial_result[k] = sum;
    }
    

    // stop serial algorithm timing
    clock_gettime(CLOCK_MONOTONIC, &serial_end);
        // start parallel algorithm timing
    clock_gettime(CLOCK_MONOTONIC, &simd_start);

    // THIS WAY we can exploit only 50% of parallelism of AVX2 because there is no mul for 64bit integers
    for (int k = 0; k < result_size; k++) {
        int64_t sum = 0;

        int i_start = (k - degree) > 0 ? (k - degree) : 0;
        int i_end   = (k < degree) ? k : degree;

        __m256i vec_sum = _mm256_setzero_si256();

        int i = i_start;
        for (; i + 3 <= i_end; i += 4) {
            
            __m256i a = _mm256_set_epi32(
                0, poly1[i+3],
                0, poly1[i+2],
                0, poly1[i+1],
                0, poly1[i]
            );

            __m256i b = _mm256_set_epi32(
                0, poly2[k-(i+3)],
                0, poly2[k-(i+2)],
                0, poly2[k-(i+1)],
                0, poly2[k-i]
            );

            __m256i prod = _mm256_mul_epi32(a, b);
            vec_sum = _mm256_add_epi64(vec_sum, prod);
        }

        int64_t tmp[4];
        _mm256_storeu_si256((__m256i*)tmp, vec_sum);
        sum += tmp[0] + tmp[1] + tmp[2] + tmp[3];

        for (; i <= i_end; i++) {
            sum += (int64_t)poly1[i] * poly2[k - i];
        }

        simd_result[k] = sum;
    }

    // end parallel algorithm timing
    clock_gettime(CLOCK_MONOTONIC, &simd_end);
    
    //Verification of results
    printf("\n=== Verification of Results ===\n");
    bool correct = verify_results(serial_result, simd_result, result_size);
    if(correct) {
        printf("The results are CORRECT!\n");
    } else {
        printf("The results DIFFER!\n");
    }
    
    // Print timings
    double init_time = get_time_sec(init_start, init_end);
    double serial_time = get_time_sec(serial_start, serial_end);
    double simd_time = get_time_sec(simd_start, simd_end);
    
    printf("\n=== Times ===\n");
    printf("Initialization time: %.5f s\n", init_time);
    printf("Serial algorithm time: %.5f s\n", serial_time);
    printf("Parallel algorithm time: %.5f s\n", simd_time);
    
    free(poly1);
    free(poly2);
    free(serial_result);
    free(simd_result);

    return 0;
}