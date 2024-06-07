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
#include <sys/stat.h>
#include <openssl/sha.h>
#include "../Common/searchSortAlgos.h"
#include "../Common/utilities.h"

#define CACHE_SIZE 256
#define BUFFER_SIZE 1024

typedef struct
{
    char filename[256];
    char hash[SHA256_DIGEST_LENGTH * 2 + 1];

} HashCache;

HashCache hashCache[CACHE_SIZE];
int hashCacheSize = 0;
pthread_mutex_t cacheLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hashLock = PTHREAD_MUTEX_INITIALIZER;

int findHash(const char *filename, char *hash)
{
    pthread_mutex_lock(&cacheLock);
    for (int i = 0; i < hashCacheSize; i++)
    {
        if (strcmp(hashCache[i].filename, filename) == 0)
        {
            strcpy(hash, hashCache[i].hash);
            pthread_mutex_unlock(&cacheLock);
            return 1;
        }
    }
    pthread_mutex_unlock(&cacheLock);
    return 0;
}

void addHash(const char *filename, const char *hash)
{
    pthread_mutex_lock(&cacheLock);
    if (hashCacheSize < CACHE_SIZE)
    {
        strcpy(hashCache[hashCacheSize].filename, filename);
        strcpy(hashCache[hashCacheSize].hash, hash);
        hashCacheSize++;
    }
    else
    {
        // overwrite hash
        strcpy(hashCache[0].filename, filename);
        strcpy(hashCache[0].hash, hash);
        hashCacheSize = 1;
    }
    pthread_mutex_unlock(&cacheLock);
}

void computeHash(const char *filename, char *hash)
{
    char dest_file[] = "hashOutput.txt";
    pid_t pid;

    pthread_mutex_lock(&hashLock);

    pid = fork();

    if (pid == 0)
    {
        // Child process
        char *args[] = {"../Common/hashing.o", (char *)filename, dest_file, NULL};
        execv(args[0], args);

        perror("execv");
        exit(EXIT_FAILURE); // Exits and does not return because it's a main service, if it's down we have to close the server.
    }
    else if (pid > 0)
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) == 0)
            {
                int destinationFD = open(dest_file, O_RDONLY);
                if (destinationFD < 0)
                {
                    perror("open dest_file");
                    strcpy(hash, "ERROR: FILE NOT FOUND");
                    pthread_mutex_unlock(&hashLock);
                    return;
                }

                int bytesRead = read(destinationFD, hash, SHA256_DIGEST_LENGTH * 2);
                if (bytesRead < 0)
                {
                    perror("read");
                    strcpy(hash, "ERROR: FILE READ ERROR");
                    close(destinationFD);
                    pthread_mutex_unlock(&hashLock);
                    return;
                }
                hash[bytesRead] = '\0';
                close(destinationFD);
            }
            else if (WEXITSTATUS(status) == 1)
            {
                strcpy(hash, "ERROR: FILE NOT FOUND");
            }
            else
            {
                strcpy(hash, "ERROR: HASH COMPUTATION FAILED");
            }
        }
        else
        {
            strcpy(hash, "ERROR: HASH COMPUTATION FAILED");
        }
        remove("hash_output.txt");
    }
    else
    {
        perror("fork");
        strcpy(hash, "ERROR: FORK FAILED");
    }
    pthread_mutex_unlock(&hashLock);
}

void handleHashRequest(int clientSocket, const char *filename)
{
    char hash[SHA256_DIGEST_LENGTH * 2 + 1];

    if (findHash(filename, hash))
    {
        // Send the cached hash to the client
        write(clientSocket, hash, strlen(hash));
        printf("Hash found in cache.\n");
    }
    else
    {
        // Compute the hash and send it to the client
        computeHash(filename, hash);
        addHash(filename, hash);
        write(clientSocket, hash, strlen(hash));
        printf("Hash computed and saved.\n");
    }
}

