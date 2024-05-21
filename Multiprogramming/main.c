#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
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
#include "../Common/hashing.h"

int main(void)
{
    // system("clear");
    double processStartTime = clock();
    srand(time(NULL));
    int servicesPID = fork();

    if (servicesPID == 0)
    {
        // Hashing
        printf("****** Hashing Service ******\n\n");
        double serviceStartTime = clock();
        hashing("sample.txt", "output.txt");
        double serviceEndTime = clock();
        printf("Hashing Service Time\t%lf Seconds", (serviceEndTime - serviceStartTime) / CLOCKS_PER_SEC);

        printf("\n\n");
        exit(0);
    }
    else if (servicesPID > 0)
    {
        waitpid(servicesPID, NULL, 0);
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

                int sortingPID[4];

                for (int i = 0; i < 4; i++)
                {
                    if ((sortingPID[i] = fork()) == 0)
                    {
                        double startTime, endTime;
                        int tempArr[ARRAY_SIZE];
                        memcpy(tempArr, arr, sizeof(int) * ARRAY_SIZE);

                        switch (i)
                        {
                        case 0:
                            startTime = clock();
                            selectionSort(tempArr, ARRAY_SIZE);
                            endTime = clock();
                            printf("Selection Sort Time\t%lf Seconds\n", (endTime - startTime) / CLOCKS_PER_SEC);
                            break;
                        case 1:
                            startTime = clock();
                            bubbleSort(tempArr, ARRAY_SIZE);
                            endTime = clock();
                            printf("Bubble Sort Time\t%lf Seconds\n", (endTime - startTime) / CLOCKS_PER_SEC);
                            break;
                        case 2:
                            startTime = clock();
                            quickSort(tempArr, 0, ARRAY_SIZE - 1);
                            endTime = clock();
                            printf("Quick Sort Time\t\t%lf Seconds\n", (endTime - startTime) / CLOCKS_PER_SEC);
                            break;
                        case 3:
                            startTime = clock();
                            mergeSort(arr, 0, ARRAY_SIZE - 1);
                            endTime = clock();
                            printf("Merge Sort Time\t\t%lf Seconds\n", (endTime - startTime) / CLOCKS_PER_SEC);
                            write(fd[1], arr, sizeof(int) * ARRAY_SIZE);
                            break;

                        default:
                            break;
                        }
                        exit(0);
                    }
                }

                for (int i = 0; i < 4; i++)
                {
                    waitpid(sortingPID[i], NULL, 0); // Wait for all sorting child processes to finish
                }

                exit(0);
            }
            else if (pid > 0)
            {
                // Searching
                wait(NULL);

                // int val = rand() % 100 + 1;
                int val = 100;
                int *sortedArr = malloc(sizeof(int) * ARRAY_SIZE);
                read(fd[0], sortedArr, sizeof(int) * ARRAY_SIZE);

                printf("\n");
                printf("Array After Sorting: ");
                dispArray(sortedArr, ARRAY_SIZE);

                printf("\n");
                printf("Key to find: %d\n", val);
                printf("\n");

                

                int searchingPID[4];

                for (int i = 0; i < 4; i++)
                {
                    if ((searchingPID[i] = fork()) == 0)
                    {
                        int index;
                        double startTime, endTime;

                        switch (i)
                        {
                        case 0:
                            startTime = clock();
                            index = linearSearch(sortedArr, ARRAY_SIZE, val);
                            endTime = clock();
                            if (index == -1)
                            {
                                printf("Linear Search Time\t\t%lf Seconds\tIndex - Not Found\n", (endTime - startTime) / CLOCKS_PER_SEC);
                            }
                            else
                            {
                                printf("Linear Search Time\t\t%lf Seconds\tIndex - %d\n", (endTime - startTime) / CLOCKS_PER_SEC, index);
                            }
                            break;
                        case 1:
                            startTime = clock();
                            index = binarySearch(sortedArr, ARRAY_SIZE, val);
                            endTime = clock();
                            if (index == -1)
                            {
                                printf("Binary Search Time\t\t%lf Seconds\tIndex - Not Found\n", (endTime - startTime) / CLOCKS_PER_SEC);
                            }
                            else
                            {
                                printf("Binary Search Time\t\t%lf Seconds\tIndex - %d\n", (endTime - startTime) / CLOCKS_PER_SEC, index);
                            }
                            break;
                        case 2:
                            startTime = clock();
                            index = interpolationSearch(sortedArr, ARRAY_SIZE, val);
                            endTime = clock();
                            if (index == -1)
                            {
                                printf("Interpolation Search Time\t%lf Seconds\tIndex - Not Found\n", (endTime - startTime) / CLOCKS_PER_SEC);
                            }
                            else
                            {
                                printf("Interpolation Search Time\t%lf Seconds\tIndex - %d\n", (endTime - startTime) / CLOCKS_PER_SEC, index);
                            }
                            break;
                        case 3:
                            startTime = clock();
                            index = jumpSearch(sortedArr, ARRAY_SIZE, val);
                            endTime = clock();
                            if (index == -1)
                            {
                                printf("Jump Search Time\t\t%lf Seconds\tIndex - Not Found\n", (endTime - startTime) / CLOCKS_PER_SEC);
                            }
                            else
                            {
                                printf("Jump Search Time\t\t%lf Seconds\tIndex - %d\n", (endTime - startTime) / CLOCKS_PER_SEC, index);
                            }
                            break;
                        default:
                            break;
                        }
                        exit(0);
                    }
                }

                for (int i = 0; i < 4; i++)
                {
                    waitpid(searchingPID[i], NULL, 0); // Wait for all sorting child processes to finish
                }

                free(sortedArr);
            }
            else
            {
                perror("Cannot Create Child...\n");
                return -1;
            }
            
            exit(0);
        }
        else if (searchSortPID > 0)
        {
            // Parent
            double serviceStartTime = clock();
            waitpid(searchSortPID,NULL,0);
            
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
