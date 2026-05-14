#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

static void writer(int fd) {
    unsigned int value = 0;

    while (1) {
        ssize_t written = write(fd, &value, sizeof(value));

        if (written == -1) {
            perror("write");
            break;
        }

        if (written != sizeof(value)) {
            fprintf(stderr, "partial write\n");
            break;
        }

        value++;
    }
}

static void reader(int fd) {
    unsigned int expected = 0;

    while (1) {
        unsigned int value;

        ssize_t bytes = read(fd, &value, sizeof(value));

        if (bytes == -1) {
            perror("read");
            break;
        }

        if (bytes == 0) {
            printf("reader: pipe closed\n");
            break;
        }

        if (bytes != sizeof(value)) {
            fprintf(stderr, "partial read\n");
            break;
        }

        if (value != expected) {
            printf("reader: error: expected %u, got %u\n",
                   expected, value);

            expected = value + 1;
        } else {
            expected++;
        }
    }
}

int main(void) {
    int fd[2];

    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        close(fd[1]);

        reader(fd[0]);

        close(fd[0]);
        return 0;
    }

    close(fd[0]);

    writer(fd[1]);

    close(fd[1]);
    wait(NULL);

    return 0;
}
