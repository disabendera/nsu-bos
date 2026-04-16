#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int global_var = 100;

int main() {
    int local_var = 200;

    printf("=== До fork ===\n");
    printf("Parent PID: %d\n", getpid());
    printf("global_var: address=%p, value=%d\n", (void *)&global_var, global_var);
    printf("local_var : address=%p, value=%d\n", (void *)&local_var, local_var);
    printf("\n");

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        printf("=== Дочерний процесс ===\n");
        printf("Child PID : %d\n", getpid());
        printf("Parent PID: %d\n", getppid());

        printf("Before change:\n");
        printf("global_var: address=%p, value=%d\n", (void *)&global_var, global_var);
        printf("local_var : address=%p, value=%d\n", (void *)&local_var, local_var);
		sleep(20); // посмотреть maps до записи

        global_var = 1111;
        local_var = 2222;

        printf("After change:\n");
        printf("global_var: address=%p, value=%d\n", (void *)&global_var, global_var);
        printf("local_var : address=%p, value=%d\n", (void *)&local_var, local_var);

		// sleep(10); // для sigkill
		sleep(40); // посмотреть maps после записи

        printf("Child exits with code 5\n");
        exit(5);
    } else {
        int status;

        printf("=== Родительский процесс ===\n");
        printf("Parent PID: %d\n", getpid());
        printf("Child PID : %d\n", pid);

        printf("Parent sees variables before sleep:\n");
        printf("global_var: address=%p, value=%d\n", (void *)&global_var, global_var);
        printf("local_var : address=%p, value=%d\n", (void *)&local_var, local_var);

		// kill(pid, SIGKILL); // нет exit code, есть signal number

        printf("Parent sleeps for 30 seconds...\n");
        sleep(70);

        pid_t finished_pid = wait(&status);

        printf("wait() returned PID = %d\n", finished_pid);

        if (WIFEXITED(status)) {
            printf("Child terminated normally\n");
            printf("Exit code: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child terminated by signal\n");
            printf("Signal number: %d\n", WTERMSIG(status));
        } else {
            printf("Child terminated for another reason\n");
        }

        printf("Parent final values:\n");
        printf("global_var: address=%p, value=%d\n", (void *)&global_var, global_var);
        printf("local_var : address=%p, value=%d\n", (void *)&local_var, local_var);
    }

    return 0;
}
