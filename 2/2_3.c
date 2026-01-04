#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#define TASK_LIMIT 10000


void merge(int [], int [], size_t, int[], size_t);
void divideArray(int [], size_t);
void divideArrayPar(int [], size_t);
bool isSorted(int [], size_t);


int main(int argc, char* argv[]){

    if(argc != 4){
        printf("Usage: %s <size of array> <serial or parallel> <number of threads>\n", argv[0]);
        exit(1);
    }

    int arraySize = atoi(argv[1]);
    int threadsNum = atoi(argv[3]);

    srand(1); // seed is a constant so the array is deterministic

    int* array = malloc(sizeof(int) * arraySize);
    for(int i = 0; i < arraySize; i++){
        array[i] = rand() % 100;
    }

    double start, end;
    start = omp_get_wtime();

    printf("--------------------\n");

    if(strcmp(argv[2], "serial") == 0){
        printf("serial execution\n");
        printf("array size: %d\n", arraySize);
        divideArray(array, arraySize);
    }
    else{
        printf("parallel execution\n");
        printf("number of threads: %d \n", threadsNum);
        printf("array size: %d\n", arraySize);

        // max threadsNum threads can execute tasks 
        #pragma omp parallel num_threads(threadsNum)
        {
            // only one thread should start the recursion
            // and create the initial tasks
            #pragma omp single
            {
                divideArrayPar(array, arraySize);
            }
        }        
    }
    end = omp_get_wtime();

    double totalTime = end - start;

    printf("Time: %.5f\n", totalTime);
    printf("----------------------------\n");


    // for(int i = 0; i < arraySize; i++){
    //     printf("A[%d] = %d\n", i, array[i]);
    // }

    isSorted(array, arraySize);

    free(array);


}

bool isSorted(int arr[], size_t size){

    for(size_t i = 1; i < size; i++){
        if(arr[i-1] > arr[i]){
            printf("array is not sorted!\n");
            return false;
        }
    }
    printf("array is sorted!\n");
    return true;
}


void divideArrayPar(int arr[], size_t size){

    if(size <= 1){
        return;
    }

    int divPoint = size / 2; // point of division of array

    int* leftArray = malloc(sizeof(int)*divPoint);
    int* rightArray = malloc(sizeof(int)*(size - divPoint));

    for(int i = 0; i < divPoint; i++){
        leftArray[i] = arr[i];
    }

    int idx = 0;
    for(int i = divPoint; i < size; i++){
        rightArray[idx] = arr[i];
        idx++;
    }

    #pragma omp task shared(leftArray) shared(divPoint) if (divPoint > TASK_LIMIT)
    divideArrayPar(leftArray, divPoint);
    
    #pragma omp task shared(rightArray) shared(divPoint) if ((size - divPoint) > TASK_LIMIT)
    divideArrayPar(rightArray, size - divPoint);

    // wait for all tasks to finish to execute merge
    // otherwise arrays may not be ready
    #pragma omp taskwait
    merge(arr, leftArray, divPoint, rightArray, size - divPoint);

    free(leftArray);
    free(rightArray);

}

void merge(int result[], int array1[], size_t size1, int array2[], size_t size2){

    int k = 0, leftIdx = 0, rightIdx = 0;
    
    while (leftIdx < size1 && rightIdx < size2) {
        
        if (array1[leftIdx] <= array2[rightIdx]){
            result[k] = array1[leftIdx];
            leftIdx++;
        }
        else{
            result[k] = array2[rightIdx];
            rightIdx++;
        }
        k++;
    }

    // elements of left array have already been moved
    if(leftIdx == size1){
        // copy remaining elements of array2
        while (rightIdx < size2){
            result[k] = array2[rightIdx];
            k++;
            rightIdx++;
        }
    }

    // elements of right array have already been moved
    else{
        // copy remaining elements of array1
        while (leftIdx < size1){
            result[k] = array1[leftIdx];
            k++;
            leftIdx++;
        }
    }

}


void divideArray(int arr[], size_t size){

    if(size <= 1){
        return;
    }

    int divPoint = size / 2; // point of division of array

    int* leftArray = malloc(sizeof(int)*divPoint);
    int* rightArray = malloc(sizeof(int)*(size - divPoint));

    for(int i = 0; i < divPoint; i++){
        leftArray[i] = arr[i];
    }

    int idx = 0;
    for(int i = divPoint; i < size; i++){
        rightArray[idx] = arr[i];
        idx++;
    }

    divideArray(leftArray, divPoint);
    divideArray(rightArray, size - divPoint);

    merge(arr, leftArray, divPoint, rightArray, size - divPoint);

    free(leftArray);
    free(rightArray);

}







