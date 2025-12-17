#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


void merge(int [], int [], size_t, int[], size_t);
void divideArray(int [], size_t);
void mergePar(int [], int [], size_t, int[], size_t);
void divideArrayPar(int [], size_t);


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

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    if(strcmp(argv[2], "serial") == 0){
        divideArray(array, arraySize);
        clock_gettime(CLOCK_REALTIME, &end);
    }
    else{
 

    }

    double totalTime = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Time: %f\n", totalTime);

    free(array);


}

void merge(int result[], int array1[], size_t size1, int array2[], size_t size2){

    size_t size = size1 + size2;
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


    if(size == 1){
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

void mergePar(int result[], int array1[], size_t size1, int array2[], size_t size2){

    size_t size = size1 + size2;
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


void divideArrayPar(int arr[], size_t size){

    if(size == 1){
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



