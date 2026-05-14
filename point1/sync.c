#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>

static volatile sig_atomic_t buffer_ready = 0;
static volatile sig_atomic_t buffer_read = 0;
static volatile sig_atomic_t need_stop = 0;
static volatile sig_atomic_t other_stopped = 0;

static pid_t other_pid = -1;
static const char *role = NULL;

static void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        buffer_ready = 1;
    } else if (sig == SIGUSR2) {
        buffer_read = 1;
    } else if (sig == SIGINT) {
        need_stop = 1;

        if (other_pid > 0) {
            kill(other_pid, SIGTERM);
        }
    } else if (sig == SIGTERM) {
        other_stopped = 1;
    }
}

static void setup_handlers(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static void wait_signal(volatile sig_atomic_t *flag) {
    while (!*flag && !need_stop && !other_stopped) {
        pause();
    }

    *flag = 0;
}

static void check_buffer(unsigned int *buf, size_t count, unsigned int *expected) {
    for (size_t i = 0; i < count; i++) {
        if (buf[i] != *expected) {
            printf("reader: error at index %zu: expected %u, got %u\n",
                   i, *expected, buf[i]);

            *expected = buf[i] + 1;
        } else {
            (*expected)++;
        }
    }
}

static void writer(unsigned int *buf, size_t count) {
    unsigned int value = 0;

    while (!need_stop && !other_stopped) {
        for (size_t i = 0; i < count; i++) {
            buf[i] = value++;
        }

        kill(other_pid, SIGUSR1);

        wait_signal(&buffer_read);
    }
}

static void reader(unsigned int *buf, size_t count) {
    unsigned int expected = 0;

    while (!need_stop && !other_stopped) {
        wait_signal(&buffer_ready);

        if (need_stop || other_stopped) {
            break;
        }

        check_buffer(buf, count, &expected);

        kill(other_pid, SIGUSR2);
    }
}

static void cleanup(unsigned int *buf, size_t page_size) {
    if (need_stop) {
        printf("%s: got SIGINT, exiting\n", role);
    }

    if (other_stopped) {
        printf("%s: exiting because other process stopped\n", role);
    }

    munmap(buf, page_size);
}

int main(void) {
    long page_size_long = sysconf(_SC_PAGESIZE);

    if (page_size_long == -1) {
        perror("sysconf");
        return 1;
    }

    size_t page_size = (size_t)page_size_long;
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

    setup_handlers();

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        munmap(buf, page_size);
        return 1;
    }

    if (pid == 0) {
        role = "reader";
        other_pid = getppid();

        reader(buf, count);

        cleanup(buf, page_size);
        return 0;
    }

    role = "writer";
    other_pid = pid;

    writer(buf, count);

    cleanup(buf, page_size);

    waitpid(pid, NULL, 0);

    return 0;
}
