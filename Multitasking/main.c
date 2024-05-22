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

typedef struct
{
    int *arr;
    int size;
    int algorithm; // 0: Bubble, 1: Merge, 2: Quick, 3: Selection
    double timeTaken;

} SortThreadData;

typedef struct
{
    int *arr;
    int size;
    int key;
    int result;
    int algorithm; // 0: Linear, 1: Binary, 2: Jump, 3: Interpolation
    double timeTaken;
} SearchThreadData;

void *threadSort(void *arg)
{
    SortThreadData *data = (SortThreadData *)arg;
    clock_t start = clock();
    switch (data->algorithm)
    {
    case 0:
        bubbleSort(data->arr, data->size);
        break;
    case 1:
        mergeSort(data->arr, 0, data->size - 1);
        break;
    case 2:
        quickSort(data->arr, 0, data->size - 1);
        break;
    case 3:
        selectionSort(data->arr, data->size);
        break;

    default:
        break;
    }
    clock_t end = clock();
    data->timeTaken = ((double)(end - start)) / CLOCKS_PER_SEC;
    return NULL;
}

void *threadSearch(void *arg)
{
    SearchThreadData *data = (SearchThreadData *)arg;
    clock_t start = clock();
    switch (data->algorithm)
    {
    case 0:
        data->result = linearSearch(data->arr, data->size, data->key);
        break;
    case 1:
        data->result = binarySearch(data->arr, data->size, data->key);
        break;
    case 2:
        data->result = jumpSearch(data->arr, data->size, data->key);
        break;
    case 3:
        data->result = interpolationSearch(data->arr, data->size, data->key);
        break;
    default:
        data->result = -1;
        break;
    }
    clock_t end = clock();
    data->timeTaken = ((double)(end - start)) / CLOCKS_PER_SEC;
    return NULL;
}

