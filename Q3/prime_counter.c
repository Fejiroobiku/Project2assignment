#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#define LIMIT 200000
#define THREADS 16

int total_primes = 0;
pthread_mutex_t mutex;

int is_prime(int n)
{
    int i;
    if (n < 2) return (0);
    if (n == 2) return (1);
    if (n % 2 == 0) return (0);
    
    for (i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return (0);
    }
    return (1);
}

void *count_segment(void *arg)
{
    int start = *((int *)arg);
    int end = start + (LIMIT / THREADS);
    int i, count = 0;
    
    if (start == 1) start = 2;
    if (start + (LIMIT / THREADS) > LIMIT) {
        end = LIMIT + 1;
    }
    
    for (i = start; i < end; i++) {
        if (is_prime(i)) count++;
    }
    
    pthread_mutex_lock(&mutex);
    total_primes += count;
    pthread_mutex_unlock(&mutex);
    
    free(arg);
    return (NULL);
}

int main(void)
{
    pthread_t t[THREADS];
    int i;
    int segment_size;
    
    segment_size = LIMIT / THREADS;
    
    printf("========================================\n");
    printf("Multi-threaded Prime Number Counter\n");
    printf("========================================\n");
    printf("Range: 1 to %d\n", LIMIT);
    printf("Number of threads: %d\n", THREADS);
    printf("========================================\n\n");
    
    pthread_mutex_init(&mutex, NULL);
    
    for (i = 0; i < THREADS; i++) {
        int *start = (int *)malloc(sizeof(int));
        *start = i * segment_size + 1;
        pthread_create(&t[i], NULL, count_segment, start);
    }
    
    for (i = 0; i < THREADS; i++) {
        pthread_join(t[i], NULL);
    }
    
    printf("\n========================================\n");
    printf("The synchronized total number of prime numbers\n");
    printf("between 1 and %d is %d\n", LIMIT, total_primes);
    printf("========================================\n");
    
    pthread_mutex_destroy(&mutex);
    return (0);
}