#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

void to_uppercase(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = toupper(str[i]);
    }
}

int main() {
    char buffer[BUFFER_SIZE];

    while (1) {
        ssize_t bytes_read = read(0, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            
            exit(EXIT_SUCCESS);
        }
        buffer[bytes_read - 1] = '\0';

        if (strlen(buffer) == 0) {
            break;
        }

        to_uppercase(buffer);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
    }

    return 0;
}
