#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


int main(int argc, char *argv[]){
    if (argc != 5) {
        printf("Usage: %s <number of rows> <percentage of 0 elements> <iterations> <threads>\n", argv[0]);
        return 1;
    }

    int dimension = atoi(argv[1]);
    float zero_percentage = atof(argv[2]);
    int iterations = atoi(argv[3]);
    int threads = atoi(argv[4]);

    if(zero_percentage < 0 || zero_percentage > 100){
        printf("Percentage of 0 elements must be between 0 and 100.\n");
        return 1;
    }

    // MATRIX NxN
    int** matrix = (int**)malloc(dimension * sizeof(int*));
    if(matrix == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }
    for(int i = 0; i < dimension; i++){
        matrix[i] = (int*)malloc(dimension * sizeof(int));
        if(matrix[i] == NULL){
            printf("Memory allocation failed\n");
            return 1;
        }
    } 

    // VECTOR Nx1
    int* vector = (int*)malloc(dimension * sizeof(int));
    if(vector == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned int seed = ts.tv_nsec ^ ts.tv_sec;
    int non_zero_elements_count = 0;
    //Initialize matrix and vector with random values and zeros
    for(int i = 0; i < dimension; i++){
        for(int j = 0; j < dimension; j++){
            bool zero_element = ((rand_r(&seed) % 100) < zero_percentage);
            if(zero_element){
                matrix[i][j] = 0;
            } 
            else {
                non_zero_elements_count++;
                bool sign = rand_r(&seed) % 2;
                // generate random non-zero value
                int value = (rand_r(&seed) % RAND_MAX) + 1;
                matrix[i][j] = sign ? value : -value;
            }
        }
        bool sign = rand_r(&seed) % 2;
        int value = (rand_r(&seed) % RAND_MAX) + 1;
        vector[i] = sign ? value : -value;
    }

    double start_init = omp_get_wtime();
    int* non_zero_values = (int*)malloc(non_zero_elements_count * sizeof(int));
    int* column_indices = (int*)malloc(non_zero_elements_count * sizeof(int));
    // save the total number of non-zero elements above the current row
    int* row_indeces = (int*)malloc((dimension + 1) * sizeof(int));
    #pragma omp parallel for num_threads(threads) default(none) shared(matrix, dimension, non_zero_values, column_indices, row_indeces)
        
    
    
    if(non_zero_values == NULL || column_indices == NULL || row_indeces == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }
    free(non_zero_values);
    free(column_indices);
    free(row_indeces);
    for(int i = 0; i < dimension; i++){
        free(matrix[i]);
    }
    free(matrix);
    free(vector);
    return 0;
}