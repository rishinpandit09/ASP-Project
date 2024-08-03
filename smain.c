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

#define PORT 8080
#define BUFFER_SIZE 1024

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void forward_file(const char *filename, const char *server_ip, int server_port)
{
    int sockfd, file_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    // Open file
    file_fd = open(filename, O_RDONLY);
    if (file_fd < 0)
    {
        error("ERROR opening file");
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        error("ERROR connecting to server");
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
    close(sockfd);
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

        // Parse command and process accordingly
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

            char *filetype = strrchr(filename, '.');
            if (filetype == NULL)
            {
                printf("Invalid file type\n");
                continue;
            }

            // Create destination path if it does not exist
            struct stat st = {0};
            if (stat(dest_path, &st) == -1)
            {
                mkdir(dest_path, 0700);
            }

            if (strcmp(filetype, ".c") == 0)
            {
                // Save .c file locally
                char filepath[BUFFER_SIZE];
                snprintf(filepath, sizeof(filepath), "%s/%s", dest_path, filename);
                receive_file(newsockfd, filepath);
                printf("Saved .c file locally: %s\n", filepath);
            }
            else if (strcmp(filetype, ".pdf") == 0)
            {
                // Forward .pdf file to Spdf
                forward_file(filename, "127.0.0.1", 8081); // Replace with Spdf server IP and port
                printf("Forwarded .pdf file to Spdf: %s\n", filename);
            }
            else if (strcmp(filetype, ".txt") == 0)
            {
                // Forward .txt file to Stext
                forward_file(filename, "127.0.0.1", 8082); // Replace with Stext server IP and port
                printf("Forwarded .txt file to Stext: %s\n", filename);
            }
            else
            {
                printf("Unsupported file type\n");
            }
        }
        else if (strcmp(command, "dfile") == 0)
        {
            char *filepath = strtok(NULL, " ");
            if (filepath == NULL)
            {
                printf("Invalid command\n");
                continue;
            }

            char *filetype = strrchr(filepath, '.');
            if (filetype == NULL)
            {
                printf("Invalid file type\n");
                continue;
            }

            if (strcmp(filetype, ".c") == 0)
            {
                // Send .c file to client
                forward_file(filepath, "127.0.0.1", 8080); // Assume Smain serves files itself
                printf("Sent .c file to client: %s\n", filepath);
            }
            else if (strcmp(filetype, ".pdf") == 0)
            {
                // Request .pdf file from Spdf
                forward_file(filepath, "127.0.0.1", 8081); // Replace with Spdf server IP and port
                printf("Requested .pdf file from Spdf: %s\n", filepath);
            }
            else if (strcmp(filetype, ".txt") == 0)
            {
                // Request .txt file from Stext
                forward_file(filepath, "127.0.0.1", 8082); // Replace with Stext server IP and port
                printf("Requested .txt file from Stext: %s\n", filepath);
            }
            else
            {
                printf("Unsupported file type\n");
            }
        }
        else if (strcmp(command, "rmfile") == 0)
        {
            char *filepath = strtok(NULL, " ");
            if (filepath == NULL)
            {
                printf("Invalid command\n");
                continue;
            }

            char *filetype = strrchr(filepath, '.');
            if (filetype == NULL)
            {
                printf("Invalid file type\n");
                continue;
            }

            if (strcmp(filetype, ".c") == 0)
            {
                // Remove .c file
                if (remove(filepath) == 0)
                {
                    printf("Deleted .c file: %s\n", filepath);
                }
                else
                {
                    perror("ERROR deleting file");
                }
            }
            else if (strcmp(filetype, ".pdf") == 0)
            {
                // Request Spdf to delete .pdf file
                forward_file(filepath, "127.0.0.1", 8081); // Replace with Spdf server IP and port
                printf("Requested Spdf to delete .pdf file: %s\n", filepath);
            }
            else if (strcmp(filetype, ".txt") == 0)
            {
                // Request Stext to delete .txt file
                forward_file(filepath, "127.0.0.1", 8082); // Replace with Stext server IP and port
                printf("Requested Stext to delete .txt file: %s\n", filepath);
            }
            else
            {
                printf("Unsupported file type\n");
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

            if (strcmp(filetype, ".c") == 0)
            {
                // Create tar of all .c files and send to client
                system("tar -cvf cfiles.tar ~/smain/*.c");
                forward_file("cfiles.tar", "127.0.0.1", 8080); // Assume Smain serves files itself
                printf("Sent cfiles.tar to client\n");
            }
            else if (strcmp(filetype, ".pdf") == 0)
            {
                // Request pdf.tar from Spdf and send to client
                forward_file("pdf.tar", "127.0.0.1", 8081); // Replace with Spdf server IP and port
                printf("Requested pdf.tar from Spdf and sent to client\n");
            }
            else if (strcmp(filetype, ".txt") == 0)
            {
                // Request text.tar from Stext and send to client
                forward_file("text.tar", "127.0.0.1", 8082); // Replace with Stext server IP and port
                printf("Requested text.tar from Stext and sent to client\n");
            }
            else
            {
                printf("Unsupported file type\n");
            }
        }
        else if (strcmp(command, "display") == 0)
        {
            char *pathname = strtok(NULL, " ");
            if (pathname == NULL)
            {
                printf("Invalid command\n");
                continue;
            }

            // Get list of .c, .pdf, and .txt files and send to client
            char list_command[BUFFER_SIZE];
            snprintf(list_command, sizeof(list_command), "ls %s/*.c %s/*.pdf %s/*.txt", pathname, pathname, pathname);
            system(list_command);
            forward_file("filelist.txt", "127.0.0.1", 8080); // Assume Smain serves files itself
            printf("Sent file list to client\n");
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
