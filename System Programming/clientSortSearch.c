#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "../Common/utilities.h"

#define BUFFER_SIZE 1024

ssize_t readFull(int sockfd, void *buf, size_t len)
{
    size_t total = 0;
    ssize_t n;

    while (total < len)
    {
        n = read(sockfd, buf + total, len - total);
        if (n <= 0)
            return n;
        total += n;
    }
    return total;
}

int readDataInChunks(int fd, void *data, size_t dataSize)
{
    size_t totalRead = 0;
    size_t chunkSize = 4096; // 4KB chunks
    while (totalRead < dataSize)
    {
        size_t bytesToRead = (dataSize - totalRead > chunkSize) ? chunkSize : (dataSize - totalRead);
        ssize_t readBytes = read(fd, (char *)data + totalRead, bytesToRead);
        if (readBytes < 0)
        {
            perror("read");
            return -1;
        }
        totalRead += readBytes;
    }
    return 0;
}

void sendFileForSortSearch(const char *filename, int sortAlgo, int searchAlgo, int keyToFind)
{
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    int *sortedArray;
    int arraySize;
    int index;

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

    // Send the sort and search command
    snprintf(buffer, BUFFER_SIZE, "SORTSEARCH\n%s\n%d\n%d\n%d\n", filename, sortAlgo, searchAlgo, keyToFind);
    // printf("Sending buffer: %s", buffer);
    if (write(sock, buffer, strlen(buffer)) < 0)
    {
        perror("Error writing to socket");
        close(sock);
        return;
    }



    // Receive the size of the sorted array
    if (readFull(sock, &arraySize, sizeof(int)) < 0)
    {
        perror("Error reading array size from socket");
        close(sock);
        return;
    }

    // Allocate memory for the sorted array
    sortedArray = (int *)malloc(arraySize * sizeof(int));
    if (!sortedArray)
    {
        perror("Memory allocation error");
        close(sock);
        return;
    }

    // Receive the sorted array from the server
    if (readDataInChunks(sock, sortedArray, sizeof(int) * arraySize) < 0)
    {
        perror("Error reading array from socket");
        free(sortedArray);
        close(sock);
        return;
    }

    int fastestSort = 0, fastestSearch = 0;

    // Read Fastest Sort
    if (readFull(sock, &fastestSort, sizeof(int)) < 0)
    {
        perror("Error reading fastest sort algo from socket");
        free(sortedArray);
        close(sock);
        return;
    }

    // Receive the index of the key from the server
    if (readFull(sock, &index, sizeof(int)) < 0)
    {
        perror("Error reading index from socket");
        free(sortedArray);
        close(sock);
        return;
    }

    // Read Fastest Search
    if (readFull(sock, &fastestSearch, sizeof(int)) < 0)
    {
        perror("Error reading fastest search algo from socket");
        free(sortedArray);
        close(sock);
        return;
    }

    printf("Sorted array:\n");
    dispArray(sortedArray, arraySize);

    if (index >= 0)
    {
        printf("The key %d is found at index %d\n", keyToFind, index);
    }
    else
    {
        printf("The key %d is not found in the array\n", keyToFind);
    }

    if (sortAlgo == 5)
    {
        switch (fastestSort)
        {
        case 1:
            printf("Bubble Sort was the Fastest\n");
            break;
        case 2:
            printf("Selection Sort was the Fastest\n");
            break;
        case 3:
            printf("Merge Sort was the Fastest\n");
            break;
        case 4:
            printf("Quick Sort was the Fastest\n");
            break;
        default:
            perror("Fastest Algo Error.\n");
            break;
        }
    }

    if (searchAlgo == 5)
    {
        switch (fastestSearch)
        {
        case 1:
            printf("Linear Search was the Fastest\n");
            break;
        case 2:
            printf("Binary Search was the Fastest\n");
            break;
        case 3:
            printf("Jump Search was the Fastest\n");
            break;
        case 4:
            printf("Interpolation Search was the Fastest\n");
            break;
        default:
            perror("Fastest Algo Error.\n");
            break;
        }
    }

    free(sortedArray);
    close(sock);
    printf("Client finished processing.\n");
}

void createFileWithRandomIntegers(const char *filename, size_t sizeInBytes)
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        perror("File open error");
        exit(EXIT_FAILURE);
    }

    size_t numIntegers = sizeInBytes / sizeof(int);
    int *array = (int *)malloc(FILE_CHUNK_SIZE * sizeof(int));
    if (!array)
    {
        perror("Memory allocation error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t integersWritten = 0;
    while (integersWritten < numIntegers)
    {
        size_t chunkSize = (numIntegers - integersWritten > FILE_CHUNK_SIZE) ? FILE_CHUNK_SIZE : (numIntegers - integersWritten);
        for (size_t i = 0; i < chunkSize; i++)
        {
            array[i] = rand() % 100 + 1; // Generate random integers
        }

        if (write(fd, array, chunkSize * sizeof(int)) != chunkSize * sizeof(int))
        {
            perror("File write error");
            free(array);
            close(fd);
            exit(EXIT_FAILURE);
        }

        integersWritten += chunkSize;
    }

    free(array);
    close(fd);
}

int main()
{

    // srand(time(NULL));

    char filename[256] = "numbers.bin";
    int sortAlgo, searchAlgo, keyToFind;
    int sizeInBytes;

    printf("Enter the size (maximum 65000): ");
    scanf("%d", &sizeInBytes);

    if (sizeInBytes > 65000)
    {
        printf("Too large of a size.\n");
        return 0;
    }

    createFileWithRandomIntegers(filename, sizeInBytes);

    printf("Enter the sorting algorithm (1: Bubble, 2: Selection, 3: Merge, 4: Quick, 5: Any): ");
    scanf("%d", &sortAlgo);

    printf("Enter the searching algorithm (1: Linear, 2: Binary, 3: Jump, 4: Interpolation, 5: Any): ");
    scanf("%d", &searchAlgo);

    printf("Enter the key to find: ");
    scanf("%d", &keyToFind);

    sendFileForSortSearch(filename, sortAlgo, searchAlgo, keyToFind);

    return 0;
}