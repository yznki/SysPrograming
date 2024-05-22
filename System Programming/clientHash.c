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

void send_file_for_hash(const char *filename)
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

    // Send the request type and filename in one go
    snprintf(buffer, BUFFER_SIZE, "hash\n%s\n", filename);
    if (write(sock, buffer, strlen(buffer)) < 0)
    {
        perror("Error writing request to socket");
        close(sock);
        return;
    }
    printf("Sent request: %s\n", buffer); // Debug statement

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

    send_file_for_hash(filename);
    return 0;
}