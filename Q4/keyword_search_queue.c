#define _GNU_SOURCE

/**
 * keyword_search_queue.c - Multi-threaded keyword search with work queue
 * Compile: gcc -pthread -o search keyword_search_queue.c
 * Usage: ./search keyword output.txt file1.txt file2.txt ... num_threads
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

/* Global variables */
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE *output_file = NULL;
long long total_occurrences = 0;
int files_processed = 0;
int total_files = 0;
int next_file_index = 0;
char **file_list = NULL;
char *keyword = NULL;

typedef struct {
    int thread_id;
    long long local_count;
    double exec_time;
} thread_args_t;

double get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

long long count_occurrences(const char *filename, const char *keyword, 
                            int output, FILE *fp_output)
{
    FILE *fp;
    char line[1024];
    long long count = 0;
    char *pos;
    int keyword_len;
    
    keyword_len = strlen(keyword);
    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return 0;
    }
    
    /* Read file line by line using fgets */
    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Remove newline character */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        /* Count occurrences in this line */
        pos = line;
        while ((pos = strstr(pos, keyword)) != NULL) {
            count++;
            pos += keyword_len;
        }
        
        /* If output is enabled and occurrences found, write to output file */
        if (output && count > 0 && fp_output != NULL) {
            pthread_mutex_lock(&file_mutex);
            fprintf(fp_output, "[%s]: %s\n", filename, line);
            fflush(fp_output);
            pthread_mutex_unlock(&file_mutex);
        }
    }
    
    fclose(fp);
    return count;
}

void *search_worker(void *arg)
{
    thread_args_t *args = (thread_args_t *)arg;
    int file_index;
    char *filename;
    long long occurrences;
    double start_time, end_time;
    int files_processed_local;
    
    start_time = get_time_ms();
    files_processed_local = 0;
    
    while (1) {
        /* Get next file from queue */
        pthread_mutex_lock(&queue_mutex);
        if (next_file_index >= total_files) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        file_index = next_file_index++;
        filename = file_list[file_index];
        pthread_mutex_unlock(&queue_mutex);
        
        /* Process the file */
        occurrences = count_occurrences(filename, keyword, 0, NULL);
        
        if (occurrences > 0) {
            count_occurrences(filename, keyword, 1, output_file);
        }
        
        /* Update global counters */
        pthread_mutex_lock(&counter_mutex);
        total_occurrences += occurrences;
        files_processed++;
        pthread_mutex_unlock(&counter_mutex);
        
        args->local_count += occurrences;
        files_processed_local++;
        
        printf("Thread %2d: Processed '%s' - Found %lld occurrences\n",
               args->thread_id, filename, occurrences);
    }
    
    end_time = get_time_ms();
    args->exec_time = end_time - start_time;
    
    printf("Thread %2d: Completed - Processed %d files, found %lld occurrences (%.2f ms)\n",
           args->thread_id, files_processed_local, args->local_count, args->exec_time);
    
    return NULL;
}

int get_cpu_cores(void)
{
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    return (cores > 0) ? cores : 1;
}

int main(int argc, char *argv[])
{
    pthread_t *threads;
    thread_args_t *thread_args;
    int num_files;
    int num_threads;
    int i;
    double start_time, end_time;
    int cpu_cores;
    
    if (argc < 5) {
        fprintf(stderr, "Usage: %s keyword output.txt file1.txt file2.txt ... num_threads\n", argv[0]);
        fprintf(stderr, "Example: %s \"error\" results.txt log1.txt log2.txt log3.txt 4\n", argv[0]);
        return 1;
    }
    
    num_threads = atoi(argv[argc - 1]);
    if (num_threads < 1) {
        fprintf(stderr, "Error: Number of threads must be at least 1\n");
        return 1;
    }
    
    num_files = argc - 3 - 1;  /* subtract keyword, output, and thread count */
    keyword = argv[1];
    
    if (num_files < 1) {
        fprintf(stderr, "Error: At least one file to search is required\n");
        return 1;
    }
    
    cpu_cores = get_cpu_cores();
    printf("\n========================================\n");
    printf("Multi-Threaded Keyword Search\n");
    printf("========================================\n");
    printf("Keyword: '%s'\n", keyword);
    printf("Output file: %s\n", argv[2]);
    printf("Files to search: %d\n", num_files);
    printf("Threads: %d (CPU cores: %d)\n", num_threads, cpu_cores);
    if (num_threads > cpu_cores) {
        printf("Warning: Using more threads than CPU cores may degrade performance\n");
    }
    printf("========================================\n\n");
    
    /* Open output file */
    output_file = fopen(argv[2], "w");
    if (output_file == NULL) {
        fprintf(stderr, "Error: Could not create output file '%s'\n", argv[2]);
        return 1;
    }
    
    fprintf(output_file, "Keyword: '%s'\n", keyword);
    fprintf(output_file, "Search results:\n");
    fprintf(output_file, "========================================\n\n");
    fflush(output_file);
    
    /* Prepare file list */
    file_list = malloc(num_files * sizeof(char *));
    for (i = 0; i < num_files; i++) {
        file_list[i] = argv[3 + i];
    }
    total_files = num_files;
    
    /* Allocate threads */
    threads = malloc(num_threads * sizeof(pthread_t));
    thread_args = malloc(num_threads * sizeof(thread_args_t));
    
    start_time = get_time_ms();
    
    /* Create worker threads */
    for (i = 0; i < num_threads; i++) {
        thread_args[i].thread_id = i;
        thread_args[i].local_count = 0;
        thread_args[i].exec_time = 0;
        
        if (pthread_create(&threads[i], NULL, search_worker, &thread_args[i]) != 0) {
            fprintf(stderr, "Error: Could not create thread %d\n", i);
            goto cleanup;
        }
    }
    
    /* Wait for all threads */
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    end_time = get_time_ms();
    
    /* Write summary */
    pthread_mutex_lock(&file_mutex);
    fprintf(output_file, "\n========================================\n");
    fprintf(output_file, "SUMMARY\n");
    fprintf(output_file, "========================================\n");
    fprintf(output_file, "Total occurrences found: %lld\n", total_occurrences);
    fprintf(output_file, "Files processed: %d\n", files_processed);
    fprintf(output_file, "Execution time: %.2f ms\n", end_time - start_time);
    fprintf(output_file, "Threads used: %d\n", num_threads);
    fprintf(output_file, "========================================\n");
    fflush(output_file);
    pthread_mutex_unlock(&file_mutex);
    
    /* Display results */
    printf("\n========================================\n");
    printf("SEARCH COMPLETE\n");
    printf("========================================\n");
    printf("Total occurrences found: %lld\n", total_occurrences);
    printf("Files processed: %d\n", files_processed);
    printf("Total execution time: %.2f ms\n", end_time - start_time);
    if (end_time > start_time) {
        printf("Throughput: %.2f files/second\n", 
               (files_processed * 1000.0) / (end_time - start_time));
    }
    printf("Results written to: %s\n", argv[2]);
    printf("========================================\n");
    
cleanup:
    free(threads);
    free(thread_args);
    free(file_list);
    fclose(output_file);
    
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&counter_mutex);
    
    return 0;
}