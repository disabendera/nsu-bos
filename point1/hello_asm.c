#include <stddef.h>
#include <stdint.h>

static long my_write_asm(int fd, const void *buf, size_t count) {
    long ret;
    register long rax __asm__("rax") = 1;
    register long rdi __asm__("rdi") = fd;
    register long rsi __asm__("rsi") = (long)buf;
    register long rdx __asm__("rdx") = (long)count;

    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx)
        : "rcx", "r11", "memory"
    );
    return ret;
}

int main() {
    const char msg[] = "Hello world\n";
    my_write_asm(1, msg, sizeof(msg) - 1);
    return 0;
}