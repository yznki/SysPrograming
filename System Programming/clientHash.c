#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/sha.h>
#include "../Common/utilities.h"

#define BUFFER_SIZE 1024

void sendFileForHash(const char *filename)
{
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    int file_fd;

    // Open the file
    file_fd = open(filename, O_RDONLY);
    if (file_fd < 0)
    {
        perror("Error opening file");
        return;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(HASH_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection Failed");
        close(sock);
        return;
    }

    // Send the filename first
    snprintf(buffer, BUFFER_SIZE, "HASH\n%s\n", filename);
    printf("Sending buffer: %s\n", buffer);
    if (write(sock, buffer, strlen(buffer)) < 0)
    {
        perror("Error writing filename to socket");
        close(file_fd);
        close(sock);
        return;
    }

    // Send the file contents
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0)
    {
        if (write(sock, buffer, bytes_read) < 0)
        {
            perror("Error writing file to socket");
            close(file_fd);
            close(sock);
            return;
        }
    }
    close(file_fd);
    shutdown(sock, SHUT_WR);

    // Receive the hash from the server
    int valread = read(sock, buffer, BUFFER_SIZE - 1);
    if (valread < 0)
    {
        perror("Error reading from socket");
        close(sock);
        return;
    }
    buffer[valread] = '\0'; // Null-terminate the string

    printf("Hash of your file: %s\n", buffer);

    close(sock);
}

int main()
{
    char filename[256];

    printf("Enter the filename: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL)
    {
        perror("Error reading filename");
        exit(EXIT_FAILURE);
    }

    // Remove newline character if present
    filename[strcspn(filename, "\n")] = '\0';

    sendFileForHash(filename);
    return 0;
}