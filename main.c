#include <fcntl.h>
#include <math.h>
#include <signal.h>
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

int send_response(char command[]) {
    // Command, callback
    char words[2][100] = {"", ""};
    int index = 0;
    int error = 1;
    // Split the command into words
    for (int i = 0; i < strlen(command); i++) {
        if (command[i] == '"') {
            error = 0;
            index++;
            continue;
        }
        strncat(words[index], &command[i], 1);
    }
    if (error) return -1;

    // printf("Command is: %s\n", words[0]);
    // printf("Response will be sent to: %s\n", words[1]);
    int outgoing_fd = open(words[1], O_WRONLY);
    if (outgoing_fd == -1) {
        printf("Error while opening callback queue\n");
        return -1;
    }
    close(STDOUT_FILENO);
    dup(outgoing_fd);

    // execl("/bin/sh", "/bin/sh", "-c", words[0], NULL);
    system(words[0]);

    return 0;
}

int await_response(char callback[]) {
    mkfifo(callback, 0664);
    int fd = open(callback, O_RDONLY);
    char command_buf[200];
    read(fd, &command_buf, sizeof(command_buf));
    printf("Response:\n%s\n", command_buf);
    unlink(callback);
    raise(SIGINT);
    return 0;
}

int send_request(char receiver[], char command[], char callback[]) {
    char receiver_key[30];
    sprintf(receiver_key, "%d", key_from_name(receiver));
    int outgoing_fd = open(receiver_key, O_WRONLY);
    if (outgoing_fd == -1) {
        printf("Wrong process name!\n");
        return -1;
    }
    char sep = '"';
    strncat(command, &sep, 1);
    strcat(command, callback);
    if (!fork())
        await_response(callback);
    int n_bytes = write(outgoing_fd, command, (strlen(command) + 1));
    printf("Sent %d bytes\n", n_bytes);

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

        printf("Recieved a proper command, sending...\n");
        if (!fork()) {
            send_request(words[0], words[1], words[2]);
            raise(SIGINT);
            return -1;
        }
    }
}

int main(int argc, char* argv[]) {
    char my_key[30];
    if (argc == 1)
        strcpy(my_key, "1");
    else
        sprintf(my_key, "%d", key_from_name(argv[1]));
    printf("%s\n", my_key);

    mkfifo(my_key, 0664);

    // Create 'receiver' process
    int receiver_pid = fork();
    if (receiver_pid == 0) {
        char incoming_msg[100];
        int incoming_fd = open(my_key, O_RDONLY);
        while (1) {
            if (!read(incoming_fd, &incoming_msg, sizeof(incoming_msg))) continue;
            printf("Got request!\n");
            if (!fork())
                send_response(incoming_msg);
        }
    } else
        await_command();

    unlink(my_key);
    return 0;
}