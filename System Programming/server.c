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
#define BUFFER_SIZE 4096
#define FILE_CHUNK_SIZE 4096

typedef struct
{
    char filename[256];
    char hash[SHA256_DIGEST_LENGTH * 2 + 1];
    time_t mtime; // Last modification time
} HashCache;

HashCache hashCache[CACHE_SIZE];
int hashCacheSize = 0;
pthread_mutex_t cacheLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hashLock = PTHREAD_MUTEX_INITIALIZER;

// Function Declarations
int findHash(const char *filename, char *hash, time_t *mtime);
void addHash(const char *filename, const char *hash, time_t mtime);
void computeHash(const char *filename, char *hash, time_t *mtime);
void handleFileTransfer(int clientSocket);
void handleHashRequest(int clientSocket, const char *filename);
void handleSortSearchRequest(const char *filename, int clientSocket, int sortAlgo, int searchAlgo, int keyToFind);
void *handleClient(void *args);
void runServer(int port);
void sortArray(int *array, int size, int algorithm);
int searchArray(int *array, int size, int algorithm, int target);
void readArrayFromFile(const char *fileName, int **array, int *arraySize);

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
    else
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}

// Find a hash in the cache
int findHash(const char *filename, char *hash, time_t *mtime)
{
    pthread_mutex_lock(&cacheLock);
    for (int i = 0; i < hashCacheSize; i++)
    {
        if (strcmp(hashCache[i].filename, filename) == 0 && hashCache[i].mtime == *mtime)
        {
            strcpy(hash, hashCache[i].hash);
            *mtime = hashCache[i].mtime;
            pthread_mutex_unlock(&cacheLock);
            return 1;
        }
    }
    pthread_mutex_unlock(&cacheLock);
    return 0;
}

// Add a new hash to the cache
void addHash(const char *filename, const char *hash, time_t mtime)
{
    pthread_mutex_lock(&cacheLock);
    if (hashCacheSize < CACHE_SIZE)
    {
        strcpy(hashCache[hashCacheSize].filename, filename);
        strcpy(hashCache[hashCacheSize].hash, hash);
        hashCache[hashCacheSize].mtime = mtime;
        hashCacheSize++;
    }
    else
    {
        // Overwrite the oldest hash
        strcpy(hashCache[0].filename, filename);
        strcpy(hashCache[0].hash, hash);
        hashCache[0].mtime = mtime;
        hashCacheSize = 1;
    }
    pthread_mutex_unlock(&cacheLock);
}

// Compute the hash for a file
void computeHash(const char *filename, char *hash, time_t *mtime)
{
    char hash_file[] = "hashOutput.txt";
    char mtime_file[] = "mtimeOutput.txt";
    pid_t pid;

    pthread_mutex_lock(&hashLock);
    pid = fork();

    if (pid == 0)
    {
        // Child process
        char *args[] = {"./hash.sh", (char *)filename, hash_file, mtime_file, NULL};
        execv(args[0], args);
        perror("execv");
        exit(EXIT_FAILURE); // Exit if execv fails
    }
    else if (pid > 0)
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            // Read the hash
            int hashFD = open(hash_file, O_RDONLY);
            if (hashFD < 0)
            {
                perror("open hash_file");
                strcpy(hash, "ERROR: FILE NOT FOUND");
            }
            else
            {
                int bytesRead = read(hashFD, hash, SHA256_DIGEST_LENGTH * 2);
                if (bytesRead < 0)
                {
                    perror("read");
                    strcpy(hash, "ERROR: FILE READ ERROR");
                }
                else
                {
                    hash[bytesRead] = '\0';
                }
                close(hashFD);
            }

            // Read the modification time
            int mtimeFD = open(mtime_file, O_RDONLY);
            if (mtimeFD < 0)
            {
                perror("open mtime_file");
                *mtime = 0;
            }
            else
            {
                char mtime_str[20];
                int bytesRead = read(mtimeFD, mtime_str, sizeof(mtime_str) - 1);
                if (bytesRead < 0)
                {
                    perror("read");
                    *mtime = 0;
                }
                else
                {
                    mtime_str[bytesRead] = '\0';
                    *mtime = atol(mtime_str);
                }
                close(mtimeFD);
            }
        }
        else
        {
            strcpy(hash, "ERROR: HASH COMPUTATION FAILED");
            *mtime = 0;
        }
        remove(hash_file);
        remove(mtime_file);
    }
    else
    {
        perror("fork");
        strcpy(hash, "ERROR: FORK FAILED");
        *mtime = 0;
    }
    pthread_mutex_unlock(&hashLock);
}

// Handle hash request from client
void handleHashRequest(int clientSocket, const char *filename)
{
    char hash[SHA256_DIGEST_LENGTH * 2 + 1];
    time_t mtime;

    // Compute the current hash and modification time
    computeHash(filename, hash, &mtime);

    if (findHash(filename, hash, &mtime))
    {
        // Send the cached hash to the client
        write(clientSocket, hash, strlen(hash));
        printf("Hash found in cache.\n");
    }
    else
    {
        // Add the new hash to the cache and send it to the client
        addHash(filename, hash, mtime);
        write(clientSocket, hash, strlen(hash));
        printf("Hash computed and saved.\n");
    }
}

