#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    printf("\n");
    // Create 'sender' process
    if (!fork()) {
        while (1) {
            sleep(1);
            char* command_buf = NULL;
            size_t len = 0;
            if (getline(&command_buf, &len, stdin) != -1) {
                // Split the command into words
                char words[3][20] = {"", "", ""};
                int index = 0;
                int quote = -1;
                for (int i = 0; i < strlen(command_buf) - 1; i++) {
                    if (command_buf[i] == '"') {
                        quote *= -1;
                        continue;
                    }
                    if (command_buf[i] == ' ' && quote == -1) {
                        index++;
                        continue;
                    }
                    strncat(words[index], &command_buf[i], 1);
                }
                // printf("%s", words[1]);
                for (int i = 0; i < 3; i++) {
                    printf("%s\n", words[i]);
                }
            }

            // READLINE
            if (!fork()) {
                // execlp();
            }
        }
    }
    while (1) sleep(1);

    printf("\n");
    return 0;
}