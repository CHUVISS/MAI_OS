#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}

int main() {
    int pipe1[2], pipe2[2];
    char buffer[BUFFER_SIZE];
    int message_count = 1;
    char file1[BUFFER_SIZE];
    char file2[BUFFER_SIZE];

    const char* error_pipe = "Failed to create pipe.\n";
    const char* error_fork1 = "Failed to fork process 1.\n";
    const char* error_open1 = "Failed to open file in child 1.\n";
    const char* error_fork2 = "Failed to fork process 2.\n";
    const char* error_open2 = "Failed to open file in child 2.\n";

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        write(2, error_pipe, strlen(error_pipe));
        exit(EXIT_FAILURE);
    }

    write(1, "Enter filename for child 1: ", strlen("Enter filename for child 1: "));
    read(0, file1, BUFFER_SIZE);
    file1[strcspn(file1, "\n")] = '\0';

    write(1, "Enter filename for child 2: ", strlen("Enter filename for child 2: "));
    read(0, file2, BUFFER_SIZE);
    file2[strcspn(file2, "\n")] = '\0';

    if (fork() == 0) {
        int fd1 = open(file1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd1 == -1) {
            write(2, error_open1, strlen(error_open1));
            exit(EXIT_FAILURE);
        }

        close(pipe1[1]);
        dup2(pipe1[0], 0);
        dup2(fd1, 1);
        close(pipe1[0]);
        close(fd1);

        execl("./child1", "child1", NULL);
        write(2, "Execution failed for child 1.\n", strlen("Execution failed for child 1.\n"));
        exit(EXIT_FAILURE);
    }

    if (fork() == 0) {
        int fd2 = open(file2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd2 == -1) {
            write(2, error_open2, strlen(error_open2));
            exit(EXIT_FAILURE);
        }

        close(pipe2[1]);
        dup2(pipe2[0], 0);
        dup2(fd2, 1);
        close(pipe2[0]);
        close(fd2);

        execl("./child2", "child2", NULL);
        write(2, "Execution failed for child 2.\n", strlen("Execution failed for child 2.\n"));
        exit(EXIT_FAILURE);
    }

    close(pipe1[0]);
    close(pipe2[0]);

    while (1) {
        write(1, "Enter a line: ", strlen("Enter a line: "));
        ssize_t bytes_read = read(0, buffer, BUFFER_SIZE);
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read - 1] = '\0';
        
        if (strlen(buffer) == 0 || strcmp(buffer, "exit") == 0) {
            break;
        }

        if (message_count % 2 == 0) {
            write(pipe2[1], buffer, bytes_read);
        } else {
            write(pipe1[1], buffer, bytes_read);
        }
        message_count++;
    }

    close(pipe1[1]);
    close(pipe2[1]);

    return 0;
}
