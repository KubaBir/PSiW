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
    strtok(name, "\n");
    int config_fd;
    config_fd = open("config", O_RDONLY);
    if (config_fd == -1) {
        perror("Error while opening config file. ");
        return -1;
    }

    char buf;
    char current[20] = "";
    int found = 0;
    while (read(config_fd, &buf, 1)) {
        if (buf == '\n') {
            if (found) break;
            strcpy(current, "");
        } else if (buf == ' ') {
            if (strcmp(current, name) == 0) {
                found = 1;
                strcpy(current, "");
            } else {
                strcpy(current, "");
                // skip this line
                while (read(config_fd, &buf, 1))
                    if (buf == '\n') break;
            }
        } else
            strncat(current, &buf, 1);
    }

    close(config_fd);
    if (!found) {
        printf("Process not found in configuration file.\n");
        return -1;
    }

    return atoi(current);
}

void send_response(char command[]) {
    // Command, callback
    char words[2][100] = {"", ""};
    int index = 0;
    int error = 1;
    // Split the command into words
    for (int i = 0; i < strlen(command); i++) {
        if (command[i] == '"') {
            index++;
            continue;
        }
        strncat(words[index], &command[i], 1);
    }
    // printf("Command is: %s\n", words[0]);
    // printf("Response will be sent to: %s\n", words[1]);

    int outgoing_fd = open(words[1], O_WRONLY);
    if (outgoing_fd == -1) {
        perror("Opening response queue");
        return;
    }

    close(STDOUT_FILENO);
    dup(outgoing_fd);

    // execl("/bin/sh", "/bin/sh", "-c", words[0], NULL);
    system(words[0]);

    raise(SIGINT);
}

int await_response(char callback[]) {
    int f_status = mkfifo(callback, 0664);
    if (f_status == -1) {
        perror("Create fifo");
    }
    int fd = open(callback, O_RDONLY);
    if (fd == -1) perror("Opening response queue");
    char command_buf[200];
    int r_status = read(fd, &command_buf, sizeof(command_buf));
    if (r_status < 1) {
        printf("%d ", r_status);
        perror("Error reading");
    }
    printf("Response:\n%s\n\n", command_buf);

    unlink(callback);
    raise(SIGINT);
    return 0;
}

void send_request(char receiver[], char command[], char callback[]) {
    char receiver_key[30];
    sprintf(receiver_key, "%d", key_from_name(receiver));
    int outgoing_fd = open(receiver_key, O_WRONLY);
    if (outgoing_fd == -1) {
        printf("Wrong process name or process is not running!\n");
        return;
    }
    printf("Requesting '%s'\n\n", command);
    fflush(stdin);
    char sep = '"';
    strncat(command, &sep, 1);
    strcat(command, callback);
    // printf("Package is %s\n", command);
    if (!fork())
        await_response(callback);
    int n_bytes = write(outgoing_fd, command, (strlen(command) + 1));
    // printf("Sent %d bytes\n\n", n_bytes);
    raise(SIGINT);
}

int await_command() {
    while (1) {
        char command_buf[100];
        size_t len = sizeof(command_buf);
        scanf("%[^\n]%*c", command_buf);
        if (strcmp(command_buf, "") == 0) continue;
        if (strcmp(command_buf, "q") == 0) return 0;

        char words[3][100] = {"", "", ""};
        int index = 0;
        int quote = -1;
        int error = 0;
        // Split the command into words
        for (int i = 0; i < strlen(command_buf); i++) {
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
        if (index < 2) {
            printf("Too little arguments.\n");
            error = 1;
        }

        if (error) continue;

        if (!fork()) {
            send_request(words[0], words[1], words[2]);
        }
    }
}

void await_request(char key[]) {
    char incoming_msg[1000];
    int incoming_fd = open(key, O_RDONLY);
    while (1) {
        int r_status = read(incoming_fd, &incoming_msg, sizeof(incoming_msg));
        if (r_status == 0) continue;
        if (r_status == -1) {
            perror("Reading requests queue");
        }
        printf("Got request!\n");
        if (!fork())
            send_response(incoming_msg);
    }
}

int main(int argc, char* argv[]) {
    printf("Press 'q' to exit.\n");
    printf("Input commands as:\n$> [Process name] [command] [return queue name].\n");
    printf("Use \"[command]\" for multi-word comands\n\n");
    char my_key[30];
    // For testing
    if (argc == 1)
        strcpy(my_key, "1");
    else
        sprintf(my_key, "%d", key_from_name(argv[1]));
    // printf("%s\n", my_key);
    if (strcmp(my_key, "-1") == 0) return -1;

    mkfifo(my_key, 0664);

    // Create 'receiver' process
    int receiver_pid = fork();
    if (receiver_pid == 0)
        await_request(my_key);
    else
        await_command();

    printf("Cleaning up...\n");
    kill(receiver_pid, SIGINT);
    unlink(my_key);
    return 0;
}