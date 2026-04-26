#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("[child] PID = %d, PPID = %d\n", getpid(), getppid());
		sleep(30);
		printf("[child] exiting now\n");
        exit(5);
    } else {
        printf("[parent] PID = %d, child PID = %d\n", getpid(), pid);
        printf("[parent] sleeping 30 seconds without wait()\n");
		int status;
        wait(&status);
		printf("waited: %d\n", status);
        printf("[parent] done\n");
    }

    return 0;
}