int main(void)
{
    system("clear");
    double processStartTime = clock();
    srand(time(NULL));

    double serviceStartTime = clock();
    int servicesPID = fork();

    if (servicesPID == 0)
    {
        // Hashing
        printf("****** Hashing Service ******\n\n");
        
        char *args[] = {"../Common/hashing.o", "sample.txt", "output.txt", NULL};
        execv("../Common/hashing.o", args);

        // If execv returns, it must have failed
        perror("execv");
        exit(EXIT_FAILURE);
    }
    else if (servicesPID > 0)
    {
        // To Calculate the Hashing Service Time
        waitpid(servicesPID, NULL, 0);

        double serviceEndTime = clock();
        printf("Hashing Service Time\t%lf Seconds\n\n", (serviceEndTime - serviceStartTime) / CLOCKS_PER_SEC);

        serviceStartTime = clock();

        // Sorting & Searching

        int searchSortFD[2];
        if (pipe(searchSortFD))
        {
            perror("Error creating pipe...\n");
            return -1;
        }

        int searchSortPID = fork();

        if (searchSortPID == 0)
        {
            // Sorting & Searching
            printf("****** Sorting & Searching Service ******\n\n");

            int fd[2];
            if (pipe(fd))
            {
                perror("Error creating pipe...\n");
                return -1;
            }

            int pid = fork();

            if (pid == 0)
            {
                // Sorting
                int arr[ARRAY_SIZE];

                for (int i = 0; i < ARRAY_SIZE; i++)
                {
                    arr[i] = rand() % 100 + 1;
                }

                printf("Array Before Sorting: ");
                dispArray(arr, ARRAY_SIZE);

                printf("\n");

                pthread_t threads[4];
                SortThreadData data[4];

                for (int i = 0; i < 4; i++)
                {
                    data[i].arr = (int *)malloc(ARRAY_SIZE * sizeof(int));
                    memcpy(data[i].arr, arr, ARRAY_SIZE * sizeof(int));
                    data[i].size = ARRAY_SIZE;
                    data[i].algorithm = i;

                    pthread_create(&threads[i], NULL, threadSort, &data[i]);
                }

                for (int i = 0; i < 4; i++)
                {
                    pthread_join(threads[i], NULL);
                }

                double minTime = data[0].timeTaken;
                int minIndex = 0;
                for (int i = 1; i < 4; i++)
                {
                    if (data[i].timeTaken < minTime)
                    {
                        minTime = data[i].timeTaken;
                        minIndex = i;
                    }
                }

                const char *algoNames[] = {"Bubble Sort", "Merge Sort", "Quick Sort", "Selection Sort"};
                printf("%s\t\t%lf Seconds.\n", algoNames[minIndex], minTime);
                for (int i = 0; i < 4; i++)
                {
                    if (i == minIndex)
                    {
                        continue;
                    }
                    printf("%s\t\t%lf Seconds.\n", algoNames[i], data[i].timeTaken);
                }

                printf("\nArray after sorting: ");
                dispArray(data[minIndex].arr, ARRAY_SIZE);

                memcpy(arr, data[minIndex].arr, ARRAY_SIZE * sizeof(int));

                write(fd[1], arr, sizeof(int) * ARRAY_SIZE);

                for (int i = 0; i < 4; i++)
                {
                    free(data[i].arr);
                }
                exit(0);
            }
            else if (pid > 0)
            {

                // Searching
                waitpid(pid, NULL, 0);
                // int val = rand() % 100 + 1;
                int val = 100;
                int *sortedArr = malloc(sizeof(int) * ARRAY_SIZE);
                read(fd[0], sortedArr, sizeof(int) * ARRAY_SIZE);

                printf("\n");
                printf("Key to find: %d\n", val);

                SearchThreadData searchData[4];
                pthread_t searchThreads[4];

                for (int i = 0; i < 4; i++)
                {
                    searchData[i].arr = sortedArr;
                    searchData[i].size = ARRAY_SIZE;
                    searchData[i].key = val;
                    searchData[i].algorithm = i;
                    pthread_create(&searchThreads[i], NULL, threadSearch, &searchData[i]);
                }

                for (int i = 0; i < 4; i++)
                {
                    pthread_join(searchThreads[i], NULL);
                }

                double minTime = searchData[0].timeTaken;
                int minIndex = 0;
                for (int i = 1; i < 4; i++)
                {
                    if (searchData[i].timeTaken < minTime)
                    {
                        minTime = searchData[i].timeTaken;
                        minIndex = i;
                    }
                }

                const char *algoNames[] = {"Linear Search", "Binary Search", "Jump Search", "Interpolation Search"};
                printf("\n%s\t\t%lf Seconds.\tIndex - %d\n", algoNames[minIndex], minTime, searchData[minIndex].result);
                for (int i = 0; i < 4; i++)
                {
                    if (i == minIndex)
                    {
                        continue;
                    }
                    if (!strcmp(algoNames[i], "Interpolation Search"))
                    {
                        printf("%s\t%lf Seconds.\tIndex - %d\n", algoNames[i], searchData[i].timeTaken, searchData[i].result);
                    }
                    else
                    {

                        printf("%s\t\t%lf Seconds.\tIndex - %d\n", algoNames[i], searchData[i].timeTaken, searchData[i].result);
                    }
                }
                free(sortedArr);
                exit(0);
            }
            else
            {
                perror("Cannot Create Child...\n");
                return -1;
            }
        }
        else if (searchSortPID > 0)
        {
            // Parent
            waitpid(searchSortPID, NULL, 0);

            printf("\n");

            double serviceEndTime = clock();
            printf("Sort & Search Time\t%lf Seconds\n", (serviceEndTime - serviceStartTime) / CLOCKS_PER_SEC);

            printf("\n");
            double processEndTime = clock();
            printf("Process Total Time\t%lf Seconds", (processEndTime - processStartTime) / CLOCKS_PER_SEC);
            printf("\n\n\n");
        }
        else
        {
            perror("Cannot Create Child...\n");
            return -1;
        }
    }
    else
    {
        perror("Cannot Create Child...\n");
        return -1;
    }

    return 0;
}