void handleSortSearchRequest(const char *filename, int clientSocket, int sortAlgo, int searchAlgo, int keyToFind)
{
    int fileFD;
    char *token;
    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];
    size_t current_size = 1024; // Initial Size to start buffering
    size_t arrSize = 0;
    int *array = NULL;

    fileFD = open(filename, O_RDONLY);
    if (fileFD < 0)
    {
        perror("Error opening file");
        return;
    }

    array = (int *)malloc(current_size * sizeof(int));
    if (!array)
    {
        perror("Error allocating memory for array");
        close(fileFD);
        return;
    }
    // Read the file contents and parse integers
    while ((bytes_read = read(fileFD, buffer, BUFFER_SIZE - 1)) > 0)
    {
        buffer[bytes_read] = '\0'; // Null-terminate the buffer
        token = strtok(buffer, " \n");
        while (token != NULL)
        {
            if (arrSize >= current_size)
            {
                current_size *= 2;
                array = (int *)realloc(array, current_size * sizeof(int));
                if (!array)
                {
                    perror("Error reallocating memory for array");
                    close(fileFD);
                    return;
                }
            }
            array[arrSize++] = atoi(token);
            token = strtok(NULL, " \n");
        }
    }

    if (bytes_read < 0)
    {
        perror("Error reading file");
        free(array);
        close(fileFD);
        return;
    }

    close(fileFD);
    dispArray(array, arrSize);
    write(clientSocket, &arrSize, sizeof(int));
    read(clientSocket, NULL, 0);

    int fd[2];
    if (pipe(fd))
    {
        perror("Unable to create pipe...\n");
        return;
    }

    int pid = fork();

    if (pid == 0)
    {
        // Sorting
        switch (sortAlgo)
        {
        case 1:
            bubbleSort(array, arrSize);
            write(fd[1], array, sizeof(int) * arrSize);
            break;
        case 2:
            selectionSort(array, arrSize);
            write(fd[1], array, sizeof(int) * arrSize);
            break;
        case 3:
            mergeSort(array, 0, arrSize - 1);
            write(fd[1], array, sizeof(int) * arrSize);
            break;
        case 4:
            quickSort(array, 0, arrSize - 1);
            write(fd[1], array, sizeof(int) * arrSize);
            break;
        case 5:
            // Code for multithreading of all sorting algos together.
            break;

        default:
            break;
        }
        write(clientSocket, array, sizeof(int) * arrSize);
        printf("Sorted Array\n");
        dispArray(array, arrSize);
    }
    else if (pid > 0)
    {
        // Searching
        int *sortedArray = malloc(sizeof(int) * arrSize);
        read(fd[0], sortedArray, sizeof(int) * arrSize);
        read(clientSocket, NULL, 0);

        int indexOfKey;

        switch (searchAlgo)
        {
        case 1:
            indexOfKey = linearSearch(array, arrSize, keyToFind);
            write(clientSocket, &indexOfKey, sizeof(int));
            break;
        case 2:
            wait(NULL);
            indexOfKey = binarySearch(sortedArray, arrSize, keyToFind);
            write(clientSocket, &indexOfKey, sizeof(int));
            break;
        case 3:
            wait(NULL);
            indexOfKey = jumpSearch(sortedArray, arrSize, keyToFind);
            write(clientSocket, &indexOfKey, sizeof(int));
            break;
        case 4:
            wait(NULL);
            indexOfKey = interpolationSearch(sortedArray, arrSize, keyToFind);
            write(clientSocket, &indexOfKey, sizeof(int));
            break;
        case 5:
            wait(NULL);
            // Code for multithreading of all searching algos together.
            break;

        default:
            break;
        }

        printf("Index at %d\n", indexOfKey);
    }
    else
    {
        perror("Unable to create child process\n");
        return;
    }
}

