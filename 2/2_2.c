#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

void check_results(long long* res1, long long* res2, int dimension){
    for(int i = 0; i < dimension; i++){
        if(res1[i] != res2[i]){
            printf("Results do not match at index %d: %lld != %lld\n", i, res1[i], res2[i]);
            exit(1);
        }
    }
    printf("Results match!\n");
}


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

    // VECTOR Nx1 declare as long long to be able to easily assign it to results without copying 
    long long* vector = (long long*)malloc(dimension * sizeof(long long));
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

    // initialize CSR arrays
    double start_init = omp_get_wtime();
    int* non_zero_values = (int*)malloc(non_zero_elements_count * sizeof(int));
    int* column_indices = (int*)malloc(non_zero_elements_count * sizeof(int));
    // save the total number of non-zero elements above the current row
    int* row_indeces = (int*)malloc((dimension + 1) * sizeof(int));
    if(non_zero_values == NULL || column_indices == NULL || row_indeces == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }

    // count of non-zero elements in each row
    int *row_nnz = malloc(dimension * sizeof(int));
    if(row_nnz == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }

    #pragma omp parallel for num_threads(threads) default(none) shared(matrix, dimension, row_nnz)
        for (int i = 0; i < dimension; i++) {
            int count = 0;
            for (int j = 0; j < dimension; j++) {
                if (matrix[i][j] != 0){
                    count++;
                } 
            }
            row_nnz[i] = count;
        }
    

    // compute row_indeces (cannot be parallelized due to dependencies)
    row_indeces[0] = 0;
    for (int i = 0; i < dimension; i++) {
        row_indeces[i + 1] = row_indeces[i] + row_nnz[i];
    }

   
    #pragma omp parallel for num_threads(threads) default(none) shared(matrix, non_zero_values, column_indices, row_indeces, dimension)
    for (int i = 0; i < dimension; i++) {
        int idx = row_indeces[i];

        //TODO : check if we have already found all non-zero elements of this row so to continue to the next
        for (int j = 0; j < dimension; j++) {
            if (matrix[i][j] != 0) {
                non_zero_values[idx]  = matrix[i][j];
                column_indices[idx] = j;
                idx++;
            }
        }
    }
    double end_init = omp_get_wtime();

    // we have to copy the initial vector to restore it later for dense multiplication
    long long* current_vec = (long long*)malloc(dimension * sizeof(long long));
    if(current_vec == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }

    for(int i = 0; i < dimension; i++){
        current_vec[i] = vector[i];
    }

    long long* result = (long long*)malloc(dimension * sizeof(long long));
    if(result == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }

    double start_parallel_csr = omp_get_wtime();
    for(int i = 0; i < iterations; i++){
        #pragma omp parallel for num_threads(threads) default(none) shared(dimension, row_indeces, column_indices, non_zero_values, current_vec, result)
            for(int row = 0; row < dimension; row++){
                long long sum = 0;
                // this way we know exactly how many non-zero elements to process in this row
                for(int idx = row_indeces[row]; idx < row_indeces[row + 1]; idx++){
                    sum += (long long)non_zero_values[idx] * current_vec[column_indices[idx]];
                }
                result[row] = sum;
            }
        
        // Swap vectors and do not copy
        long long* temp = current_vec;
        current_vec = result;
        // now current_vec points to the result as new input and result to the old
        // just so as not to write to the same array
        result = temp;
    }

    double end_parallel_csr = omp_get_wtime();

    // save results to compare with dense multiplication
    long long* results1 = malloc(dimension * sizeof(long long));
    if(results1 == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }
    for(int i = 0; i < dimension; i++){
        results1[i] = current_vec[i];
    }


    double start_parallel_dense = omp_get_wtime();
    current_vec = vector;
    // multiply with dense way for comparison
     for(int i = 0; i < iterations; i++){
        #pragma omp parallel for num_threads(threads) default(none) shared(dimension, matrix, current_vec, result)
            for(int row = 0; row < dimension; row++){
                long long sum = 0;
                for(int col = 0; col < dimension; col++){
                    sum += (long long)matrix[row][col] * current_vec[col];
                }
                result[row] = sum;
            }
        
        // Swap vectors and do not copy
        long long* temp = current_vec;
        current_vec = result;
        // now current_vec points to the result as new input and result to the old
        // just so as not to write to the same array
        result = temp;
        
    }
    double end_parallel_dense = omp_get_wtime();

    // check if results are the same
    check_results(results1, current_vec, dimension);
    free(results1);

    printf("Initialization Time: %f seconds\n", end_init - start_init);
    printf("Parallel CSR Multiplication Time: %f seconds\n", end_parallel_csr - start_parallel_csr);
    printf("Parallel Dense Multiplication Time: %f seconds\n", end_parallel_dense - start_parallel_dense);
    free(non_zero_values);
    free(column_indices);
    free(row_indeces);
    for(int i = 0; i < dimension; i++){
        free(matrix[i]);
    }
    free(matrix);
    free(vector);
    free(row_nnz);
    return 0;
 }