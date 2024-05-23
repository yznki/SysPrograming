#include "hashing.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <openssl/sha.h>

void hashing(const char *src, const char *dest)
{
    unsigned char sha256_digest[SHA256_DIGEST_LENGTH];
    unsigned char *buffer;
    unsigned char *temp_buffer;
    int sourceFile = open(src, O_RDONLY);
    if (sourceFile < 0)
    {
        perror("Error opening source file");
        exit(1); // Exit with specific failure status for file not found
    }

    int buffer_size = 1024; // Initial buffer size
    int bytes_read;
    int total_bytes_read = 0;

    buffer = malloc(buffer_size * sizeof(unsigned char));
    if (buffer == NULL)
    {
        perror("Error allocating memory");
        close(sourceFile);
        exit(2); // Memory Allocation Error
    }

    while ((bytes_read = read(sourceFile, buffer + total_bytes_read, buffer_size - total_bytes_read)) > 0)
    {
        total_bytes_read += bytes_read;
        if (total_bytes_read == buffer_size)
        {
            buffer_size *= 2;
            temp_buffer = realloc(buffer, buffer_size);
            if (temp_buffer == NULL)
            {
                perror("Error reallocating memory");
                free(buffer);
                close(sourceFile);
                exit(2); // Memory Allocation Error
            }
            buffer = temp_buffer;
        }
    }

    if (bytes_read < 0)
    {
        perror("Error reading source file");
        free(buffer);
        close(sourceFile);
        exit(3); // File Reading Error
    }

    SHA256(buffer, total_bytes_read, sha256_digest);

    // Convert binary hash to hexadecimal string
    char hex_output[SHA256_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(hex_output + (i * 2), "%02x", sha256_digest[i]);
    }
    hex_output[SHA256_DIGEST_LENGTH * 2] = '\0'; // Null-terminate the string

    int destinationFile = open(dest, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (destinationFile < 0)
    {
        perror("Error opening destination file");
        free(buffer);
        close(sourceFile);
        exit(4); // File Opening Error
    }

    if (write(destinationFile, hex_output, strlen(hex_output)) < 0)
    {
        perror("Error writing to destination file");
        free(buffer);
        close(sourceFile);
        close(destinationFile);
        exit(5); // File Writing Error
    }

    free(buffer);
    close(sourceFile);
    close(destinationFile);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <source file> <destination file>\n", argv[0]);
        return 1;
    }

    hashing(argv[1], argv[2]);
    return 0;
}
