# Systems Programming Project 2: Process Management, I/O, and Multithreading

Comprehensive low-level Linux systems programming project exploring process lifecycle, IPC, I/O performance, and POSIX multithreading — implemented in **C89/C90** following **Betty style** (ALX-Holberton compliant).

## Project Objectives

- Implement a Unix-style command **pipeline** using raw system calls (`fork`, `pipe`, `execvp`, `dup2`)
- Compare performance of **low-level system calls** (`read/write`) vs **buffered standard I/O** (`fread/fwrite`)
- Build a **multi-threaded prime number counter** (16 threads + mutex synchronization)
- Create a **concurrent multi-threaded keyword search** utility with dynamic work distribution and safe file writing

## Technical Stack

- **Language**: C (C89 / `-std=gnu89`)
- **OS**: Linux / Ubuntu / WSL
- **Compiler**: gcc
- **Tools**: strace, valgrind, gdb, time
- **Libraries**: pthread, math (`-lm`)
- **Key system calls**: `fork()`, `pipe()`, `execvp()`, `dup2()`, `read()`, `write()`, `open()`, `close()`

## Directory Structure
Project_2_assignment/
├── Q1/
│   ├── pipeline.c           # ps aux | grep root pipeline
│   └── output.txt           # captured output
├── Q2/
│   ├── file_copy.c          # file copy with syscall vs stdio comparison
│   └── largefile.bin        # 100 MB test file (generated via dd)
├── Q3/
│   ├── prime_counter.c      # 16-thread prime counter (1–200,000)
│   └── prime_counter_timed.c # version with detailed timing
├── Q4/
│   ├── keyword_search.c     # multi-threaded keyword search across files
│   └── test_files/          # sample text/log files
└── README.md
text## Installation & Compilation

```bash
# Install dependencies (Ubuntu / WSL)
sudo apt update
sudo apt install build-essential gcc strace valgrind

# Compile each question
gcc -Wall -Werror -Wextra -pedantic -std=gnu89 -o pipeline    Q1/pipeline.c
gcc -O2                     -o file_copy   Q2/file_copy.c
gcc -O2 -pthread            -o prime_counter Q3/prime_counter.c -lm
gcc -O2 -pthread            -o search       Q4/keyword_search.c -lm
Question 1 – Unix Pipeline (ps aux | grep root)
Implements a two-process pipeline using raw system calls.
Bash./pipeline
Features:

fork() → two children
pipe() → connect stdout → stdin
dup2() → I/O redirection
execvp() → run ps aux and grep root
Output captured to file + first 10 lines displayed

Question 2 – I/O Performance Comparison
Copies 100 MB file using two methods:
MethodThroughputSystem CallsTimeNotesread()/write()~576 MB/s~51,20073.57 msBaseline – many kernel transitionsfread()/fwrite()~724 MB/s~12,000138.14 ms~25–30% faster – user buffering
Generate test file:
Bashdd if=/dev/urandom of=largefile.bin bs=1M count=100
Run:
Bash./file_copy 1 largefile.bin copy_sys.bin     # system calls
./file_copy 2 largefile.bin copy_stdio.bin   # standard I/O
strace -c ./file_copy 1 ...                  # syscall count
Conclusion: Standard I/O is significantly faster for typical file operations due to buffering.
Question 3 – Multi-threaded Prime Counter
Counts prime numbers from 1 to 200,000 using 16 threads.

Static range partitioning
Global counter protected by pthread_mutex_t
Verified result: 17,984 primes

Performance (example run):

Multi-threaded (16 threads): ~2.3 ms
Single-threaded: ~8.5 ms
Speedup: ~3.7×

Run:
Bash./prime_counter
Question 4 – Concurrent Keyword Search
Multi-threaded search for a keyword across multiple files.

Dynamic work queue (better load balancing)
Mutex-protected shared output file
Counts occurrences + writes matching lines

Example usage:
Bash./search "ERROR" results.txt *.log 8
Scaling example (100 MB data):
ThreadsSpeedupThroughput (files/sec)11.0×~2043.6×~7385.8×~119126.4×~130166.7×~136
Best scaling near physical core count.
