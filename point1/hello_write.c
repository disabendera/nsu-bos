#include <unistd.h>
#include <string.h>

int main() {
    const char msg[] = "Hello world\n";
    ssize_t rc = write(1, msg, sizeof(msg) - 1);
    (void)rc;
    return 0;
}
