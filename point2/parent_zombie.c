#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t parent_pid = fork();

    if (parent_pid < 0) {
        perror("fork parent");
        return 1;
    }

    if (parent_pid == 0) {
        // Это parent
        pid_t child_pid = fork();

        if (child_pid < 0) {
            perror("fork child");
            exit(1);
        }

        if (child_pid == 0) {
            // Это child
            printf("[child ] PID=%d, initial PPID=%d\n", getpid(), getppid());
            sleep(10);
            printf("[child ] PID=%d, new PPID=%d\n", getpid(), getppid());
            sleep(20);
            printf("[child ] exiting\n");
            exit(0);
        } else {
            // Это parent
            printf("[parent] PID=%d, PPID=%d, child=%d\n", getpid(), getppid(), child_pid);
            printf("[parent] exiting immediately\n");
            exit(0);
        }
    } else {
        // Это grandparent
        printf("[grand ] PID=%d, child(parent)=%d\n", getpid(), parent_pid);
        printf("[grand ] sleeping 40 seconds without wait()\n");
        sleep(40);
        printf("[grand ] exiting\n");
    }

    return 0;
}