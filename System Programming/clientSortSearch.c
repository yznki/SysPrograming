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
    printf("Sending buffer: %s", buffer);
    if (write(sock, buffer, strlen(buffer)) < 0)
    {
        perror("Error writing to socket");
        close(sock);
        return;
    }

    // Receive the size of the sorted array
    if (read(sock, &arraySize, sizeof(int)) < 0)
    {
        perror("Error reading array size from socket");
        close(sock);
        return;
    }
    printf("Received array size: %d\n", arraySize);

    // Allocate memory for the sorted array
    sortedArray = (int *)malloc(arraySize * sizeof(int));
    if (!sortedArray)
    {
        perror("Memory allocation error");
        close(sock);
        return;
    }

    // Receive the sorted array from the server
    if (read(sock, sortedArray, sizeof(int) * arraySize) < 0)
    {
        perror("Error reading array from socket");
        free(sortedArray);
        close(sock);
        return;
    }
    printf("Received sorted array from server\n");

    int fastestSort = 0, fastestSearch = 0;

    // Read Fastest Sort
    if (read(sock, &fastestSort, sizeof(int)) < 0)
    {
        perror("Error reading fastest sort algo from socket");
        free(sortedArray);
        close(sock);
        return;
    }
    printf("Received fastest sorting algorithm from server\n");

    // Receive the index of the key from the server
    if (read(sock, &index, sizeof(int)) < 0)
    {
        perror("Error reading index from socket");
        free(sortedArray);
        close(sock);
        return;
    }
    printf("Received search result index from server\n");

    // Read Fastest Search
    if (read(sock, &fastestSearch, sizeof(int)) < 0)
    {
        perror("Error reading fastest search algo from socket");
        free(sortedArray);
        close(sock);
        return;
    }
    printf("Received fastest searching algorithm from server\n");

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

    srand(time(NULL));

    char filename[256] = "numbers.txt";
    int sortAlgo, searchAlgo, keyToFind;
    int sizeInBytes;

    printf("Enter the size: ");
    scanf("%d", &sizeInBytes);

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