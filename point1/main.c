#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    uid_t ruid = getuid();
    uid_t euid = geteuid();

    printf("Реальный UID (RUID): %d\n", ruid);
    printf("Эффективный UID (EUID): %d\n", euid);

    FILE *file = fopen("secret.txt", "r");
    if (file == NULL) {
        perror("Ошибка при открытии файла");
        return 1;
    }

    char buffer[256];
    printf("Содержимое файла:\n");
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }

    fclose(file);
    return 0;
}