#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>

#define BUFFER_SIZE 4096
#define DEFAULT_SRC "largefile.bin"
#define DEFAULT_DEST_SYS "copy1.bin"
#define DEFAULT_DEST_STDIO "copy2.bin"

/**
 * get_time_ms - Get current time in milliseconds
 * Return: current time in milliseconds
 */
double get_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

/**
 * copy_syscall - Copy using low-level read/write system calls
 * @src: source file path
 * @dest: destination file path
 * Return: number of bytes copied, -1 on error
 */
ssize_t copy_syscall(const char *src, const char *dest)
{
    int s_fd, d_fd;
    char buf[BUFFER_SIZE];
    ssize_t n, total_bytes;
    double start_time, end_time;
    
    total_bytes = 0;
    
    /* Open source file for reading */
    s_fd = open(src, O_RDONLY);
    if (s_fd == -1) {
        perror("Error opening source file");
        return -1;
    }
    
    /* Open destination file for writing */
    d_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (d_fd == -1) {
        perror("Error opening destination file");
        close(s_fd);
        return -1;
    }
    
    /* Start timing */
    start_time = get_time_ms();
    
    /* Copy file using read/write system calls */
    while ((n = read(s_fd, buf, sizeof(buf))) > 0) {
        if (write(d_fd, buf, n) != n) {
            perror("Write error");
            break;
        }
        total_bytes += n;
    }
    
    /* End timing */
    end_time = get_time_ms();
    
    /* Close file descriptors */
    close(s_fd);
    close(d_fd);
    
    /* Display performance metrics */
    printf("\n=== System Calls Version (read/write) ===\n");
    printf("Source: %s\n", src);
    printf("Destination: %s\n", dest);
    printf("Buffer size: %d bytes\n", BUFFER_SIZE);
    printf("Total bytes: %ld (%.2f MB)\n", (long)total_bytes, 
           (double)total_bytes / (1024.0 * 1024.0));
    printf("Execution time: %.2f ms\n", end_time - start_time);
    if (end_time > start_time) {
        printf("Throughput: %.2f MB/s\n", 
               ((double)total_bytes / (1024.0 * 1024.0)) / ((end_time - start_time) / 1000.0));
    }
    
    return total_bytes;
}

/**
 * copy_stdio - Copy using standard I/O functions
 * @src: source file path
 * @dest: destination file path
 * Return: number of bytes copied, -1 on error
 */
size_t copy_stdio(const char *src, const char *dest)
{
    FILE *s, *d;
    char buf[BUFFER_SIZE];
    size_t n, total_bytes;
    double start_time, end_time;
    
    total_bytes = 0;
    
    /* Open source file for reading */
    s = fopen(src, "rb");
    if (s == NULL) {
        perror("Error opening source file");
        return -1;
    }
    
    /* Open destination file for writing */
    d = fopen(dest, "wb");
    if (d == NULL) {
        perror("Error opening destination file");
        fclose(s);
        return -1;
    }
    
    /* Start timing */
    start_time = get_time_ms();
    
    /* Copy file using fread/fwrite standard I/O */
    while ((n = fread(buf, 1, sizeof(buf), s)) > 0) {
        if (fwrite(buf, 1, n, d) != n) {
            fprintf(stderr, "Write error\n");
            break;
        }
        total_bytes += n;
    }
    
    /* End timing */
    end_time = get_time_ms();
    
    /* Close files */
    fclose(s);
    fclose(d);
    
    /* Display performance metrics */
    printf("\n=== Standard I/O Version (fread/fwrite) ===\n");
    printf("Source: %s\n", src);
    printf("Destination: %s\n", dest);
    printf("Buffer size: %d bytes\n", BUFFER_SIZE);
    printf("Total bytes: %lu (%.2f MB)\n", (unsigned long)total_bytes,
           (double)total_bytes / (1024.0 * 1024.0));
    printf("Execution time: %.2f ms\n", end_time - start_time);
    if (end_time > start_time) {
        printf("Throughput: %.2f MB/s\n", 
               ((double)total_bytes / (1024.0 * 1024.0)) / ((end_time - start_time) / 1000.0));
    }
    
    return total_bytes;
}

/**
 * create_test_file - Create a test file of specified size
 * @filename: name of the file to create
 * @size_mb: size in megabytes
 * Return: 0 on success, -1 on failure
 */
int create_test_file(const char *filename, size_t size_mb)
{
    FILE *f;
    char *buffer;
    size_t buffer_size;
    size_t written;
    size_t to_write;
    unsigned int i;
    
    f = fopen(filename, "wb");
    if (f == NULL) {
        perror("Error creating test file");
        return -1;
    }
    
    printf("Creating test file %s (%lu MB)...\n", filename, (unsigned long)size_mb);
    
    buffer_size = 1024 * 1024; /* 1MB buffer */
    buffer = (char *)malloc(buffer_size);
    if (buffer == NULL) {
        perror("Memory allocation failed");
        fclose(f);
        return -1;
    }
    
    /* Fill buffer with pattern */
    for (i = 0; i < buffer_size; i++) {
        buffer[i] = (char)(i % 256);
    }
    
    /* Write the file */
    written = 0;
    while (written < size_mb * 1024 * 1024) {
        to_write = buffer_size;
        if (written + to_write > size_mb * 1024 * 1024) {
            to_write = size_mb * 1024 * 1024 - written;
        }
        if (fwrite(buffer, 1, to_write, f) != to_write) {
            perror("Write error");
            free(buffer);
            fclose(f);
            return -1;
        }
        written += to_write;
    }
    
    free(buffer);
    fclose(f);
    printf("Test file created successfully.\n");
    return 0;
}

int main(int argc, char **argv)
{
    int mode;
    const char *src, *dest;
    ssize_t sys_bytes;
    size_t stdio_bytes;
    FILE *test;
    
    /* Parse command line arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mode> [source] [destination]\n", argv[0]);
        fprintf(stderr, "mode: 1 = system calls, 2 = standard I/O\n");
        fprintf(stderr, "Example: %s 1 largefile.bin copy.bin\n", argv[0]);
        fprintf(stderr, "If no source/destination specified, defaults will be used.\n");
        return 1;
    }
    
    mode = atoi(argv[1]);
    
    if (argc >= 4) {
        src = argv[2];
        dest = argv[3];
    } else {
        /* Use default filenames */
        src = DEFAULT_SRC;
        if (mode == 1)
            dest = DEFAULT_DEST_SYS;
        else
            dest = DEFAULT_DEST_STDIO;
    }
    
    /* Check if source file exists, if not create test file */
    test = fopen(src, "rb");
    if (test == NULL) {
        printf("Source file '%s' not found. Creating 100MB test file...\n", src);
        if (create_test_file(src, 100) != 0) {
            fprintf(stderr, "Failed to create test file\n");
            return 1;
        }
    } else {
        fclose(test);
        printf("Using existing source file: %s\n", src);
    }
    
    /* Execute the appropriate copy function */
    if (mode == 1) {
        printf("\n--- Running System Calls Version ---\n");
        sys_bytes = copy_syscall(src, dest);
        if (sys_bytes == -1) {
            fprintf(stderr, "Copy failed\n");
            return 1;
        }
    } else if (mode == 2) {
        printf("\n--- Running Standard I/O Version ---\n");
        stdio_bytes = copy_stdio(src, dest);
        if (stdio_bytes == (size_t)-1) {
            fprintf(stderr, "Copy failed\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Invalid mode. Use 1 for system calls or 2 for standard I/O\n");
        return 1;
    }
    
    printf("\nCopy completed successfully.\n");
    return 0;
}