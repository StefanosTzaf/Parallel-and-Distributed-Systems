#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

void check_CSR_initialization(int* row_indeces, int* column_indeces, int* non_zero_values,
                              int* row_indeces_serial, int* column_indeces_serial, int* non_zero_values_serial,
                              int dimension);
void check_results(long long* result1, long long* result2, int dimension);
 


 int main(int argc, char *argv[]){
    if (argc != 5 && argc != 6) {
        printf("Usage: %s <number of rows> <percentage of 0 elements> <iterations> <threads> <Y for non-uniform distribution>\n", argv[0]);
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

    // if non uniform distribution is selected pack all zeros at the beginning of the matrix
    if(argc == 6){
        if(argv[5][0] == 'y' || argv[5][0] == 'Y'){
            int zero_elements = (int)((zero_percentage / 100.0) * dimension * dimension);
            int count = 0;
            for(int i = 0; i < dimension; i++){
                for(int j = 0; j < dimension; j++){
                    if(count < zero_elements){
                        matrix[i][j] = 0;
                        count++;
                    }
                    else{
                        bool sign = rand_r(&seed) % 2;
                        int value = (rand_r(&seed) % RAND_MAX) + 1;
                        matrix[i][j] = sign ? value : -value;
                    }
                }
                bool sign = rand_r(&seed) % 2;
                int value = (rand_r(&seed) % RAND_MAX) + 1;
                vector[i] = sign ? value : -value;
            }
        }
        else{
            printf("Invalid argument for non-uniform distribution. Use 'Y' for yes. Else use no argument.\n");
            return 1;
        }
    }
    else{
        //Initialize matrix and vector with random values and zeros
        for(int i = 0; i < dimension; i++){
            for(int j = 0; j < dimension; j++){
                bool zero_element = ((rand_r(&seed) % 100) < zero_percentage);
                if(zero_element){
                    matrix[i][j] = 0;
                } 
                else {
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
    }


    double start_init_serial = omp_get_wtime();
    // initialize CSR arrays - serial
        
        int* row_indeces_serial = (int*)malloc((dimension + 1) * sizeof(int));
        // count of non-zero elements in each row
        int *row_nnz_serial = malloc(dimension * sizeof(int));
        if(row_indeces_serial == NULL || row_nnz_serial == NULL){
            printf("Memory allocation failed\n");
            return 1;
        }
        for (int i = 0; i < dimension; i++) {
            int count = 0;
            for (int j = 0; j < dimension; j++) {
                if (matrix[i][j] != 0){
                    count++;
                } 
            }
            row_nnz_serial[i] = count;
        }
        // compute row_indeces
        row_indeces_serial[0] = 0;
        for (int i = 0; i < dimension; i++) {
            row_indeces_serial[i + 1] = row_indeces_serial[i] + row_nnz_serial[i];
        }
        free(row_nnz_serial);
        // 1st and 2nd CSR arrays
        int* non_zero_values_serial = (int*)malloc(row_indeces_serial[dimension] * sizeof(int));
        int* column_indeces_serial = (int*)malloc(row_indeces_serial[dimension] * sizeof(int));
        if(non_zero_values_serial == NULL || column_indeces_serial == NULL){
            printf("Memory allocation failed\n");
            return 1;
        }
        for (int i = 0; i < dimension; i++) {
            int idx = row_indeces_serial[i];
            for (int j = 0; j < dimension; j++) {
                if (matrix[i][j] != 0) {
                    non_zero_values_serial[idx]  = matrix[i][j];
                    column_indeces_serial[idx] = j;
                    idx++;
                }
            }
        }

    double end_init_serial = omp_get_wtime();


    double start_init = omp_get_wtime();
    // initialize CSR arrays - parallel

        // first compute the 3rd CSR array because we will need to know the number of non-zero elements
        int* row_indeces = (int*)malloc((dimension + 1) * sizeof(int));
        // count of non-zero elements in each row
        int *row_nnz = malloc(dimension * sizeof(int));
        if(row_indeces == NULL || row_nnz == NULL){
            printf("Memory allocation failed\n");
            return 1;
        }
        #pragma omp parallel for num_threads(threads) schedule(runtime) default(none) shared(matrix, dimension, row_nnz)
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
        free(row_nnz);
    
        // 1st and 2nd CSR arrays
        int* non_zero_values = (int*)malloc(row_indeces[dimension] * sizeof(int));
        int* column_indeces = (int*)malloc(row_indeces[dimension] * sizeof(int));
        // save the total number of non-zero elements above the current row
        if(non_zero_values == NULL || column_indeces == NULL){
            printf("Memory allocation failed\n");
            return 1;
        }
    
        #pragma omp parallel for num_threads(threads) schedule(runtime) default(none) shared(matrix, non_zero_values, column_indeces, row_indeces, dimension)
        for (int i = 0; i < dimension; i++) {
            int idx = row_indeces[i];
            for (int j = 0; j < dimension; j++) {
                if (matrix[i][j] != 0) {
                    non_zero_values[idx]  = matrix[i][j];
                    column_indeces[idx] = j;
                    idx++;
                }
            }
        }

    double end_init = omp_get_wtime();
    
    check_CSR_initialization(row_indeces, column_indeces, non_zero_values, row_indeces_serial, column_indeces_serial, non_zero_values_serial, dimension);

    // we have to copy the initial vector to restore it later for dense multiplication
    // is not counted in the time measurements because we only do it to be able to run both multiplications
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
    // multiply with CSR format - parallel
        
        for(int i = 0; i < iterations; i++){
            #pragma omp parallel for num_threads(threads) schedule(runtime) default(none) shared(dimension, row_indeces, column_indeces, non_zero_values, current_vec, result)
                for(int row = 0; row < dimension; row++){
                    long long sum = 0;
                    // this way we know exactly how many non-zero elements to process in this row
                    for(int idx = row_indeces[row]; idx < row_indeces[row + 1]; idx++){
                        sum += (long long)non_zero_values[idx] * current_vec[column_indeces[idx]];
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
    // re-initialize current_vec with the original input vector values
    for(int i = 0; i < dimension; i++){
        current_vec[i] = vector[i];
    }

    double start_serial_csr = omp_get_wtime();
    // multiply with CSR way for comparison - serial
        for(int i = 0; i < iterations; i++){
            for(int row = 0; row < dimension; row++){
                long long sum = 0;
                // this way we know exactly how many non-zero elements to process in this row
                for(int idx = row_indeces_serial[row]; idx < row_indeces_serial[row + 1]; idx++){
                    sum += (long long)non_zero_values_serial[idx] * current_vec[column_indeces_serial[idx]];
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
    double end_serial_csr = omp_get_wtime();



    // re-initialize current_vec with the original input vector values
    for(int i = 0; i < dimension; i++){
        current_vec[i] = vector[i];
    }

    double start_parallel_dense = omp_get_wtime();

    // multiply with dense way for comparison
     for(int i = 0; i < iterations; i++){
        #pragma omp parallel for num_threads(threads) schedule(runtime) default(none) shared(dimension, matrix, current_vec, result)
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
    
    printf("Initialization Parallel Time: %f seconds\n", end_init - start_init);
    printf("Initialization Serial Time: %f seconds\n", end_init_serial - start_init_serial);
    printf("Parallel CSR Multiplication Time: %f seconds\n", end_parallel_csr - start_parallel_csr);
    printf("Serial CSR Multiplication Time: %f seconds\n", end_serial_csr - start_serial_csr);
    printf("Parallel Dense Multiplication Time: %f seconds\n", end_parallel_dense - start_parallel_dense);
    
    free(results1);
    free(non_zero_values);
    free(column_indeces);
    free(row_indeces);
    for(int i = 0; i < dimension; i++){
        free(matrix[i]);
    }
    free(matrix);
    free(current_vec);
    free(vector);
    free(result);
    free(non_zero_values_serial);
    free(column_indeces_serial);
    free(row_indeces_serial);
    return 0;
 }


 void check_results(long long* res1, long long* res2, int dimension){
    for(int i = 0; i < dimension; i++){
        if(res1[i] != res2[i]){
            printf("Results do not match at index %d: %lld != %lld\n", i, res1[i], res2[i]);
            exit(1);
        }
    }
    printf("Results match!\n");
}

void check_CSR_initialization(int* row_indeces, int* column_indeces, int* non_zero_values, int* row_indeces_serial, int* column_indeces_serial, int* non_zero_values_serial, int dimension){
    for(int i = 0; i <= dimension; i++){
        if(row_indeces[i] != row_indeces_serial[i]){
            printf("Row indeces do not match at index %d: %d != %d\n", i, row_indeces[i], row_indeces_serial[i]);
            exit(1);
        }
    }
    int nnz = row_indeces[dimension];
    for(int i = 0; i < nnz; i++){
        if(column_indeces[i] != column_indeces_serial[i]){
            printf("Column indeces do not match at index %d: %d != %d\n", i, column_indeces[i], column_indeces_serial[i]);
            exit(1);
        }
        if(non_zero_values[i] != non_zero_values_serial[i]){
            printf("Non-zero values do not match at index %d: %d != %d\n", i, non_zero_values[i], non_zero_values_serial[i]);
            exit(1);
        }
    }
    printf("CSR Initialization match!\n");
}