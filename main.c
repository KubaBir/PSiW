#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int key_from_name(char* name) {
    int config_fd;
    config_fd = open("config", O_RDONLY);
    if (config_fd == -1) {
        perror("Error while opening config file. ");
        return -1;
    }
    FILE* config_file = fdopen(config_fd, "r");

    char* command_buf = NULL;
    size_t len = 0;
    char current[20] = "";
    int found = 0;
    char res[20] = "";
    while (1) {
        if (getline(&command_buf, &len, config_file) == -1) return -1;
        // Assume the line can contain the correct name
        int contains = 1;
        for (int i = 0; i < strlen(command_buf) - 1; i++) {
            if (!contains) break;
            if (found) {
                strncat(res, &command_buf[i], 1);
            }
            if (command_buf[i] == ' ' && !found) {
                if (!strcmp(current, name))
                    found = 1;
                else {
                    strcpy(current, "");
                    // If the first word is wrong, don't check the next one
                    contains = 0;
                    continue;
                }
            }
            strncat(current, &command_buf[i], 1);
        }
        if (found) break;
    }
    fclose(config_file);
    close(config_fd);
    if (!found) {
        printf("Process not found!\n");
        return -1;
    }

    return atoi(res);
}

int send_request(char receiver[], char command[]) {
    char receiver_key[30];
    sprintf(receiver_key, "%d", key_from_name(receiver));
    int outgoing_fd = open(receiver_key, O_WRONLY);
    if (outgoing_fd == -1) {
        printf("Wrong process name!\n");
        return -1;
    }

    dup(outgoing_fd);
    fflush(stdout);
    execl("/bin/sh", "/bin/sh", "-c", command, NULL);

    return 0;
}

int await_command() {
    while (1) {
        sleep(1);
        char* command_buf = NULL;
        size_t len = 0;
        if (getline(&command_buf, &len, stdin) == -1) continue;
        char words[3][100] = {"", "", ""};
        int index = 0;
        int quote = -1;
        int error = 0;
        // Split the command into words
        for (int i = 0; i < strlen(command_buf) - 1; i++) {
            if (command_buf[i] == '"') {
                quote *= -1;
                continue;
            }
            if (command_buf[i] == ' ' && quote == -1) {
                index++;
                if (index > 2) {
                    printf("Too many arguments!\n");
                    error = 1;
                    break;
                }
                continue;
            }
            strncat(words[index], &command_buf[i], 1);
        }

        if (error) continue;
        fflush(stdout);
        if (!fork()) {
            send_request(words[0], words[1]);
            return -1;
        }
    }
}

int main(int argc, char* argv[]) {
    char my_key[30];
    sprintf(my_key, "%d", key_from_name("test"));
    printf("%s\n", my_key);
    // Open file

    // Process file

    printf("\n");

    mkfifo(my_key, 0664);
    // Create 'sender' process
    if (!fork())
        await_command();

    // Create receive process
    char incoming_msg[100];
    int incoming_fd = open(my_key, O_RDONLY);
    while (1) {
        sleep(1);
        if (!read(incoming_fd, &incoming_msg, sizeof(incoming_msg))) continue;
        printf("essa%s\n", incoming_msg);
    }

    printf("\n");
    return 0;
}