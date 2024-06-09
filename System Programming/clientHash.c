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
#define EOF_MARKER "EOF\n"

void handleFileSending(const char *filename, int sock)
{
    ssize_t bytesRead;
    char data[BUFFER_SIZE];

    int fp = open(filename, O_RDONLY);
    if (fp == -1)
    {
        perror("[-]Error in opening file.");
        exit(1);
    }

    while ((bytesRead = read(fp, data, BUFFER_SIZE)) > 0)
    {
        if (send(sock, data, bytesRead, 0) == -1)
        {
            perror("[-]Error in sending file.");
            close(fp);
            exit(1);
        }
    }

    if (bytesRead == -1)
    {
        perror("[-]Error in reading file.");
    }

    // Send EOF marker separately
    if (send(sock, EOF_MARKER, strlen(EOF_MARKER), 0) == -1)
    {
        perror("[-]Error in sending EOF marker.");
        close(fp);
        exit(1);
    }

    // Small delay to ensure EOF is processed separately
    usleep(1000);

    close(fp);
}

void sendFileForHash(const char *filename)
{
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

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

    // Send the file first
    handleFileSending(filename, sock);

    // Wait for server acknowledgment of EOF marker
    int valread = read(sock, buffer, BUFFER_SIZE - 1);
    if (valread < 0)
    {
        perror("Error reading from socket");
        close(sock);
        return;
    }
    buffer[valread] = '\0'; // Null-terminate the string
    if (strcmp(buffer, "EOF_ACK") != 0)
    {
        fprintf(stderr, "Did not receive EOF_ACK\n");
        close(sock);
        return;
    }

    // Send the hashing command
    snprintf(buffer, BUFFER_SIZE, "HASH\n%s\n", filename);
    printf("Sending buffer: %s\n", buffer);
    if (write(sock, buffer, strlen(buffer)) < 0)
    {
        perror("Error writing filename to socket");
        close(sock);
        return;
    }

    // Receive the hash from the server
    valread = read(sock, buffer, BUFFER_SIZE - 1);
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
    char filename[256] = "sample.txt";

    // printf("Enter the filename: ");
    // if (fgets(filename, sizeof(filename), stdin) == NULL)
    // {
    //     perror("Error reading filename");
    //     exit(EXIT_FAILURE);
    // }

    // // Remove newline character if present
    // filename[strcspn(filename, "\n")] = '\0';

    sendFileForHash(filename);
    return 0;
}
