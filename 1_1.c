#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>



int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <Degree of the Polynomials> <Threads number>\n", argv[0]);
        return 1;
    }
    int degree = atoi(argv[1]);
    int threads_num = atoi(argv[2]);

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

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned int seed = ts.tv_sec * 1000000000 + ts.tv_nsec; // nanoseconds

    for(int i = 0; i <= degree; i++){
        //rand_r is thread-safe version of rand
        int value = (rand_r(&seed) % 100) + 1; // 1-100
        bool sign = rand_r(&seed) % 2;
        poly1[i] = sign ? -value : value;
    }
    for(int i = 0; i <= degree; i++){
        int value = (rand_r(&seed) % 100) + 1; // 1-100
        bool sign = rand_r(&seed) % 2;
        poly2[i] = sign ? -value : value;
    }


    int* serial_result = (int*)calloc(degree*2 + 1, sizeof(int));
    if(serial_result == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    // Serial Polynomial Multiplication 
    //for example (4 + 2x)*(3 + 5x) will be calculated as:
    // result[0] += 4 * 3 = 12
    // result[1] += 4 * 5 = 20
    // result[1] += 2 * 3 = 6  (added to previous 20)
    // result[2] += 2 * 5 = 10
    // so it is no needed to make the addition separately 
    for(int i = 0; i <= degree; i++){
        for(int j = 0; j <= degree; j++){
            serial_result[i + j] += poly1[i] *  poly2[j];
        }
    }

    printf("Polynomial 1:\n");
    for(int i = 0; i <= degree; i++){
        printf("%d * x^%d\n", poly1[i], i);
    }
    printf("Polynomial 2:\n");
    for(int i = 0; i <= degree; i++){
        printf("%d * x^%d\n", poly2[i], i);
    }
    printf("Result Polynomial:\n\n");
    for(int i = 0; i <= degree*2; i++){
        printf("%d * x^%d\n", serial_result[i], i);
    }

    free(poly1);
    free(poly2);
    free(serial_result);


    return 0;
}