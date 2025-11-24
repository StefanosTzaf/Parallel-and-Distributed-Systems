#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define perror2(s, e) fprintf(stderr ,"%s: %s\n", s, strerror(e))

// holds only the number of non-zero elements of each array
struct array_stats_s {
 long long int info_array_0;
 long long int info_array_1;
 long long int info_array_2;
 long long int info_array_3;
} array_stats;

 // struct to hold arguments for each thread
struct threadArgs{
    int* array;
    int size;
    int arrayIndex; // so we know which value of the above struct to update
};

// forward declarations
void serialAlgorithm(int*, int, int);
void* threadFunc(void*);
void printStats();

int main(int argc, char* argv[]){
    
    if(argc != 2){
        printf("Usage: %s size_of_array\n", argv[0]);
        exit(1);
    }

    int sizeOfArray = atoi(argv[1]);

    // time needed to create and initialize arrays and structs
    struct timespec arrayInitStart, arrayInitEnd;
    clock_gettime(CLOCK_REALTIME, &arrayInitStart);

    int* arrays[4]; 
    for(int i = 0; i < 4; i++){
        arrays[i] = malloc(sizeOfArray * sizeof(int)); // memory for each array
    }

    srand(time(NULL)); 
    for(int i = 0; i < sizeOfArray; i++){
        arrays[0][i] = rand() % 10;
        arrays[1][i] = rand() % 10;
        arrays[2][i] = rand() % 10;
        arrays[3][i] = rand() % 10;
    }
  
    // arguments for each thread
    struct threadArgs* args[4];
    for(int i = 0; i < 4; i++){
        args[i] = malloc(sizeof(struct threadArgs));
        args[i]->array = arrays[i];
        args[i]->size = sizeOfArray;
        args[i]->arrayIndex = i;
    }

    clock_gettime(CLOCK_REALTIME, &arrayInitEnd);
    double arrayInitTime = (arrayInitEnd.tv_sec - arrayInitStart.tv_sec) + 
        (arrayInitEnd.tv_nsec - arrayInitStart.tv_nsec) / 1e9;
    printf("Array Initialization Time: %f seconds\n\n", arrayInitTime);



    //######## serial execution ########//
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    
    for(int i = 0; i < 4; i++){ // call serial algorithm on each array
        serialAlgorithm(arrays[i], sizeOfArray, i);
    }

    clock_gettime(CLOCK_REALTIME, &end);
    double serialTime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Serial Execution Time: %f seconds\n\n", serialTime);
    
    
    // store stats for serial execution in temporary variables
    long long int temp_array_0 = array_stats.info_array_0;
    long long int temp_array_1 =  array_stats.info_array_1;;
    long long int temp_array_2 =  array_stats.info_array_2;;
    long long int temp_array_3 =  array_stats.info_array_3;
    
    // reset stats
    array_stats.info_array_0 = 0;
    array_stats.info_array_1 = 0;
    array_stats.info_array_2 = 0;
    array_stats.info_array_3 = 0;
   
   
    // ##### parallel execution ######//

    int numOfThreads = 4;
    pthread_t* threads = malloc(numOfThreads * sizeof(pthread_t));

    clock_gettime(CLOCK_REALTIME, &start);


    int err, status;
    for(int i = 0; i < 4; i++){
        if((err = pthread_create(&threads[i], NULL, threadFunc, (void*)args[i]))){
            perror2("pthread_create", err);
            exit(1);
        }
    }

    for(int i = 0; i < numOfThreads; i++){
        if((err = pthread_join(threads[i], (void**)&status))){
            perror2("pthread_join", err);
            exit(1);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);
    double parallelTime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Parallel Execution Time: %f seconds\n\n", parallelTime);

    // verify that serial and parallel results are the same
    if(temp_array_0 == array_stats.info_array_0 &&
       temp_array_1 == array_stats.info_array_1 &&
       temp_array_2 == array_stats.info_array_2 &&
       temp_array_3 == array_stats.info_array_3){
        
        printf("Verification Successful: Serial and Parallel results match!\n\n");
    } 
    else{
        printf("Verification Failed: Serial and Parallel results do not match!\n\n");
    }
    
    // free allocated memory
    for(int i = 0; i < 4; i++){
        free(args[i]);
        free(arrays[i]);
    }
    
    free(threads);

    exit(0);

}

void* threadFunc(void* arg){

    struct threadArgs* currArgs = (struct threadArgs*)arg;
    int* array = currArgs->array;
    int size = currArgs->size;
    int idx = currArgs->arrayIndex;

    for(int i = 0; i < size; i++){
        
        if(array[i] != 0){
            
            // update the correct field in the struct
            switch(idx){
                case 0:
                    array_stats.info_array_0++;
                    break;
                case 1:
                    array_stats.info_array_1++;
                    break;
                case 2:
                    array_stats.info_array_2++;
                    break;
                case 3:
                    array_stats.info_array_3++;
                    break;
                default:
                    break;
            }
        }
    }

    return NULL;

}


void serialAlgorithm(int* array, int size, int arrayIndex){
    
    for(int i = 0; i < size; i++){
        
        if(array[i] != 0){
            
            // update the correct field in the struct
            switch(arrayIndex){
                case 0:
                    array_stats.info_array_0++;
                    break;
                case 1:
                    array_stats.info_array_1++;
                    break;
                case 2:
                    array_stats.info_array_2++;
                    break;
                case 3:
                    array_stats.info_array_3++;
                    break;
                default:
                    break;
            }
        }
    }
    
}

void printStats(){
    printf("Array 0 non-zero elements: %lld\n", array_stats.info_array_0);
    printf("Array 1 non-zero elements: %lld\n", array_stats.info_array_1);
    printf("Array 2 non-zero elements: %lld\n", array_stats.info_array_2);
    printf("Array 3 non-zero elements: %lld\n\n", array_stats.info_array_3);
}
