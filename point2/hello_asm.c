#include <stddef.h>
#include <stdint.h>

static long my_write_asm(int fd, const void *buf, size_t count) {
    long ret;
    register long rax __asm__("rax") = 1;
    register long rdi __asm__("rdi") = fd;
    register long rsi __asm__("rsi") = (long)buf;
    register long rdx __asm__("rdx") = (long)count;

	/*
	mov rax, 1
	mov rdi, 1
	mov rsi, msg
	mov rdx, 12
	syscall
	*/

    __asm__ volatile (
        "syscall"
        : "=a"(ret) // = - только запись
        : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx) // регистровые константы
        : "rcx", "r11", "memory" // clobbered - регистры rcx, r11 будут изменены внутри asm, а также может читать и изменять память
    );
    return ret;
}

int main() {
    const char msg[] = "Hello world\n";
    my_write_asm(1, msg, sizeof(msg) - 1);
    return 0;
}