#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/sha.h>
#include "../Common/utilities.h"

#define FILE_SIZE_BYTES 0.005 * 1024 * 1024
#define BUFFER_SIZE 1024

void generateFile(const char *filename)
{
    srand(time(NULL));

    // Large file generation.
    int fd = open(filename, O_CREAT | O_WRONLY, 0644);

    if (fd < 0)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int totalWritten = 0;

    while (totalWritten < FILE_SIZE_BYTES)
    {
        int randomNumber = rand() % 100 + 1;
        char buffer[12];
        int length = snprintf(buffer, sizeof(buffer), "%d ", randomNumber);

        int bytesWritten = write(fd, buffer, length);

        if (bytesWritten < 0)
        {
            perror("Error writing to file");
            close(fd);
            exit(EXIT_FAILURE);
        }

        totalWritten += bytesWritten;
    }

    close(fd);
    printf("Random integers file created.\n");
}

void connectToServer(const char *filename, char sortAlgo, char searchAlgo, char *keyToFind)
{
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SORT_SEARCH_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection Failed");
        close(sock);
        return;
    }

    snprintf(buffer, BUFFER_SIZE, "SORTSEARCH\n%s\n%c\n%c\n%s\n", filename, sortAlgo, searchAlgo, keyToFind);
    // printf("Sending buffer: %s\n", buffer);
    if (write(sock, buffer, strlen(buffer)) < 0)
    {
        perror("Error writing filename to socket");
        close(sock);
        return;
    }


    int keyFound;
    size_t arrSize;
    if (read(sock, &arrSize, sizeof(int)) < 0)
    {
        perror("Error reading array size from socket");
        close(sock);
        return;
    }

    printf("Read array size - %zu\n", arrSize);

    int *sortedArr = malloc(sizeof(int) * arrSize);
    if (!sortedArr)
    {
        perror("Error allocating memory for sorted array");
        close(sock);
        return;
    }

    if (read(sock, sortedArr, sizeof(int) * arrSize) < 0)
    {
        perror("Error reading sorted array from socket");
        free(sortedArr);
        close(sock);
        return;
    }

    write(sock, NULL, 0);

    if (read(sock, &keyFound, sizeof(int)) < 0)
    {
        perror("Error reading key index from socket");
        free(sortedArr);
        close(sock);
        return;
    }

    printf("The key - %d - is found at index: %d\n", atoi(keyToFind), keyFound);
    printf("Sorted array:\n");
    dispArray(sortedArr, arrSize);

    free(sortedArr);
    close(sock);
}

int main(void)
{
    char filename[] = "ClientFiles/clientFileXXXXXX"; // Template for mkstemp
    int fileFD = mkstemp(filename);
    if (fileFD < 0)
    {
        perror("Error creating temporary file");
        exit(EXIT_FAILURE);
    }
    close(fileFD);

    char sortAlgo;
    printf("Enter desired sort algorithm:\n1. Bubble Sort\n2. Selection Sort\n3. Merge Sort\n4. Quick Sort\n5. Anything\n ->");
    scanf(" %c", &sortAlgo);
    getchar(); // Consume newline character

    char searchAlgo;
    printf("Enter desired search algorithm:\n1. Linear Search\n2. Binary Search\n3. Jump Search\n4. Interpolation Search\n5. Anything\n ->");
    scanf(" %c", &searchAlgo);
    getchar(); // Consume newline character

    char keyToFind[5];
    printf("Enter desired key value to find: ");
    if (fgets(keyToFind, sizeof(keyToFind), stdin) == NULL)
    {
        perror("Error reading keyToFind");
        exit(EXIT_FAILURE);
    }
    keyToFind[strcspn(keyToFind, "\n")] = '\0'; // Remove newline character

    generateFile(filename);
    connectToServer(filename, sortAlgo, searchAlgo, keyToFind);

    return 0;
}
