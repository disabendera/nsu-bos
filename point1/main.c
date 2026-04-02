#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int g_init = 42;

int g_uninit;

const int g_const = 100;

const char *g_str = "global string literal";

void print_addresses(void) {
    int local_var = 10;

    static int static_var = 20;

    const int local_const = 30;

    const char *local_str = "local string literal";

    printf("PID: %d\n\n", getpid());

    printf("=== Addresses of variables ===\n");
    printf("local_var                : %p (value=%d)\n", (void *)&local_var, local_var);
    printf("static_var               : %p (value=%d)\n", (void *)&static_var, static_var);
    printf("local_const              : %p (value=%d)\n", (void *)&local_const, local_const);

    printf("g_init                   : %p (value=%d)\n", (void *)&g_init, g_init);
    printf("g_uninit                 : %p (value=%d)\n", (void *)&g_uninit, g_uninit);
    printf("g_const                  : %p (value=%d)\n", (void *)&g_const, g_const);

    printf("\n=== Addresses of pointers / literals ===\n");
    printf("g_str variable           : %p\n", (void *)&g_str);
    printf("g_str points to          : %p -> \"%s\"\n", (void *)g_str, g_str);

    printf("local_str variable       : %p\n", (void *)&local_str);
    printf("local_str points to      : %p -> \"%s\"\n", (void *)local_str, local_str);
}

int *return_local_address(void) {
    int local_in_func = 12345;

    printf("\n=== Returning address of local variable ===\n");
    printf("Inside return_local_address():\n");
    printf("local_in_func address      : %p\n", (void *)&local_in_func);
    printf("local_in_func value        : %d\n", local_in_func);

    return &local_in_func;
}

void heap_experiment(void) {
    printf("\n=== Heap experiment ===\n");

    char *buf = (char *)malloc(100);
    if (buf == NULL) {
        perror("malloc");
        return;
    }

    strcpy(buf, "hello world");

    printf("buf address               : %p\n", (void *)buf);
    printf("buf content before free   : %s\n", buf);

    free(buf);

    printf("buf content after free    : %s\n", buf);

    char *buf2 = (char *)malloc(100);
    if (buf2 == NULL) {
        perror("malloc");
        return;
    }

    strcpy(buf2, "hello world");

    printf("buf2 address              : %p\n", (void *)buf2);
    printf("buf2 content              : %s\n", buf2);

    char *middle = buf2 + 50;
    printf("middle pointer            : %p\n", (void *)middle);

    printf("free(middle) ...\n");
    free(middle);

    printf("buf2 content after bad free: %s\n", buf2);
}

void env_experiment(void) {
    const char *var_name = "LAB4_ENV";

    printf("\n=== Environment variable experiment ===\n");

    char *initial_value = getenv(var_name);
    printf("%s before change       : %s\n",
           var_name, initial_value ? initial_value : "(null)");

    if (setenv(var_name, "second_value", 1) != 0) {
        perror("setenv");
        return;
    }

    char *new_value = getenv(var_name);
    printf("%s after change        : %s\n",
           var_name, new_value ? new_value : "(null)");
}

int main(void) {
    print_addresses();

    int *dangling_ptr = return_local_address();

    printf("\nAfter returning from function:\n");
    printf("dangling_ptr               : %p\n", (void *)dangling_ptr);

    // printf("value by dangling_ptr      : %d\n", *dangling_ptr);

    heap_experiment();

    env_experiment();

    printf("\nProgram is sleeping for 60 seconds...\n");
    sleep(60);

    return 0;
}