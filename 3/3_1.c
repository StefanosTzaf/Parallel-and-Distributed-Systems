#include <stdio.h>
#include <mpi.h>
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
    int my_rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    if(my_rank == 0) {
        if (argc != 2) {
            printf("Usage: %s <Degree of the Polynomials>\n", argv[0]);
            return 1;
        }
        int degree = atoi(argv[1]);
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
    
        
        double serial_start, serial_end;
        // start serial algorithm timing
        serial_start = MPI_Wtime();
        
        //Serial Polynomial Multiplication
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
        serial_end = MPI_Wtime();
        printf("Serial Polynomial Multiplication Time: %f seconds\n", serial_end - serial_start);
        free(poly1);
        free(poly2);
        free(serial_result);

    }
    else{
        
    }



    
    MPI_Finalize();
    return 0;
}