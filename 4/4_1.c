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

    int* poly2_rev = malloc((degree+1)*sizeof(int));
    if(poly2_rev == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    // reverse array poly2 to be able to load contiguous elements
    for (int i = 0; i <= degree; i++) {
        poly2_rev[i] = poly2[degree - i];
    }


    for (int k = 0; k < result_size; k++) {
        int64_t sum = 0;

        int i_start = (k - degree) > 0 ? (k - degree) : 0;
        int i_end   = (k < degree) ? k : degree;

        __m256i vec_sum = _mm256_setzero_si256();

        int i = i_start;
        for (; i + 7 <= i_end; i += 8) {

            // load 8 elements from poly1 and poly2_rev
            __m256i a = _mm256_loadu_si256((__m256i*)&poly1[i]);
            __m256i b = _mm256_loadu_si256((__m256i*)&poly2_rev[degree - k+i]);

            // result is saved in 64 bit numbers to avoid overflow, so we cannot have all the results
            // -- at least in avx2 technology --
           __m256i prod_even = _mm256_mul_epi32(a, b);

           __m256i a_odd = _mm256_srli_si256(a, 4);
           __m256i b_odd = _mm256_srli_si256(b, 4);
           __m256i prod_odd = _mm256_mul_epi32(a_odd, b_odd);

           vec_sum = _mm256_add_epi64(vec_sum, prod_even);
           vec_sum = _mm256_add_epi64(vec_sum, prod_odd);
 
        }
        // horizontal sum of vec_sum
        __m128i hi128 = _mm256_extracti128_si256(vec_sum, 1);
        __m128i lo128 = _mm256_castsi256_si128(vec_sum);
        __m128i sum128 = _mm_add_epi64(hi128, lo128);
        // now we have 2 int64_t in sum128, we need to sum them up
        int64_t final_vec_sum[2];
        _mm_storeu_si128((__m128i*)final_vec_sum, sum128);
        sum = final_vec_sum[0] + final_vec_sum[1];

        // and the remainder
        for (; i <= i_end; i++) {
            sum += (int64_t)poly1[i] * poly2[k - i];
        }

        simd_result[k] = sum;
    }

    // end parallel algorithm timing
    clock_gettime(CLOCK_MONOTONIC, &simd_end);
    
    // Verification of results
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