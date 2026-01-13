#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>


// Verify that serial and parallel results match
bool verify_results(long long*, long long*, int);

// executes matrix-vector multiplication in CSR format serially
void serial_CSR_multiplication(long long*, long long*, long long*, int, int, int*, int*, int*);


int main(int argc, char* argv[]){

    MPI_Init(NULL, NULL);
    int my_rank, nprocs;
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if(argc != 4){
        if(my_rank == 0){
            fprintf(stderr, "Usage: %s <number of rows> <percentage of 0 elements> <iterations> \n", argv[0]);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int dimension = atoi(argv[1]);
    float zero_percentage = atof(argv[2]);
    int iterations = atoi(argv[3]);

    if(my_rank == 0){

        if(zero_percentage < 0 || zero_percentage > 100){
            fprintf(stderr, "Percentage of 0 elements must be between 0 and 100.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    //##### MEMORY ALLOCATIONS #####//

    // all processes should allocate memory for the row_indeces_serial
    // array because it will be used in broadcast
    // its initialization is executed only by process 0
    int* row_indeces_serial = (int*)malloc((dimension + 1) * sizeof(int));

    // only process 0 needs memory for these, but they still
    // should be NULL for the rest of the processes for
    // the scatter and broadcast calls
    int* matrix = NULL;
    int* non_zero_values_serial = NULL; // 1st CSR array
    int* column_indeces_serial = NULL; // 2nd CSR array
    long long* result = NULL; // result vector for serial multiplication

    // VECTOR Nx1: all processes need memory for it for the multiplication
    long long* vector = (long long*)malloc(dimension * sizeof(long long));
    // we have to copy the initial vector to restore it later for dense multiplication
    long long* current_vec = (long long*)malloc(dimension * sizeof(long long));   
    if((vector == NULL) || (current_vec == NULL) || (row_indeces_serial == NULL)){
        printf("Memory allocation failed 1\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Timing variables 
    double init_time = 0.0;
    double serial_time_mult = 0.0;


    if(my_rank == 0){
        // MATRIX NxN ALLOCATION: allocate contiguously for MPI_Scatterv
        matrix = (int*)malloc(dimension * dimension * sizeof(int));
        if(matrix == NULL){
            printf("Memory allocation failed 2\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
     
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned int seed = ts.tv_nsec ^ ts.tv_sec;

        //##### INITIALIZATION OF MATRIX #####//

        for(int i = 0; i < dimension; i++){
            for(int j = 0; j < dimension; j++){
                bool zero_element = ((rand_r(&seed) % 100) < zero_percentage);
                if(zero_element){
                    matrix[i * dimension + j] = 0;
                } 
                else {
                    bool sign = rand_r(&seed) % 2;
                    // generate random non-zero value
                    int value = (rand_r(&seed) % RAND_MAX) + 1;
                    matrix[i * dimension + j] = sign ? value : -value;
                }
            }
            bool sign = rand_r(&seed) % 2;
            int value = (rand_r(&seed) % RAND_MAX) + 1;
            vector[i] = sign ? value : -value;
        }

        //##### INITIALIZATION OF CSR MATRICES #####//
        double start_init = MPI_Wtime();
                
        // count total non zero values
        size_t totalCount = 0;
        for(int i = 0; i < dimension; i++){

            for(int j = 0; j < dimension; j++){
                
                if (matrix[i * dimension + j] != 0) {
                    totalCount++;
                }
            }
        }
        
        non_zero_values_serial = (int*)malloc(totalCount * sizeof(int)); // CSR array of non zero values
        column_indeces_serial = (int*)malloc(totalCount * sizeof(int)); // CSR array of column indeces of nzv
        if(non_zero_values_serial == NULL || column_indeces_serial == NULL){
            printf("Memory allocation failed 3\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        row_indeces_serial[0] = 0;
        for (int i = 0; i < dimension; i++) {
            
            int count = 0;
            int idx = row_indeces_serial[i];
            
            for (int j = 0; j < dimension; j++) {

                if (matrix[i * dimension + j] != 0) {
                    count++;
                    non_zero_values_serial[idx]  = matrix[i * dimension + j]; // CSR ARRAY 1
                    column_indeces_serial[idx] = j; // CSR ARRAY 2
                    idx++;
                }
            }
            row_indeces_serial[i + 1] = row_indeces_serial[i] + count; // CSR ARRAY 3
            current_vec[i] = vector[i];
        }

        double end_init = MPI_Wtime();
        init_time = end_init - start_init;
        
        //##### END OF INITIALIZATION OF CSR MATRICES#####//


        //##### MULTIPLICATION OF MATRIX WITH VECTOR #####//
        // ################### SERIAL ####################//

        result = (long long*)malloc(dimension * sizeof(long long));
        if(result == NULL){
            printf("Memory allocation failed 4\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        double start_serial_mult = MPI_Wtime();

        serial_CSR_multiplication(vector, current_vec, result, dimension, iterations, 
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

    // compute the start and end row for each process (global row indices)
    size_t row_start = 0;
    for(size_t curr_rank = 0; curr_rank < my_rank; curr_rank++){
        
        row_start += rows_per_process;

        if(curr_rank < remaining){
            row_start += 1;
        }
    }
    size_t row_end = row_start + local_rows;

    // compute the corresponding non_zero_values_start and end (global indices)
    size_t non_zero_values_start = row_indeces_serial[row_start];
    size_t non_zero_values_end = row_indeces_serial[row_end];
    
    // how many non zero values each process needs
    size_t local_non_zero_values_count = non_zero_values_end - non_zero_values_start;

    // allocate memory for the local values
    int* local_values = malloc(sizeof(int) * local_non_zero_values_count);
    int* local_columns = malloc(sizeof(int) * local_non_zero_values_count);

    // local_matrix for dense multiplication needs to be contiguous
    // it is allocated as 1D meaning the rows are stored one after the other [row0 row1 ... local_rows-1]
    int* local_matrix = malloc(local_rows * dimension * sizeof(int));
    if((local_values == NULL) || (local_columns == NULL) || (local_matrix == NULL)){
        printf("Memory allocation failed 5\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // only process 0 can pass these as arguments
    int* local_nzv_counts_array = NULL;
    int* local_rows_counts = NULL;
    int* row_offsets = NULL;
    int* local_elements_counts_array = NULL;
    int* local_element_offsets = NULL;

    // Now that we now how many values each process should read
    // process 0 should scatter only those values 
    // but it also needs to calculate the local values of each process
    // so it can scatter properly the array with the non zero values
   if(my_rank == 0){
      
        local_rows_counts = malloc(sizeof(int) * nprocs); // holds the number of rows each process should read
        local_nzv_counts_array = malloc(sizeof(int) * nprocs); // holds the number of non zero values each process should read
        row_offsets = malloc(sizeof(int) * nprocs); //offsets for scattering the non zero values and column indeces
        local_elements_counts_array = malloc(sizeof(int) * nprocs); // for dense offsets
        local_element_offsets = malloc(sizeof(int) * nprocs);
        
        if((local_rows_counts == NULL) || (local_nzv_counts_array == NULL) || (row_offsets == NULL) ||
            (local_elements_counts_array == NULL) || (local_element_offsets == NULL)){
           
            printf("Memory allocation failed 6\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        // calculation of number of local rows, local start, local end
        size_t row_start = 0;
        row_offsets[0] = 0;
        local_element_offsets[0] = 0;
        for(int rank = 0; rank < nprocs; rank++){
        
            size_t rows = rows_per_process;

            // if they are remaining rows, give an 
            // extra row to some processes
            if(remaining != 0){

                if(rank < remaining){
                    rows += 1;
                }
            }
            local_rows_counts[rank] = rows;
            int row_end = row_start + local_rows_counts[rank];
            
            //number of non zero values for each process
            local_nzv_counts_array[rank] = row_indeces_serial[row_end] - row_indeces_serial[row_start];
            
            row_start += local_rows_counts[rank]; // row start for next rank

            // number of nzv till the one the process should start reading from
            if(rank != 0){
                row_offsets[rank] = row_offsets[rank-1] + local_nzv_counts_array[rank-1];
            }
            
            // number of elements each process should read: dense multiplication
            local_elements_counts_array[rank] = local_rows_counts[rank] * dimension;
            
            // from which element each process should start reading
            local_element_offsets[rank] = local_element_offsets[rank-1] + local_elements_counts_array[rank-1];
        }
    }

    //##### DISTRIBUTION OF DATA TO ALL PROCESSES #####//

    MPI_Barrier(MPI_COMM_WORLD);

    send_start = MPI_Wtime();

    // scatter the non zero values
    MPI_Scatterv(non_zero_values_serial, local_nzv_counts_array, row_offsets, MPI_INT, local_values, 
        local_non_zero_values_count, MPI_INT, 0, MPI_COMM_WORLD); 
        
    // scatter the column indeces        
    MPI_Scatterv(column_indeces_serial, local_nzv_counts_array, row_offsets, MPI_INT, local_columns, 
            local_non_zero_values_count, MPI_INT, 0 , MPI_COMM_WORLD);
            
    // Broadcast the vector, since every process needs it
    MPI_Bcast(vector, dimension, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    
    // Broadcast current_vec (initialized by process 0) to all processes
    MPI_Bcast(current_vec, dimension, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    // Scatter the rows of the matrix to all processes for dense multiplication
    // they might not be exactly equal due to remaining rows so use of scatterv
    // Note: sendbuf only matters at root, but we need valid pointers
    
    MPI_Scatterv(matrix, local_elements_counts_array, local_element_offsets, MPI_INT, 
        local_matrix, local_rows * dimension, MPI_INT, 0, MPI_COMM_WORLD);
    
    send_end = MPI_Wtime();
    send_time += (send_end - send_start);

    //##### MULTIPLICATION OF MATRIX WITH VECTOR #####//
    //################### PARALLEL ##################//
    
    // Prepare recv_counts and displs for Allgatherv (needed by all processes)
    // recv_counts: how many rows each process will send
    // displs: displacements in the final array where each process data will go
    int* recv_counts = malloc(sizeof(int) * nprocs);
    int* displs = malloc(sizeof(int) * nprocs);
    if(recv_counts == NULL || displs == NULL){
        printf("Memory allocation failed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
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
    
    // local result array for each process
    long long* local_result = malloc(sizeof(long long) * local_rows);
    // for Allgather result (full vector on each rank)
    long long* csr_parallel_result = malloc(sizeof(long long) * dimension);
    if((local_result == NULL) || (csr_parallel_result == NULL)){
        printf("Memory allocation failed 7\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Base index in CSR arrays
    int local_base = row_indeces_serial[row_start];

    MPI_Barrier(MPI_COMM_WORLD);
    double parallel_start_local = MPI_Wtime();

    // Save original pointers for restoration after swapping
    long long* original_current_vec = current_vec;
    long long* original_csr_parallel_result = csr_parallel_result;

    for(int iter = 0; iter < iterations; iter++){

        // Compute local rows
        for(int row = row_start; row < row_end; row++){

            long long sum = 0;

            int global_begin = row_indeces_serial[row];
            int global_end   = row_indeces_serial[row + 1];

            int local_begin = global_begin - local_base;
            int local_end   = global_end   - local_base;

            for(int idx = local_begin; idx < local_end; idx++){
                sum += (long long)local_values[idx] * current_vec[local_columns[idx]];
            }

            local_result[row - row_start] = sum;   //local result index
        }
        
        // Gather all partial results into csr_parallel_result
        MPI_Allgatherv(local_result, local_rows, MPI_LONG_LONG, csr_parallel_result, 
            recv_counts, displs, MPI_LONG_LONG, MPI_COMM_WORLD);
        
        // Swap pointers for next iteration
        long long* temp = current_vec;
        current_vec = csr_parallel_result;
        csr_parallel_result = temp;
    }

  
    // If even iterations: current_vec points to original_current_vec buffer
    // If odd iterations: current_vec points to original_csr_parallel_result buffer
    // We want result in original_csr_parallel_result for verification
    if(iterations % 2 == 0) {
        for(int i = 0; i < dimension; i++){
            original_csr_parallel_result[i] = current_vec[i];
        }
    }

    double parallel_end_local = MPI_Wtime();
    double parallel_time_local = parallel_end_local - parallel_start_local;

    // reset original_current_vec to original vector for dense multiplication
    // and restore pointers for proper cleanup
    for(int i = 0; i < dimension; i++){
        original_current_vec[i] = vector[i];
    }
    current_vec = original_current_vec;
    csr_parallel_result = original_csr_parallel_result;
    
    
    
    //##### PREPARATION OF LOCAL DATA FOR DENSE EXECUTION #####//
    long long* csr_dense_result = malloc(sizeof(long long) * dimension);
    if((csr_dense_result == NULL)){
        printf("Memory allocation failed 8\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    //##### DENSE MULTIPLICATION #####//
    MPI_Barrier(MPI_COMM_WORLD);
    double dense_start = MPI_Wtime();
    
    // Save original pointers for restoration after swapping
    long long* original_current_vec_dense = current_vec;
    long long* original_csr_dense_result = csr_dense_result;
    
    for(int iter = 0; iter < iterations; iter++){
        // Each process computes its portion of rows
        for(int row = row_start; row < row_end; row++){
            
            long long sum = 0;
            
            for(int col = 0; col < dimension; col++){
                // access local_matrix as 1D array
                sum += (long long)local_matrix[(row - row_start) * dimension + col] * current_vec[col];
            }
            
            local_result[row - row_start] = sum;   //local result index
        }
        
        // Gather all partial results into csr_dense_result
        MPI_Allgatherv(local_result, local_rows, MPI_LONG_LONG, csr_dense_result, 
            recv_counts, displs, MPI_LONG_LONG, MPI_COMM_WORLD);
            
        // Swap pointers for next iteration
        long long* swap = current_vec;
        current_vec = csr_dense_result;
        csr_dense_result = swap;
    }
    double dense_end = MPI_Wtime();
    double dense_time_local = dense_end - dense_start;
    
    // After iterations swaps: if even, result is in current_vec (original current_vec buffer)
    // if odd, result is in current_vec (original csr_dense_result buffer)
    // We want result in original_csr_dense_result for verification
    if(iterations % 2 == 0) {
        for (size_t i = 0; i < dimension; i++){
            original_csr_dense_result[i] = current_vec[i];
        }
        
    }
    
    // Restore pointers for proper cleanup
    current_vec = original_current_vec_dense;
    csr_dense_result = original_csr_dense_result;
    
    
    // Reduce to get max times across all processes
    double max_parallel_time, max_send_time, max_dense_time;
    MPI_Reduce(&parallel_time_local, &max_parallel_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&send_time, &max_send_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&dense_time_local, &max_dense_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    // Verification and output
    if(my_rank == 0){
        // Verify results match
        bool match = verify_results(result, csr_parallel_result, dimension);
        bool dense_match = verify_results(result, csr_dense_result, dimension);
        
        if(match && dense_match){
            printf("Results match!\n");
        }
        else{
            if(!match)
                printf("CSR Parallel Results do NOT match!\n");
            if(!dense_match)
                printf("Dense Parallel Results do NOT match!\n");
        }
        
        printf("========================================\n");
        printf("Serial initialization time: %f seconds\n", init_time);
        printf("Serial multiplication time: %f seconds\n", serial_time_mult);
        printf("Max time to send data: %f seconds\n", max_send_time);
        printf("Max parallel multiplication time: %f seconds\n", max_parallel_time);
        printf("Total Parallel Time: %f seconds\n", max_send_time + max_parallel_time + init_time);
        printf("Max dense multiplication time: %f seconds\n", max_dense_time);
        printf("=========================================\n");
       
        //##### DEALLOCATION OF MEMORY #####//
        free(local_nzv_counts_array);
        free(row_offsets);
        free(local_elements_counts_array);
        free(local_element_offsets);
        free(non_zero_values_serial);
        free(column_indeces_serial);
        free(result);
        
        // Free matrix only on rank 0 (where it was allocated)
        // Matrix was allocated as contiguous block, so free matrix[0] which points to the block
        free(matrix);
    }
    
    // Free local matrix on all processes
    free(local_matrix);
    
    free(row_indeces_serial);
    free(vector);
    free(current_vec);
    free(csr_parallel_result);
    free(csr_dense_result);
    free(local_result);
    free(local_values);
    free(local_columns);
    free(recv_counts);
    free(displs);

    MPI_Finalize();
    return 0;
}


void serial_CSR_multiplication(long long* vector, long long* current_vec, long long* result, int dimension, int iterations, 
    int* row_indeces_serial, int* non_zero_values_serial, int* column_indeces_serial){

    // Save original pointers
    long long* original_result = result;
    long long* original_current_vec = current_vec;
    
    for(int i = 0; i < iterations; i++){
        for(int row = 0; row < dimension; row++){
            long long sum = 0;
            // this way we know exactly how many non-zero elements to process in this row
            for(int idx = row_indeces_serial[row]; idx < row_indeces_serial[row + 1]; idx++){
                sum += (long long)non_zero_values_serial[idx] * current_vec[column_indeces_serial[idx]];
            }
            result[row] = sum;
        }
    
        // Swap vectors for next iteration
        long long* temp = current_vec;
        current_vec = result;
        // now current_vec points to the result as new input and result to the old
        // just so as not to write to the same array
        result = temp;   
    }

    // If odd iterations: current_vec points to original_result buffer (no copy needed)
    if(iterations % 2 == 0) {
        // current_vec has the final result, need to copy to original_result
        for(int i = 0; i < dimension; i++){
            original_result[i] = current_vec[i];
        }
    }

    // re-initialize the original current_vec with the original input vector values
    for(int i = 0; i < dimension; i++){
        original_current_vec[i] = vector[i];
    }

}


bool verify_results(long long* serial_result, long long* parallel_result, int dimension){
    for(int i = 0; i < dimension; i++){
    
        if(serial_result[i] != parallel_result[i]){
    
            printf("Mismatch at index %d: serial=%lld, parallel=%lld\n", i, serial_result[i], parallel_result[i]);
            return false;
        }
    }
    return true;
}