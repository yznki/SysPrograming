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
    time_t mtime; // Last modification time
} HashCache;

typedef struct
{
    int *array;
    int size;
    int algorithm;
    int *finished;
    int *fastestAlgorithm;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
} SortArgs;

typedef struct
{
    int *array;
    int size;
    int algorithm;
    int target;
    int *found;
    int *index;
    int *fastestAlgorithm; 
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
} SearchArgs;

HashCache hashCache[CACHE_SIZE];
int hashCacheSize = 0;
pthread_mutex_t cacheLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hashLock = PTHREAD_MUTEX_INITIALIZER;

ssize_t writeFull(int sockfd, const void *buf, size_t len)
{
    size_t total = 0;
    ssize_t n;

    while (total < len)
    {
        n = write(sockfd, buf + total, len - total);
        if (n <= 0)
            return n;
        total += n;
    }
    return total;
}

int writeDataInChunks(int fd, void *data, size_t dataSize)
{
    size_t totalWritten = 0;
    size_t chunkSize = 4096; // 4KB chunks
    while (totalWritten < dataSize)
    {
        size_t bytesToWrite = (dataSize - totalWritten > chunkSize) ? chunkSize : (dataSize - totalWritten);
        ssize_t written = write(fd, (char *)data + totalWritten, bytesToWrite);
        if (written < 0)
        {
            perror("write");
            return -1;
        }
        totalWritten += written;
    }

    return 0;
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

// Function Declarations
int findHash(const char *filename, char *hash, time_t *mtime);
void addHash(const char *filename, const char *hash, time_t mtime);
void computeHash(const char *filename, char *hash, time_t *mtime);
void handleFileTransfer(int clientSocket);
void handleHashRequest(int clientSocket, const char *filename);
void handleSortSearchRequest(const char *filename, int clientSocket, int sortAlgo, int searchAlgo, int keyToFind);
void *handleClient(void *args);
void runServer(int port);
void sortArray(int *array, int size, int *algorithm);
int searchArray(int *array, int size, int *algorithm, int target);
void readArrayFromFile(const char *fileName, int **array, int *arraySize);
void *sort(void *args);

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
    char hashFile[] = "hashOutput.txt";
    char mtimeFile[] = "mtimeOutput.txt";
    pid_t pid;

    pthread_mutex_lock(&hashLock);
    pid = fork();

    if (pid == 0)
    {
        // Child process
        char *args[] = {"./hash.sh", (char *)filename, hashFile, mtimeFile, NULL};
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
            int hashFD = open(hashFile, O_RDONLY);
            if (hashFD < 0)
            {
                perror("open hashFile");
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
            int mtimeFD = open(mtimeFile, O_RDONLY);
            if (mtimeFD < 0)
            {
                perror("open mtimeFile");
                *mtime = 0;
            }
            else
            {
                char mtimeStr[20];
                int bytesRead = read(mtimeFD, mtimeStr, sizeof(mtimeStr) - 1);
                if (bytesRead < 0)
                {
                    perror("read");
                    *mtime = 0;
                }
                else
                {
                    mtimeStr[bytesRead] = '\0';
                    *mtime = atol(mtimeStr);
                }
                close(mtimeFD);
            }
        }
        else
        {
            strcpy(hash, "ERROR: HASH COMPUTATION FAILED");
            *mtime = 0;
        }
        remove(hashFile);
        remove(mtimeFile);
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
    int *array = NULL;
    int arraySize = 0;

    // Read the array from the file
    readArrayFromFile(filename, &array, &arraySize);

    printf("Read Array from file: \n");
    dispArray(array, arraySize);

    // printf("The array's size is: %d\n", arraySize);

    int sortPipe[2];
    if (pipe(sortPipe) == -1)
    {
        perror("Pipe failed");
        close(clientSocket);
        free(array);
        return;
    }

    pid_t sortPid = fork();
    if (sortPid == 0)
    {
        close(sortPipe[0]); // Close unused read end
        
        sortArray(array, arraySize, &sortAlgo);
        writeDataInChunks(sortPipe[1], array, arraySize * sizeof(int));
        writeFull(sortPipe[1], &sortAlgo, sizeof(sortAlgo));
        close(sortPipe[1]); // Close write end after writing
        exit(0);
    }
    else if (sortPid < 0)
    {
        perror("Unable to create child.\n");
        return;
    }

    // Searching
    close(sortPipe[1]);

    // Wait for the sorting process to finish
    waitpid(sortPid, NULL, 0);

    int *sortedArray = (int *)malloc(arraySize * sizeof(int));
    if (sortedArray == NULL)
    {
        perror("Malloc failed");
        free(array);
        close(clientSocket);
        return;
    }

    readDataInChunks(sortPipe[0], sortedArray, arraySize * sizeof(int));
    read(sortPipe[0], &sortAlgo, sizeof(sortAlgo));
    close(sortPipe[0]); // Close read end after reading

    printf("Sorted Array: \n");
    dispArray(sortedArray, arraySize);

    // Perform the search

    index = searchArray(sortedArray, arraySize, &searchAlgo, keyToFind);

    // Send the sorted array size first
    if (writeFull(clientSocket, &arraySize, sizeof(int)) < 0)
    {
        perror("Error sending array size to client");
        free(array);
        free(sortedArray);
        close(clientSocket);
        return;
    }

    // Send the sorted array and the search result back to the client
    if (writeDataInChunks(clientSocket, sortedArray, arraySize * sizeof(int)) < 0)
    {
        perror("Error sending sorted array to client");
        free(array);
        free(sortedArray);
        close(clientSocket);
        return;
    }

    // Fastest Sorting Algo
    if (writeFull(clientSocket, &sortAlgo, sizeof(int)) < 0)
    {
        perror("Error sending fastest sort algo to client");
        free(array);
        free(sortedArray);
        close(clientSocket);
        return;
    }

    if (writeFull(clientSocket, &index, sizeof(index)) < 0)
    {
        perror("Error sending index to client");
        free(array);
        free(sortedArray);
        close(clientSocket);
        return;
    }

    // Fastest Searching Algo
    if (writeFull(clientSocket, &searchAlgo, sizeof(int)) < 0)
    {
        perror("Error sending fastest search algo to client");
        free(array);
        free(sortedArray);
        close(clientSocket);
        return;
    }

    printf("Search result index: %d\n", index);

    close(clientSocket);
    free(array);
    free(sortedArray);
    printf("Server finished processing request.\n");
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

void *sort(void *args)
{
    SortArgs *sortArgs = (SortArgs *)args;

    int *localArray = malloc(sortArgs->size * sizeof(int));
    if (!localArray)
    {
        perror("Failed to allocate memory");
        pthread_exit(NULL);
    }

    for (int i = 0; i < sortArgs->size; i++)
    {
        localArray[i] = sortArgs->array[i];
    }

    switch (sortArgs->algorithm)
    {
    case 1:
        bubbleSort(localArray, sortArgs->size);
        break;
    case 2:
        selectionSort(localArray, sortArgs->size);
        break;
    case 3:
        mergeSort(localArray, 0, sortArgs->size - 1);
        break;
    case 4:
        quickSort(localArray, 0, sortArgs->size - 1);
        break;
    }

    pthread_mutex_lock(sortArgs->mutex);
    if (*(sortArgs->finished) == 0)
    {
        // printf("Thread %d finished first\n", sortArgs->algorithm);
        for (int i = 0; i < sortArgs->size; i++)
        {
            sortArgs->array[i] = localArray[i];
        }
        *(sortArgs->finished) = 1;
        *(sortArgs->fastestAlgorithm) = sortArgs->algorithm; // Set the fastest algorithm
        pthread_cond_signal(sortArgs->cond);
    }
    pthread_mutex_unlock(sortArgs->mutex);

    free(localArray);
    return NULL;
}

void sortArray(int *array, int size, int *algorithm)
{
    static int fastestSortAlgorithm = 0; // Static to keep track of fastest algorithm across calls

    switch (*algorithm)
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
    case 5:
    { // Fastest Sorting Algo
        pthread_t threads[4];
        SortArgs args[4];
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
        int finished = 0;

        // Initialize thread arguments
        for (int i = 0; i < 4; i++)
        {
            args[i].array = array; // Use the shared array
            args[i].size = size;
            args[i].algorithm = i + 1;
            args[i].finished = &finished;
            args[i].fastestAlgorithm = &fastestSortAlgorithm; // Pass the pointer
            args[i].mutex = &mutex;
            args[i].cond = &cond;
        }

        // Create threads
        for (int i = 0; i < 4; i++)
        {
            if (pthread_create(&threads[i], NULL, sort, &args[i]) != 0)
            {
                perror("Failed to create thread");
                exit(EXIT_FAILURE);
            }
        }

        // Wait for the first thread to finish
        pthread_mutex_lock(&mutex);
        while (!finished)
        {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        // Cancel other threads and ensure they are terminated properly
        for (int i = 0; i < 4; i++)
        {
            pthread_cancel(threads[i]);
            pthread_join(threads[i], NULL); // Ensure threads have terminated
        }

        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
        break;
    }
    default:
        printf("Unknown sorting algorithm\n");
    }

    printf("Fastest Algo: %d\n", fastestSortAlgorithm);

    *algorithm = fastestSortAlgorithm;
}

void *search(void *args)
{
    SearchArgs *searchArgs = (SearchArgs *)args;

    int localIndex = -1;
    // printf("Thread %d starting search.\n", searchArgs->algorithm);

    switch (searchArgs->algorithm)
    {
    case 1:
        localIndex = linearSearch(searchArgs->array, searchArgs->size, searchArgs->target);
        break;
    case 2:
        localIndex = binarySearch(searchArgs->array, searchArgs->size, searchArgs->target);
        break;
    case 3:
        localIndex = jumpSearch(searchArgs->array, searchArgs->size, searchArgs->target);
        break;
    case 4:
        localIndex = interpolationSearch(searchArgs->array, searchArgs->size, searchArgs->target);
        break;
    }

    // printf("Thread %d search completed. Local index: %d\n", searchArgs->algorithm, localIndex);

    pthread_mutex_lock(searchArgs->mutex);
    if (*(searchArgs->found) == 0 && localIndex != -1)
    {
        // printf("Thread %d found the target first at index %d\n", searchArgs->algorithm, localIndex);
        *(searchArgs->index) = localIndex;
        *(searchArgs->found) = 1;
        *(searchArgs->fastestAlgorithm) = searchArgs->algorithm;
        pthread_cond_signal(searchArgs->cond);
    }
    else if (*(searchArgs->found) == 0)
    {
        *(searchArgs->found) = 1;
        pthread_cond_signal(searchArgs->cond);
    }
    pthread_mutex_unlock(searchArgs->mutex);

    return NULL;
}

int searchArray(int *array, int size, int *algorithm, int target)
{
    static int fastestSearchAlgorithm = 0;

    int index;
    // printf("Starting search with algorithm %d\n", *algorithm);
    switch (*algorithm)
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
    case 5:
    { // Fastest Search Algorithm
        pthread_t threads[4];
        SearchArgs args[4];
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
        int found = 0;
        int foundIndex = -1;
        int completed = 0; // Track number of completed threads

        // Initialize thread arguments
        for (int i = 0; i < 4; i++)
        {
            args[i].array = array;
            args[i].size = size;
            args[i].algorithm = i + 1;
            args[i].target = target;
            args[i].found = &found;
            args[i].index = &foundIndex;
            args[i].fastestAlgorithm = &fastestSearchAlgorithm; 
            args[i].mutex = &mutex;
            args[i].cond = &cond;
            // printf("Initializing search thread %d\n", i + 1);
        }

        // Create threads
        for (int i = 0; i < 4; i++)
        {
            if (pthread_create(&threads[i], NULL, search, &args[i]) != 0)
            {
                perror("Failed to create thread");
                exit(EXIT_FAILURE);
            }
        }

        // Wait for the first thread to find the target or all threads to complete
        pthread_mutex_lock(&mutex);
        while (!found && completed < 4)
        {
            pthread_cond_wait(&cond, &mutex);
            completed++;
        }
        pthread_mutex_unlock(&mutex);

        // Cancel other threads and ensure they are terminated properly
        for (int i = 0; i < 4; i++)
        {
            pthread_cancel(threads[i]);
            pthread_join(threads[i], NULL); // Ensure threads have terminated
        }

        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
        index = foundIndex;
        break;
    }
    default:
        printf("Unknown search algorithm\n");
        index = -1;
    }

    printf("Fastest search algorithm: %d, Index found: %d\n", fastestSearchAlgorithm, index);
    *algorithm = fastestSearchAlgorithm;
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

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        perror("fstat error");
        close(fd);
        return;
    }

    off_t fileSize = st.st_size;
    *arraySize = fileSize / sizeof(int);

    *array = (int *)malloc(fileSize);
    if (!*array)
    {
        perror("Memory allocation error");
        close(fd);
        return;
    }

    ssize_t bytesRead = 0;
    ssize_t totalBytesRead = 0;
    while ((bytesRead = read(fd, *array + totalBytesRead / sizeof(int), fileSize - totalBytesRead)) > 0)
    {
        totalBytesRead += bytesRead;
    }

    if (bytesRead == -1)
    {
        perror("File read error");
        free(*array);
        *array = NULL;
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

        printf("\n\n\n\nAccepted new connection\n");

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