#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>

int main(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    size_t count = page_size / sizeof(unsigned int);

    unsigned int *buf = mmap(
        NULL,
        page_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (buf == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        munmap(buf, page_size);
        return 1;
    }

    if (pid == 0) {
        unsigned int expected = 0;

        while (1) {
            for (size_t i = 0; i < count; i++) {
                if (buf[i] != expected) {
                    printf("reader: error at index %zu: expected %u, got %u\n",
                           i, expected, buf[i]);

                    expected = buf[i] + 1;
                } else {
                    expected++;
                }
            }
        }
    } else {
        unsigned int value = 0;

        while (1) {
            for (size_t i = 0; i < count; i++) {
                buf[i] = value++;
            }
        }

        wait(NULL);
        munmap(buf, page_size);
    }

    return 0;
}
