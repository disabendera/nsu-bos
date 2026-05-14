#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static pid_t create_process_with_redirected_stdout_to_child_stdin(void) {
    int fd[2];

    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        close(fd[1]);

        if (dup2(fd[0], STDIN_FILENO) == -1) {
            perror("dup2 child");
            exit(1);
        }

        close(fd[0]);

        char buffer[256];

        while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            fprintf(stderr, "child got from stdin: %s", buffer);
        }

        exit(0);
    }

    close(fd[0]);

    fflush(stdout);

    if (dup2(fd[1], STDOUT_FILENO) == -1) {
        perror("dup2 parent");
        exit(1);
    }

    close(fd[1]);

    return pid;
}

int main(void) {
    pid_t child_pid = create_process_with_redirected_stdout_to_child_stdin();

    printf("message 1 from parent\n");
    printf("message 2 from parent\n");
    printf("message 3 from parent\n");

    fflush(stdout);

    fclose(stdout);

    waitpid(child_pid, NULL, 0);

    return 0;
}