void handleSortSearchRequest(const char *filename, int clientSocket, int sortAlgo, int searchAlgo, int keyToFind)
{

    int index;
    int *array;
    int arraySize;

    // Read the array from the file
    readArrayFromFile(filename, &array, &arraySize);

    int sortPipe[2], searchPipe[2];
    if (pipe(sortPipe) == -1 || pipe(searchPipe) == -1)
    {
        perror("Pipe failed");
        close(clientSocket);
        return NULL;
    }

    pid_t sortPid = fork();
    if (sortPid == 0)
    { // Sorting process
        close(sortPipe[0]);
        sortArray(array, arraySize, sortAlgo);
        write(sortPipe[1], array, arraySize * sizeof(int));
        close(sortPipe[1]);
        exit(0);
    }

    pid_t searchPid = fork();
    if (searchPid == 0)
    { // Searching process
        close(searchPipe[0]);
        if (searchAlgo == 1)
        { // Linear search can happen immediately
            index = searchArray(array, arraySize, searchAlgo, keyToFind);
        }
        else
        { // Other search algorithms wait for sorting to finish
            int *sortedArray = (int *)malloc(arraySize * sizeof(int));
            read(sortPipe[0], sortedArray, arraySize * sizeof(int));
            close(sortPipe[0]);
            index = searchArray(sortedArray, arraySize, searchAlgo, keyToFind);
            free(sortedArray);
        }
        write(searchPipe[1], &index, sizeof(index));
        close(searchPipe[1]);
        exit(0);
    }

    close(sortPipe[1]);
    close(searchPipe[1]);

    // Wait for both child processes to finish
    waitpid(sortPid, NULL, 0);
    waitpid(searchPid, NULL, 0);

    int sortedArray[arraySize];
    read(sortPipe[0], sortedArray, arraySize * sizeof(int));
    read(searchPipe[0], &index, sizeof(index));
    close(sortPipe[0]);
    close(searchPipe[0]);

    // Send the sorted array and the search result back to the client
    send(clientSocket, sortedArray, arraySize * sizeof(int), 0);
    send(clientSocket, &index, sizeof(index), 0);

    close(clientSocket);
    return NULL;
}

// Thread function to handle client requests
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

    // Parse the command and parameters
    char *command = strtok(buffer, "\n");
    char *filename = strtok(NULL, "\n");
    char *sortAlgo = strtok(NULL, "\n");
    char *searchAlgo = strtok(NULL, "\n");
    char *key = strtok(NULL, "\n");

    // Debug logging
    printf("Command: %s\n", command);
    printf("Filename: %s\n", filename);

    if (command == NULL)
    {
        fprintf(stderr, "Invalid request format\n");
        close(clientSocket);
        return NULL;
    }

    // Handle the command
    if (strcmp(command, "HASH") == 0)
    {
        handleHashRequest(clientSocket, filename);
    }
    else if (strcmp(command, "SORTSEARCH") == 0)
    {
        printf("Sort Algorithm: %s\n", sortAlgo);
        printf("Search Algorithm: %s\n", searchAlgo);
        printf("Key to Find: %s\n", key);
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

void sortArray(int *array, int size, int algorithm)
{
    switch (algorithm)
    {
    case 1: // Bubble sort
        bubbleSort(array, size);
        break;
    case 2: // Selection sort
        selectionSort(array, size);
        break;
    case 3: // Merge sort
        mergeSort(array, 0, size - 1);
        break;
    case 4: // Quick sort
        quickSort(array, 0, size - 1);
        break;
    default:
        printf("Unknown sorting algorithm\n");
    }
}

int searchArray(int *array, int size, int algorithm, int target)
{
    int index;
    switch (algorithm)
    {
    case 1:
        index = linearSearch(array, size, target);
        break;
    case 2:
        index = binarySearch(array, size, target);
        break;
    case 3:
        index = jumpSearch(array, size, target);
        break;
    case 4:
        index = interpolationSearch(array, size, target);
        break;
    default:
        printf("Unknown search algorithm\n");
        index = -1;
    }
    return index;
}

void readArrayFromFile(const char *fileName, int **array, int *arraySize)
{
    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        perror("File open error");
        return;
    }

    off_t fileSize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    *arraySize = fileSize / sizeof(int);
    *array = (int *)malloc(*arraySize * sizeof(int));
    if (!*array)
    {
        perror("Memory allocation error");
        close(fd);
        return;
    }

    ssize_t bytesRead = 0;
    ssize_t totalBytesRead = 0;
    while ((bytesRead = read(fd, *array + totalBytesRead / sizeof(int), FILE_CHUNK_SIZE)) > 0)
    {
        totalBytesRead += bytesRead;
    }

    if (bytesRead == -1)
    {
        perror("File read error");
    }

    close(fd);
}

// Function to run the server
void runServer(int port)
{
    int serverFD;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    if ((serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket
    if (bind(serverFD, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        close(serverFD);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverFD, 3) < 0)
    {
        perror("Listen failed");
        close(serverFD);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", port);

    while (1)
    {
        int *newSocket = malloc(sizeof(int));
        if ((*newSocket = accept(serverFD, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Accept failed");
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
