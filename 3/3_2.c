#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


// Verify that serial and parallel results match
bool verify_results(long long* serial_result, long long* parallel_result, int dimension){
    for(int i = 0; i < dimension; i++){
        if(serial_result[i] != parallel_result[i]){
            printf("Mismatch at index %d: serial=%lld, parallel=%lld\n", 
                   i, serial_result[i], parallel_result[i]);
            return false;
        }
    }
    return true;
}

// Pack zeros at the beginning of the matrix
void non_uniform_distribution(int**, long long*, int, float, unsigned int*);


//Initialize matrix and vector with random values and zeros
void uniform_distribution(int**, long long*, int, float, unsigned int*);

void serial_CSR_multiplication(int**, long long*, long long*, long long*, int, int, int*, int*, int*);


int main(int argc, char* argv[]){

    MPI_Init(NULL, NULL);
    int my_rank, nprocs;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if(argc != 4 && argc != 5){
        if(my_rank == 0){
            printf("Usage: %s <number of rows> <percentage of 0 elements> <iterations> [Y for non-uniform distribution]\n", argv[0]);
        }
        MPI_Finalize();
        return -1;
    }

    int dimension = atoi(argv[1]);
    float zero_percentage = atof(argv[2]);
    int iterations = atoi(argv[3]);

    if(my_rank == 0){

        if(zero_percentage < 0 || zero_percentage > 100){
            printf("Percentage of 0 elements must be between 0 and 100.\n");
            return 1;
        }
    }

    //##### MEMORY ALLOCATIONS #####//
    // MATRIX NxN ALLOCATION
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

    // all processes should allocate memory for the row_indeces_serial
    // array because it will be used in broadcast
    // its initialization is executed only by process 0
    int* row_indeces_serial = (int*)malloc((dimension + 1) * sizeof(int));
    if(row_indeces_serial == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }

    // only process 0 needs memory for these, but they still
    // should be NULL for the rest of the processes for
    // the scatter and broadcast calls
    int* non_zero_values_serial = NULL;
    int* column_indeces_serial = NULL;

    // VECTOR Nx1 declare as long long to be able to easily assign it to results without copying 
    // all processes need memory for this vector
    long long* vector = (long long*)malloc(dimension * sizeof(long long));
    if(vector == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }

    // we have to copy the initial vector to restore it later for dense multiplication
    // is not counted in the time measurements because we only do it to be able to run both multiplications
    long long* current_vec = (long long*)malloc(dimension * sizeof(long long));
    long long* result = (long long*)malloc(dimension * sizeof(long long));
    if((current_vec == NULL) || (result == NULL)){
        printf("Memory allocation failed\n");
        return 1;
    }

    double init_time = 0.0;
    double serial_time_mult = 0.0;

    if(my_rank == 0){

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned int seed = ts.tv_nsec ^ ts.tv_sec;


        //##### INITIALIZATION OF MATRIX #####//

        // if non uniform distribution is selected pack all zeros at the beginning of the matrix
        if(argc == 5){
           
            if(argv[4][0] == 'y' || argv[4][0] == 'Y'){
            
                non_uniform_distribution(matrix, vector, dimension, zero_percentage, &seed);
            }
            else{
                printf("Invalid argument for non-uniform distribution. Use 'Y' for yes. Else use no argument.\n");
                return 1;
            }
        }

        //Initialize matrix and vector with random values and zeros
        else{
            uniform_distribution(matrix, vector, dimension, zero_percentage, &seed);
        }

        //##### INITIALIZATION OF CSR MATRICES #####//
        double start_init = MPI_Wtime();
        
        // count of non-zero elements in each row
        int *row_nnz_serial = malloc(dimension * sizeof(int));
        if(row_nnz_serial == NULL){
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
        
        // equal to the number of non zero elements before the current row
        for (int i = 0; i < dimension; i++) {
            row_indeces_serial[i + 1] = row_indeces_serial[i] + row_nnz_serial[i];
        }
        free(row_nnz_serial);

        // 1st and 2nd CSR arrays
        non_zero_values_serial = (int*)malloc(row_indeces_serial[dimension] * sizeof(int));
        column_indeces_serial = (int*)malloc(row_indeces_serial[dimension] * sizeof(int));
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

        double end_init = MPI_Wtime();
        init_time = end_init - start_init;
        
        for(int i = 0; i < dimension; i++){
            current_vec[i] = vector[i];
        }
        //##### END OF INITIALIZATION OF CSR MATRICES#####//


      
        //##### MULTIPLICATION OF MATRIX WITH VECTOR #####//
        // ################### SERIAL ####################//

        double start_serial_mult = MPI_Wtime();

        serial_CSR_multiplication(matrix, vector, current_vec, result, dimension, iterations, 
            row_indeces_serial, non_zero_values_serial, column_indeces_serial);
        
        double end_serial_mult = MPI_Wtime();
        serial_time_mult = end_serial_mult - start_serial_mult;
    }
    

    //##### PREPARATION OF LOCAL DATA FOR PARALLEL EXECUTION #####//

    MPI_Barrier(MPI_COMM_WORLD);

    double send_start = MPI_Wtime();

    // BROADCAST row_indices_serial so processes can compute
    // their local number of non zero values
    MPI_Bcast(row_indeces_serial, dimension + 1, MPI_INT, 0, MPI_COMM_WORLD);

    double send_end = MPI_Wtime();
    double send_time = send_end - send_start;

    size_t rows_per_process = dimension / nprocs;
    size_t remaining = dimension % nprocs; // remaining rows to distribute

    size_t local_rows = rows_per_process; 

    // if they are remaining rows, give an 
    // extra row to some processes
    if(remaining != 0){
        if(my_rank < remaining){
            local_rows += 1;
        }
    }

    // compute the start and end row for each process
    size_t row_start = 0;
    for(size_t curr_rank = 0; curr_rank < my_rank; curr_rank++){
        
        row_start += rows_per_process;

        if(curr_rank < remaining){
            row_start += 1;
        }
    }
    size_t row_end = row_start + local_rows;

    // compute the corresponding non_zero_values_start and end
    size_t non_zero_values_start = row_indeces_serial[row_start];
    size_t non_zero_values_end = row_indeces_serial[row_end];
    
    // how many non zero values each process needs
    size_t local_non_zero_values_count = non_zero_values_end - non_zero_values_start;

    // allocate memory for the local values and columns
    int* local_values = malloc(sizeof(int) * local_non_zero_values_count);
    int* local_columns = malloc(sizeof(int) * local_non_zero_values_count);
    if((local_values == NULL) || (local_columns == NULL)){
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // only process 0 can pass these as arguments
    int* local_nzv_count_root = NULL;
    int* offsets = NULL;

    // Now that we now how many values each process should read
    // process 0 should scatter only those values 
    // but it also needs to calculate the local values of each process
    // so it can scatter properly the array with the non zero values
   if(my_rank == 0){
      
        int* row_starts_array = malloc(sizeof(int) * nprocs); // holds the start row each process should read from
        int* row_ends_array = malloc(sizeof(int) * nprocs); // holds the end row till which each process should read 
        int* local_rows_array = malloc(sizeof(int) * nprocs); // holds the number of rows each process should read
        local_nzv_count_root = malloc(sizeof(int) * nprocs); // holds the number of non zero values each process should read
        if((row_starts_array == NULL) || (row_ends_array == NULL) || 
                (local_rows_array == NULL) || (local_nzv_count_root == NULL)){
            printf("Memory allocation failed\n");
            return 1;
        }

        // calculate number of local rows for each process
        for(int rank = 0; rank < nprocs; rank++){
        
            size_t rows = rows_per_process;

            // if they are remaining rows, give an 
            // extra row to some processes
            if(remaining != 0){

                if(rank < remaining){
                    rows += 1;
                }
            }
            local_rows_array[rank] = rows;
        }

        // calculate start and end row for each process
        size_t row_start = 0;
        for(int rank = 0; rank < nprocs; rank++){
            
            row_starts_array[rank] = row_start;

            row_start += local_rows_array[rank];

            row_ends_array[rank] = row_start;
        }

        offsets = malloc(sizeof(int) * nprocs);
        if(offsets == NULL){
            printf("Memory allocation failed\n");
            return 1;
        }
        
        // number of non zero values for each process
        for(int rank = 0; rank < nprocs; rank++){
            local_nzv_count_root[rank] = row_indeces_serial[row_ends_array[rank]] - row_indeces_serial[row_starts_array [rank]];
        }
        
        // where each count begins in the non_zero_values_serial
        offsets[0] = 0;
        for(int rank = 1; rank < nprocs; rank++){
            offsets[rank] = offsets[rank-1] + local_nzv_count_root[rank-1];
        }

        free(row_starts_array);
        free(row_ends_array);
        free(local_rows_array);

    }

    MPI_Barrier(MPI_COMM_WORLD);

    send_start = MPI_Wtime();

    // scatter the non zero values
    MPI_Scatterv(non_zero_values_serial, local_nzv_count_root, offsets, MPI_INT, local_values, 
        local_non_zero_values_count, MPI_INT, 0, MPI_COMM_WORLD); 
        
    // scatter the column indeces        
    MPI_Scatterv(column_indeces_serial, local_nzv_count_root, offsets, MPI_INT, local_columns, 
            local_non_zero_values_count, MPI_INT, 0 , MPI_COMM_WORLD);
            
    // Broadcast the vector, since every process needs it
    MPI_Bcast(vector, dimension, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    
    // Broadcast current_vec (initialized by process 0) to all processes
    MPI_Bcast(current_vec, dimension, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    // Broadcast initialization time by process 0 to all processes
    MPI_Bcast(&init_time, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Broadcast serial multiplication time by process 0 to all processes
    MPI_Bcast(&serial_time_mult, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Broadcast matrix rows to all processes for dense multiplication
    for(int i = 0; i < dimension; i++){
        MPI_Bcast(matrix[i], dimension, MPI_INT, 0, MPI_COMM_WORLD);
    }
    
    send_end = MPI_Wtime();
    send_time += send_end - send_start;

    //##### MULTIPLICATION OF MATRIX WITH VECTOR #####//
    //################### PARALLEL ##################//
    
    // Prepare recv_counts and displs for Allgatherv (needed by all processes)
    int* recv_counts = malloc(sizeof(int) * nprocs);
    int* displs = malloc(sizeof(int) * nprocs);
    if(recv_counts == NULL || displs == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // Calculate receive counts and displacements
    size_t disp = 0;
    for(int rank = 0; rank < nprocs; rank++){
        size_t rows = rows_per_process;
        if(rank < remaining){
            rows += 1;
        }
        recv_counts[rank] = rows;
        displs[rank] = disp;
        disp += rows;
    }
    
    
    long long* local_result = malloc(sizeof(long long) * local_rows);
    if(local_result == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }
    
    long long* temp_result = malloc(sizeof(long long) * dimension);
    if(temp_result == NULL){
        printf("Memory allocation failed\n");
        return 1;
    }
    
    int local_base = row_indeces_serial[row_start];
    
    MPI_Barrier(MPI_COMM_WORLD);
    double parallel_start_local = MPI_Wtime();

    for(int iter = 0; iter < iterations; iter++){
        // Each process computes its portion of rows
        for(int row = row_start; row < row_end; row++){

            long long sum = 0;

            // global begin and end refer to non_zero_values_serial
            int global_begin = row_indeces_serial[row];
            int global_end   = row_indeces_serial[row + 1];

            // local begin and end refer to local_values
            int local_begin = global_begin - local_base;
            int local_end   = global_end   - local_base;

            for(int idx = local_begin; idx < local_end; idx++){
                sum += (long long)local_values[idx] * current_vec[local_columns[idx]];
            }

            local_result[row - row_start] = sum;   //local result index
        }
        
        // Gather all partial results into temp_result
        MPI_Allgatherv(local_result, local_rows, MPI_LONG_LONG, temp_result, 
            recv_counts, displs, MPI_LONG_LONG, MPI_COMM_WORLD);
        
        // Swap pointers for next iteration
        long long* swap = current_vec;
        current_vec = temp_result;
        temp_result = swap;
    }

    double parallel_end_local = MPI_Wtime();
    double parallel_time_local = parallel_end_local - parallel_start_local;
    

    // reset current_vec to original vector for dense multiplication
    for(int i = 0; i < dimension; i++){
        current_vec[i] = vector[i];
    }
    
    //##### DENSE MULTIPLICATION #####//
    
    MPI_Barrier(MPI_COMM_WORLD);
    double dense_start = MPI_Wtime();
    for(int iter = 0; iter < iterations; iter++){
        // Each process computes its portion of rows
        for(int row = row_start; row < row_end; row++){
            
            long long sum = 0;
            
            for(int col = 0; col < dimension; col++){
                sum += (long long)matrix[row][col] * current_vec[col];
            }
            
            local_result[row - row_start] = sum;   //local result index
        }
        
        // Gather all partial results into temp_result
        MPI_Allgatherv(local_result, local_rows, MPI_LONG_LONG, temp_result, 
            recv_counts, displs, MPI_LONG_LONG, MPI_COMM_WORLD);
            
            // Swap pointers for next iteration
            long long* swap = current_vec;
            current_vec = temp_result;
            temp_result = swap;
        }
        double dense_end = MPI_Wtime();
        double dense_time_local = dense_end - dense_start;
        
        // Reduce to get max times across all processes
        double max_parallel_time, max_send_time, max_dense_time;
        MPI_Reduce(&parallel_time_local, &max_parallel_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&send_time, &max_send_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&dense_time_local, &max_dense_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        
        // Verification and output
        if(my_rank == 0){
            // Verify results match
            bool match = verify_results(result, current_vec, dimension);
            
            if(match){
                printf("Results match!\n");
            }
        else{
            printf("Results do NOT match!\n");
        }
        
        printf("Serial initialization time: %f seconds\n", init_time);
        printf("Serial multiplication time: %f seconds\n", serial_time_mult);
        printf("Max time to send data: %f seconds\n", max_send_time);
        printf("Max parallel multiplication time: %f seconds\n", max_parallel_time);
        printf("Total Parallel Time: %f seconds\n", max_send_time + max_parallel_time + init_time);
        printf("Max dense multiplication time: %f seconds\n", max_dense_time);
    }



    //##### DEALLOCATION OF MEMORY #####//

    
    if (my_rank == 0) {
        free(local_nzv_count_root);
        free(offsets);
        free(non_zero_values_serial);
        free(column_indeces_serial);
        free(result);
    }
    
    // All processes now have matrix allocated
    for(int i = 0; i < dimension; i++){
        free(matrix[i]);
    }
    free(matrix);
    free(row_indeces_serial);
    free(vector);
    free(current_vec);
    free(local_result);
    free(local_values);
    free(local_columns);
    free(recv_counts);
    free(displs);

    MPI_Finalize();
    return 0;
}




// Pack zeros at the beginning of the matrix
void non_uniform_distribution(int** matrix, long long* vector, int dimension, float zero_percentage, unsigned int* seed){
    int zero_elements = (int)((zero_percentage / 100.0) * dimension * dimension);
    int count = 0;
    
    for(int i = 0; i < dimension; i++){
        for(int j = 0; j < dimension; j++){
            if(count < zero_elements){
                matrix[i][j] = 0;
                count++;
            }
            else{
                bool sign = rand_r(seed) % 2;
                int value = (rand_r(seed) % RAND_MAX) + 1;
                matrix[i][j] = sign ? value : -value;
            }
        }
        bool sign = rand_r(seed) % 2;
        int value = (rand_r(seed) % RAND_MAX) + 1;
        vector[i] = sign ? value : -value;
    }

}


//Initialize matrix and vector with random values and zeros
void uniform_distribution(int** matrix, long long* vector, int dimension, float zero_percentage, unsigned int* seed){
    for(int i = 0; i < dimension; i++){
        for(int j = 0; j < dimension; j++){
            bool zero_element = ((rand_r(seed) % 100) < zero_percentage);
            if(zero_element){
                matrix[i][j] = 0;
            } 
            else {
                bool sign = rand_r(seed) % 2;
                // generate random non-zero value
                int value = (rand_r(seed) % RAND_MAX) + 1;
                matrix[i][j] = sign ? value : -value;
            }
        }
        bool sign = rand_r(seed) % 2;
        int value = (rand_r(seed) % RAND_MAX) + 1;
        vector[i] = sign ? value : -value;
    }
}

void serial_CSR_multiplication(int** matrix, long long* vector, long long* current_vec, long long* result, int dimension, int iterations, 
    int* row_indeces_serial, int* non_zero_values_serial, int* column_indeces_serial){
    
    double start_serial_mult = MPI_Wtime();

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

    // Save final serial result to result array (current_vec has final result after swapping)
    for(int i = 0; i < dimension; i++){
        result[i] = current_vec[i];
    }

    // re-initialize current_vec with the original input vector values for parallel computation
    for(int i = 0; i < dimension; i++){
        current_vec[i] = vector[i];
    }

}