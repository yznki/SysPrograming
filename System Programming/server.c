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
    char dest_file[] = "hash_output.txt";
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
                int dest_fd = open(dest_file, O_RDONLY);
                if (dest_fd < 0)
                {
                    perror("open dest_file");
                    strcpy(hash, "ERROR: FILE NOT FOUND");
                    pthread_mutex_unlock(&hashLock);
                    return;
                }

                int bytesRead = read(dest_fd, hash, SHA256_DIGEST_LENGTH * 2);
                if (bytesRead < 0)
                {
                    perror("read");
                    strcpy(hash, "ERROR: FILE READ ERROR");
                    close(dest_fd);
                    pthread_mutex_unlock(&hashLock);
                    return;
                }
                hash[bytesRead] = '\0';
                close(dest_fd);
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

void handleClientSortSearch(int clientSocket, const char *filename) {}

void *handleClient(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int read_size = read(client_sock, buffer, BUFFER_SIZE - 1);
    if (read_size <= 0)
    {
        close(client_sock);
        return NULL;
    }
    buffer[read_size] = '\0'; // Null-terminate the string

    // Extract the filename from the received data
    char filename[256];
    sscanf(buffer, "%s", filename);

    // Create a temporary file to store the received file contents
    char temp_filename[] = "/tmp/tempfileXXXXXX";
    int temp_fd = mkstemp(temp_filename);
    if (temp_fd == -1)
    {
        perror("Error creating temporary file");
        close(client_sock);
        return NULL;
    }

    // Write the remaining data to the temporary file
    write(temp_fd, buffer + strlen(filename) + 1, read_size - strlen(filename) - 1);

    // Continue receiving the file contents
    while ((read_size = read(client_sock, buffer, BUFFER_SIZE)) > 0)
    {
        write(temp_fd, buffer, read_size);
    }
    close(temp_fd);

    // Compute the hash
    char hash[SHA256_DIGEST_LENGTH * 2 + 1];
    computeHash(temp_filename, hash);

    // Send the hash to the client
    write(client_sock, hash, strlen(hash));

    // Clean up
    remove(temp_filename);
    close(client_sock);
    return NULL;
}

void runServer(int port)
{
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", port);

    while (1)
    {
        int *new_sock = malloc(sizeof(int));
        if ((*new_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            free(new_sock);
            continue;
        }

        printf("Accepted new connection\n");

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handleClient, (void *)new_sock) != 0)
        {
            perror("pthread_create");
            free(new_sock);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
}

int main(void)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        // Child process - Run the sort/search server
        // runServer(SORT_SEARCH_PORT);
    }
    else if (pid > 0)
    {
        // Parent process - Run the hash server
        runServer(HASH_PORT);
    }

    return 0;
}
