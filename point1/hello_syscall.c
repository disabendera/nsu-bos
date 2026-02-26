#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

static ssize_t my_write(int fd, const void *buf, size_t count) {
    return syscall(SYS_write, fd, buf, count);
}

int main() {
    const char msg[] = "Hello world\n";
    ssize_t rc = my_write(1, msg, sizeof(msg) - 1);
    (void)rc;
    return 0;
}