#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("pid = %d\n", getpid());
    fflush(stdout);

    if (argc > 1 && strcmp(argv[1], "--after-exec") == 0) {
        printf("Hello world\n");
        sleep(5);
        fflush(stdout);
        return 0;
    }

    sleep(5);

    printf("About to exec myself...\n");
    fflush(stdout);

    execl(argv[0], argv[0], "--after-exec", NULL);

    perror("execl");

    return 1;
}