void *handleClient(void *args)
{
    int clientSocket = *((int *)args);
    free(args);
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    // Read the request from the client
    bytesRead = read(clientSocket, buffer, BUFFER_SIZE - 1);
    if (bytesRead < 0)
    {
        perror("Error reading from socket");
        close(clientSocket);
        return NULL;
    }
    buffer[bytesRead] = '\0'; // Null-terminate the buffer

    // Parse the command keyword
    char *command = strtok(buffer, "\n");
    char *filename = strtok(NULL, "\n");
    char *sortAlgo = strtok(NULL, "\n");
    char *searchAlgo = strtok(NULL, "\n");
    char *key = strtok(NULL, "\n");

    printf("Command received: %s\n", command);
    printf("Filename received: %s\n", filename);

    if (command == NULL)
    {
        fprintf(stderr, "Invalid request format\n");
        close(clientSocket);
        return NULL;
    }

    // Handle the command
    if (strcmp(command, "HASH") == 0)
    {
        const char *baseDir = "./ClientHashedFiles/";
        char clientFileName[512];
        snprintf(clientFileName, sizeof(clientFileName), "%s%s", baseDir, filename);
        printf("New Client File Name: %s\n", clientFileName);

        // Ensure the directory exists
        struct stat st = {0};
        if (stat(baseDir, &st) == -1)
        {
            if (mkdir(baseDir, 0700) < 0)
            {
                perror("Error creating directory");
                close(clientSocket);
                return NULL;
            }
        }

        int fileFD = open(clientFileName, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fileFD < 0)
        {
            perror("Error opening file");
            close(clientSocket);
            return NULL;
        }

        // Read the file contents from the client and write to a file to save on server
        while ((bytesRead = read(clientSocket, buffer, BUFFER_SIZE)) > 0)
        {
            printf("Read %zd bytes from client\n", bytesRead);
            if (write(fileFD, buffer, bytesRead) < 0)
            {
                perror("Error writing to file");
                close(fileFD);
                close(clientSocket);
                return NULL;
            }
        }

        if (bytesRead < 0)
        {
            perror("Error reading from socket");
        }
        else
        {
            printf("Finished reading from client\n");
        }

        close(fileFD);

        handleHashRequest(clientSocket, filename);
    }
    else if (strcmp(command, "SORTSEARCH") == 0)
    {
        char fileName[] = "ClientFiles/clientFileXXXXXX"; // Template for mkstemp
        int fileFD = mkstemp(fileName);

        // Read the file contents from the client and write to the temporary file
        while ((bytesRead = read(clientSocket, buffer, BUFFER_SIZE)) > 0)
        {
            printf("Read %zd bytes from client\n", bytesRead);
            if (write(fileFD, buffer, bytesRead) < 0)
            {
                perror("Error writing to file");
                close(fileFD);
                close(clientSocket);
                return NULL;
            }
        }

        if (bytesRead < 0)
        {
            perror("Error reading from socket");
        }
        else
        {
            printf("Finished reading from client\n");
        }

        handleSortSearchRequest(filename, clientSocket, atoi(sortAlgo), atoi(searchAlgo), atoi(key));
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", command);
    }

    // Close the client socket
    close(clientSocket);

    return NULL;
}

void runServer(int port)
{
    int serverFD;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFD, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(serverFD);
        exit(EXIT_FAILURE);
    }

    if (listen(serverFD, 3) < 0)
    {
        perror("listen failed");
        close(serverFD);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", port);

    while (1)
    {
        int *newSocket = malloc(sizeof(int));
        if ((*newSocket = accept(serverFD, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            free(newSocket);
            continue;
        }

        printf("Accepted new connection\n");

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handleClient, (void *)newSocket) != 0)
        {
            perror("pthread_create");
            free(newSocket);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(serverFD);
}

int main(void)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process - Run the sort/search server
        runServer(SORT_SEARCH_PORT);
    }
    else if (pid > 0)
    {
        // Parent process - Run the hash server
        runServer(HASH_PORT);
    }

    return 0;
}
