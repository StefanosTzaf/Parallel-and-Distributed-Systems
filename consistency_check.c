#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

int x = 0, y = 0, r1 = 0, r2 = 0;
sem_t start1, start2, end;

void* thread1(void* p) {
    while(1) {
        sem_wait(&start1);
        x = 1;
        // αν σχολιαστεί η παρακάτω γραμμή εχουμε θεμα γιατι δεν έχουμε σειριακο consistency
        //αλλα TSO (Total Store Ordering) μοντέλο με χαλαρωση σε W->R
         asm volatile("mfence" ::: "memory");
        r1 = y;
        sem_post(&end);
    }
}

void* thread2(void* p) {
    while(1) {
        sem_wait(&start2);
        y = 1;
        asm volatile("mfence" ::: "memory"); 
        r2 = x;
        sem_post(&end);
    }
}

int main() {
    pthread_t t1, t2;
    sem_init(&start1, 0, 0); sem_init(&start2, 0, 0); sem_init(&end, 0, 0);
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    for (int i = 0; ; i++) {
        x = 0; y = 0; r1 = 0; r2 = 0;
        sem_post(&start1); sem_post(&start2);
        sem_wait(&end); sem_wait(&end);

        if (r1 == 0 && r2 == 0) {
            printf("Error! Iteration %d: r1=%d, r2=%d\n", i, r1, r2);
        }
    }
    return 0;
}