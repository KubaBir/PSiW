#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    // Load config

    // Open file
    int config_fd;
    config_fd = open("config", O_RDONLY);
    if (config_fd == -1)
        perror("Error while opening config file. ");

    // Process file
    char buf;
    while (read(config_fd, &buf, 1) != 0) {
        printf("%c", buf);
    }
    // Create 'sender' process
    if (!fork()) {
        while (1) {
            sleep(1);
            char* command_buf = NULL;
            size_t len = 0;
            if (getline(&command_buf, &len, stdin) != -1)
                printf("%s", command_buf);

            // READLINE
            if (!fork()) {
                // execlp();
            }
        }
    }

    printf("\n");
    return 0;
}