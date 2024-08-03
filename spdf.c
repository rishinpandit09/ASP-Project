#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8081
#define BUFFER_SIZE 1024

void error(const char *msg)
{
    perror(msg);
    exit(1);
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

void prcclient(int newsockfd)
{
    char buffer[BUFFER_SIZE];
    ssize_t n;

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        n = read(newsockfd, buffer, BUFFER_SIZE - 1);
        if (n <= 0)
        {
            if (n < 0)
                error("ERROR reading from socket");
            break;
        }

        printf("Received command: %s\n", buffer);

        // Process command
        char *command = strtok(buffer, " ");
        if (strcmp(command, "ufile") == 0)
        {
            char *filename = strtok(NULL, " ");
            char *dest_path = strtok(NULL, " ");
            if (filename == NULL || dest_path == NULL)
            {
                printf("Invalid command\n");
                continue;
            }

            // Create destination path if it does not exist
            struct stat st = {0};
            if (stat(dest_path, &st) == -1)
            {
                mkdir(dest_path, 0700);
            }

            // Save .pdf file
            char filepath[BUFFER_SIZE];
            snprintf(filepath, sizeof(filepath), "%s/%s", dest_path, filename);
            receive_file(newsockfd, filepath);
            printf("Saved .pdf file: %s\n", filepath);
        }
        else if (strcmp(command, "dfile") == 0)
        {
            // Handle download request (if needed)
        }
        else if (strcmp(command, "rmfile") == 0)
        {
            // Handle remove request (if needed)
        }
        else if (strcmp(command, "dtar") == 0)
        {
            // Handle tar request (if needed)
        }
        else
        {
            printf("Unknown command\n");
        }
    }
    close(newsockfd);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pid_t pid;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    // Setup server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }

    // Listen for connections
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1)
    {
        // Accept connection
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }

        // Fork process to handle client
        pid = fork();
        if (pid < 0)
        {
            error("ERROR on fork");
        }
        if (pid == 0)
        {
            close(sockfd);
            prcclient(newsockfd);
            exit(0);
        }
        else
        {
            close(newsockfd);
        }
    }

    close(sockfd);
    return 0;
}
