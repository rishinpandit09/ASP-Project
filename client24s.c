#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void send_file(int sockfd, const char *filename)
{
    int file_fd;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    // Open file
    file_fd = open(filename, O_RDONLY);
    if (file_fd < 0)
    {
        error("ERROR opening file");
    }

    // Send file
    while ((n = read(file_fd, buffer, BUFFER_SIZE)) > 0)
    {
        if (write(sockfd, buffer, n) < 0)
        {
            error("ERROR writing to socket");
        }
    }

    close(file_fd);
}

void receive_file(int sockfd, const char *filename)
{
    int file_fd;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd < 0)
    {
        error("ERROR opening file");
    }

    while ((n = read(sockfd, buffer, BUFFER_SIZE)) > 0)
    {
        if (write(file_fd, buffer, n) < 0)
        {
            error("ERROR writing to file");
        }
    }

    close(file_fd);
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char filename[BUFFER_SIZE];
    char dest_path[BUFFER_SIZE];

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    // Setup server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting to server");
    }

    while (1)
    {
        printf("client24s$ ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character

        // Parse command
        strcpy(command, strtok(buffer, " "));
        if (strcmp(command, "ufile") == 0)
        {
            strcpy(filename, strtok(NULL, " "));
            strcpy(dest_path, strtok(NULL, " "));
            if (filename == NULL || dest_path == NULL)
            {
                printf("Invalid command\n");
                continue;
            }

            // Send command to server
            snprintf(buffer, BUFFER_SIZE, "ufile %s %s", filename, dest_path);
            if (write(sockfd, buffer, strlen(buffer)) < 0)
            {
                error("ERROR writing to socket");
            }

            // Send file to server
            send_file(sockfd, filename);
        }
        else if (strcmp(command, "dfile") == 0)
        {
            strcpy(filename, strtok(NULL, " "));
            if (filename == NULL)
            {
                printf("Invalid command\n");
                continue;
            }

            // Send command to server
            snprintf(buffer, BUFFER_SIZE, "dfile %s", filename);
            if (write(sockfd, buffer, strlen(buffer)) < 0)
            {
                error("ERROR writing to socket");
            }

            // Receive file from server
            receive_file(sockfd, filename);
        }
        else if (strcmp(command, "rmfile") == 0)
        {
            strcpy(filename, strtok(NULL, " "));
            if (filename == NULL)
            {
                printf("Invalid command\n");
                continue;
            }

            // Send command to server
            snprintf(buffer, BUFFER_SIZE, "rmfile %s", filename);
            if (write(sockfd, buffer, strlen(buffer)) < 0)
            {
                error("ERROR writing to socket");
            }
        }
        else if (strcmp(command, "dtar") == 0)
        {
            char *filetype = strtok(NULL, " ");
            if (filetype == NULL)
            {
                printf("Invalid command\n");
                continue;
            }

            // Send command to server
            snprintf(buffer, BUFFER_SIZE, "dtar %s", filetype);
            if (write(sockfd, buffer, strlen(buffer)) < 0)
            {
                error("ERROR writing to socket");
            }

            // Receive tar file from server
            char tar_filename[BUFFER_SIZE];
            snprintf(tar_filename, sizeof(tar_filename), "%s.tar", filetype + 1); // Remove '.' from filetype
            receive_file(sockfd, tar_filename);
        }
        else if (strcmp(command, "display") == 0)
        {
            char *pathname = strtok(NULL, " ");
            if (pathname == NULL)
            {
                printf("Invalid command\n");
                continue;
            }

            // Send command to server
            snprintf(buffer, BUFFER_SIZE, "display %s", pathname);
            if (write(sockfd, buffer, strlen(buffer)) < 0)
            {
                error("ERROR writing to socket");
            }

            // Receive and display file list from server
            receive_file(sockfd, "filelist.txt");
            system("cat filelist.txt");
        }
        else
        {
            printf("Unknown command\n");
        }
    }

    close(sockfd);
    return 0;
}
