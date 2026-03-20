#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define DISPLAY_LINES 10

/**
 * main - Implements a pipeline ps aux | grep root and captures output
 * Return: 0 on success, 1 on failure
 */
int main(void)
{
	int pipefd[2];
	pid_t pid1, pid2;
	int file_fd;
	char line[1024];
	FILE *fp;
	int line_count;
	char *args_ps[] = {"ps", "aux", NULL};
	char *args_grep[] = {"grep", "root", NULL};
	
	/* Initialize variables */
	line_count = 0;
	
	/* Create pipe for communication between processes */
	if (pipe(pipefd) == -1)
		return (perror("pipe"), 1);
	
	/* First child - executes "ps aux" */
	pid1 = fork();
	if (pid1 == 0)
	{
		/* Redirect stdout to pipe write end */
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		
		execvp(args_ps[0], args_ps);
		perror("execvp ps");
		exit(1);
	}
	
	/* Second child - executes "grep root" and writes to file */
	pid2 = fork();
	if (pid2 == 0)
	{
		/* Open output file */
		file_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (file_fd == -1)
		{
			perror("open");
			exit(1);
		}
		
		/* Redirect stdin from pipe read end */
		dup2(pipefd[0], STDIN_FILENO);
		/* Redirect stdout to file */
		dup2(file_fd, STDOUT_FILENO);
		
		close(pipefd[1]);
		close(pipefd[0]);
		close(file_fd);
		
		execvp(args_grep[0], args_grep);
		perror("execvp grep");
		exit(1);
	}
	
	/* Parent process - close pipe ends */
	close(pipefd[0]);
	close(pipefd[1]);
	
	/* Wait for both child processes to complete */
	wait(NULL);
	wait(NULL);
	
	printf("\n================================================\n");
	printf("Pipeline finished. Results saved to output.txt\n");
	printf("================================================\n");
	
	/* Read and display part of the output (first 10 lines) */
	printf("\n=== First %d lines of output ===\n", DISPLAY_LINES);
	printf("----------------------------------------\n");
	
	fp = fopen("output.txt", "r");
	if (fp == NULL)
	{
		perror("fopen");
		return (1);
	}
	
	while (fgets(line, sizeof(line), fp) != NULL && line_count < DISPLAY_LINES)
	{
		printf("%3d: %s", line_count + 1, line);
		line_count++;
	}
	
	if (line_count == 0)
	{
		printf("No lines containing 'root' were found.\n");
	}
	else if (line_count == DISPLAY_LINES)
	{
		printf("\n... (truncated after %d lines) ...\n", DISPLAY_LINES);
	}
	
	fclose(fp);
	printf("\n");
	
	return (0);
}