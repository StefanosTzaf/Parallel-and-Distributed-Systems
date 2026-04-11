#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// Function to verify results
bool verify_results(__int128_t* serial, __int128_t* parallel, int size){
    for(int i = 0; i < size; i++){
        if(serial[i] != parallel[i]){
            printf("Mismatch at coefficient %d:\n", i);
            return false;
        }
    }
    return true;
}


int main(int argc, char *argv[]){

    MPI_Init(NULL, NULL);
    int my_rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    
    bool run_serial = true;
    if (my_rank == 0){
        if (argc != 2 && argc != 3){
            fprintf(stderr, "Usage: %s <degree_of_polynomials>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        if(argc == 3 && (strcmp(argv[2], "No") == 0 || strcmp(argv[2], "no") == 0 || strcmp(argv[2], "NO") == 0)){
            run_serial = false;
            fprintf(stderr, "Serial algorithm will not be run for verification.\n");
        }
        else if(argc == 3){
            fprintf(stderr, "Invalid second argument. Use 'No' to skip serial algorithm.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
    }

    int degree = atoi(argv[1]);

    int* poly1 = NULL;
    int* poly2 = NULL;

    if (my_rank == 0) {
        poly1 = (int*)malloc((degree + 1) * sizeof(int));
        if(poly1 == NULL){
            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        poly2 = (int*)malloc((degree + 1) * sizeof(int));
        if(poly2 == NULL){
            fprintf(stderr, "Memory allocation failed\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
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
    }

    int result_size = degree * 2 + 1;
    int coeffs_per_process = result_size / nprocs;
    int rem = result_size % nprocs;

    // check if this process will take 1 more coefficient
    int my_coeffs = coeffs_per_process + (my_rank < rem ? 1 : 0);
    // if we are in a process that takes extra coefficient, we need to adjust the start index
    int start_coeff = my_rank * coeffs_per_process + (my_rank < rem ? my_rank : rem);
    int end_coeff = start_coeff + my_coeffs - 1;

    // each rank only needs a part of coefficients from both polynomials.
    // for every coefficient k in [start_coeff, end_coeff] that a process takes, indices that must be sent are:
    // i in [max(0, k - degree), min(degree, k)] and (k-i) in the same range. (remember x^7 can be made by x^3 * x^4 and x^4 * x^3)
    int part_start = (start_coeff - degree) > 0 ? (start_coeff - degree) : 0;
    int part_end = (end_coeff < degree) ? end_coeff : degree;
    int part_len = part_end - part_start + 1;

    int* poly1_local = (int*)malloc((size_t)part_len * sizeof(int));
    int* poly2_local = (int*)malloc((size_t)part_len * sizeof(int));
    if (poly1_local == NULL || poly2_local == NULL) {
        fprintf(stderr, "Rank %d: Memory allocation failed for local polynomial parts\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }


    __int128_t *local_res = (__int128_t*)calloc((size_t)my_coeffs, sizeof(__int128_t));
    if (!local_res) {
        fprintf(stderr, "Rank %d: Memory allocation failed for local_res\n", my_rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    __int128_t *parallel_result = NULL;
    __int128_t *serial_result = NULL;

    if (my_rank == 0){
        parallel_result = (__int128_t*)calloc((size_t)result_size, sizeof(__int128_t));
        serial_result   = (__int128_t*)calloc((size_t)result_size, sizeof(__int128_t));
        if (parallel_result == NULL || serial_result == NULL){
            fprintf(stderr, "Root: Memory allocation failed for results\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    //--------------Timings (without init)--------------

    MPI_Barrier(MPI_COMM_WORLD);
    double total_start = MPI_Wtime();
    double send_start = MPI_Wtime();

    // root sends only the needed parts to each rank.
    if (my_rank == 0){
        // root's own part
        for (int i = 0; i < part_len; i++){
            poly1_local[i] = poly1[part_start + i];
            poly2_local[i] = poly2[part_start + i];
        }

        // send parts to other processes
        for (int r = 1; r < nprocs; r++){
            int r_coeffs = coeffs_per_process + (r < rem ? 1 : 0);
            int r_start_coeff = r * coeffs_per_process + (r < rem ? r : rem);
            int r_end_coeff = r_start_coeff + r_coeffs - 1;

            int r_part_start = (r_start_coeff - degree) > 0 ? (r_start_coeff - degree) : 0;
            int r_part_end = (r_end_coeff < degree) ? r_end_coeff : degree;
            int r_part_len = r_part_end - r_part_start + 1;

            // send with tags 100 and 101 to distinguish between the two polynomials
            MPI_Send(poly1 + r_part_start, r_part_len, MPI_INT, r, 100, MPI_COMM_WORLD);
            MPI_Send(poly2 + r_part_start, r_part_len, MPI_INT, r, 101, MPI_COMM_WORLD);
        }
    }
    else{
        MPI_Recv(poly1_local, part_len, MPI_INT, 0, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(poly2_local, part_len, MPI_INT, 0, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    double send_end = MPI_Wtime();
    double send_time_local = send_end - send_start;

    MPI_Barrier(MPI_COMM_WORLD);
    double parallel_start = MPI_Wtime();
    // Parallel Polynomial Multiplication (same as pthread logic but here the first loop starts always
    // from 0 because it saves results in local buffer)
    for(int k = 0; k < my_coeffs; k++){
        int global_k = start_coeff + k;
        __int128_t sum = 0;
        
        int i_start = (global_k - degree) > 0 ? (global_k - degree) : 0;
        int i_end = (global_k < degree) ? global_k : degree;
        
        for (int i = i_start; i <= i_end; i++){
            sum += (__int128_t)poly1_local[i - part_start] * poly2_local[(global_k - i) - part_start];
        }

        local_res[k] = sum;
    }
    double parallel_end = MPI_Wtime();
    // parallel time for this process (remember this will run concurrently with others)
    double parallel_time_local = parallel_end - parallel_start;
    
    MPI_Barrier(MPI_COMM_WORLD);
    double gather_start = MPI_Wtime();

    int bytes_to_send = (int)(my_coeffs * sizeof(__int128_t));

    // saves how many bytes each process will send to root
    int* recv_counts = NULL;
    // saves the displacements in the final array where each process data will go
    int* displs = NULL;

    if (my_rank == 0){

        recv_counts = (int*)malloc(nprocs * sizeof(int));
        displs = (int*)malloc(nprocs * sizeof(int));
        if (recv_counts == NULL || displs == NULL) {
            fprintf(stderr, "Root: Memory allocation failed for recv_counts or displs\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        // prepare recv_counts and displs arrays before gathering
        for(int i = 0; i < nprocs; i++){
            int proc_coeffs = coeffs_per_process + (i < rem ? 1 : 0);
            int start_idx = i * coeffs_per_process + (i < rem ? i : rem);
            recv_counts[i] = proc_coeffs * sizeof(__int128_t);
            displs[i] = start_idx * sizeof(__int128_t);
        }
    }

    MPI_Gatherv(local_res, bytes_to_send, MPI_BYTE,
                parallel_result, recv_counts, displs, MPI_BYTE,
                0, MPI_COMM_WORLD);

    double gather_end = MPI_Wtime();
    double total_end = MPI_Wtime();
    double gather_time_local = gather_end - gather_start;
    double total_time_local = total_end - total_start;

    // reduction to find max times among all processes
    double max_send_time, max_parallel_time, max_gather_time, max_total_time;
    MPI_Reduce(&send_time_local, &max_send_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&parallel_time_local, &max_parallel_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&gather_time_local, &max_gather_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&total_time_local, &max_total_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_rank == 0){
        // Serial Polynomial Multiplication for verification
        double serial_start = MPI_Wtime();
        if(run_serial == true){
            for(int k = 0; k < result_size; k++) {
                __int128_t sum = 0;
                
                int i_start = (k - degree) > 0 ? (k - degree) : 0;
                int i_end = (k < degree) ? k : degree;
                
                for (int i = i_start; i <= i_end; i++) {
                    sum += (__int128_t)poly1[i] * poly2[k - i];
                }
                serial_result[k] = sum;
            }
        }
        double serial_end = MPI_Wtime();
        double serial_time = serial_end - serial_start;

        if (run_serial == true){
            if (verify_results(serial_result, parallel_result, result_size)){
                printf("Results match!\n");
            } 
            else{
                printf("Results do not match!\n");
            }
        }

        // Print timings
        if(run_serial == true){
            printf("Serial Computation Time: %f seconds\n\n", serial_time);
            // Derived metrics
            double speedup = serial_time > 0.0 ? (serial_time / max_total_time) : 0.0;
            double efficiency = speedup / (double)nprocs;
            printf("Speedup: %f\n", speedup);
            printf("Efficiency: %f\n\n", efficiency);
        }
            
        printf("Max Send Time: %f seconds\n", max_send_time);
        printf("Max Gather Time: %f seconds\n\n", max_gather_time);
        printf("Max Parallel Computation Time: %f seconds\n\n", max_parallel_time);
        printf("Max Total Time: %f seconds\n", max_total_time);

        free(recv_counts);
        free(displs);
        free(parallel_result);
        free(serial_result);
        free(poly1);
        free(poly2);
    }
    free(local_res);
    free(poly1_local);
    free(poly2_local);
    MPI_Finalize();
    return 0;
}