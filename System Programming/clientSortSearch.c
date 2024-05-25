#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/sha.h>
#include "../Common/utilities.h"

#define FILE_SIZE_BYTES 0.0005 * 1024 * 1024
#define BUFFER_SIZE 1024

void generateFile(const char *filename) {

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

        totalWritten += bytesWritten;
        printf("Total Written: %d\n", totalWritten);
    }

    close(fd);
    printf("\nRandom integers file created.\n\n");



}

void connectToServer(const char *filename, char sortAlgo, char searchAlgo, char *keyToFind) {
    int sock;
    struct sockaddr_in serv_addr;
    int fd;
    char buffer[BUFFER_SIZE];

    // Open the file
    fd = open(filename, O_RDONLY);
    if (fd < 0)
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
    serv_addr.sin_port = htons(SORT_SEARCH_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection Failed");
        close(sock);
        return;
    }

    // Send the filename first
    snprintf(buffer, BUFFER_SIZE, "SORTSEARCH\n%s\n%c\n%c\n%s\n", filename, sortAlgo, searchAlgo, keyToFind);
    printf("\nSending buffer: %s\n", buffer);
    if (write(sock, buffer, strlen(buffer)) < 0)
    {
        perror("Error writing filename to socket");
        close(fd);
        close(sock);
        return;
    }

    // Send the file contents

    // ssize_t bytesRead;
    // while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0)
    // {
    //     if (write(sock, buffer, bytesRead) < 0)
    //     {
    //         perror("Error writing file to socket");
    //         close(fd);
    //         close(sock);
    //         return;
    //     }
    //     printf("Written %zd bytes to server\n", bytesRead);
    // }

    close(fd);

    // Sort Search Logic

    int keyFound, arrSize;
    read(sock, &arrSize, sizeof(int));
    write(sock, NULL, 0);
    int *sortedArr = malloc(sizeof(int) * arrSize);
    read(sock, sortedArr, sizeof(int) * arrSize);
    write(sock, NULL, 0);
    read(sock, &keyFound, sizeof(int));

    printf("The key - %d - is found at index: %d\n", atoi(keyToFind), keyFound);

    printf("Sorted array\n");

    dispArray(sortedArr, arrSize);

    close(sock);
};

int main(void)
{
    // const char *filename = "500mb_file.txt";

    char filename[256];

    printf("Enter the filename: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL)
    {
        perror("Error reading filename");
        exit(EXIT_FAILURE);
    }

    // Remove newline character if present
    filename[strcspn(filename, "\n")] = '\0';

    char destFile[256] = "./TestingFiles/";
    strcat(destFile, filename);
    
    char sortAlgo;
    printf("Enter desired sort algorithm:\n1. Bubble Sort\n2. Selection Sort\n3. Merge Sort\n4. Quick Sort\n5. Anything\n");
    scanf(" %c", &sortAlgo);
    char searchAlgo;
    printf("Enter desired search algorithm:\n1. Linear Search\n2. Binary Search\n3. Jump Search\n4. Interpolation Search\n5. Anything\n");
    scanf(" %c", &searchAlgo);

    getchar();

    char keyToFind[5];
    printf("Enter desired key value to find: ");
    if (fgets(keyToFind, sizeof(keyToFind), stdin) == NULL)
    {
        perror("Error reading keyToFind");
        exit(EXIT_FAILURE);
    }

    keyToFind[strcspn(keyToFind, "\n")] = '\0';

    generateFile(destFile);
    connectToServer(destFile, sortAlgo, searchAlgo, keyToFind);

    return 0;
}
