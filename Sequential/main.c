#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
#include "../Common/hashing.h"

int main(void)
{
    double serviceStartTime, serviceEndTime, processStartTime = clock(), processEndTime;
    system("clear");
    srand(time(NULL));

    // Hashing
    printf("****** Hashing Service ******\n\n");
    serviceStartTime = clock();
    hashing("sample.txt", "output.txt");
    serviceEndTime = clock();
    printf("Hashing Service Time\t%lf Seconds", (serviceEndTime - serviceStartTime) / CLOCKS_PER_SEC);

    printf("\n\n");

    // Sorting & Searching
    printf("****** Sorting & Searching Service ******\n\n");

    serviceStartTime = clock();
    int arr[ARRAY_SIZE];
    // int val = rand() % 100 + 1;
    int val = 100;

    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        arr[i] = rand() % 100 + 1;
    }

    printf("Array Before Sorting: ");
    dispArray(arr, ARRAY_SIZE);

    printf("\n");

    for (int i = 0; i < 4; i++)
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
            printf("Selection Sort Time\t%lf Seconds\n",(endTime-startTime)/CLOCKS_PER_SEC);
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
            break;

        default:
            break;
        }
    }

    printf("\n");

    printf("Array After Sorting: ");
    dispArray(arr, ARRAY_SIZE);

    printf("\n");
    printf("Key to find: %d\n", val);
    printf("\n");

    for (int i = 0; i < 4; i++)
    {
        int index;
        double startTime, endTime;

        switch (i)
        {
        case 0:
            startTime = clock();
            index = linearSearch(arr, ARRAY_SIZE, val);
            endTime = clock();
            
            if (index == -1)
            {
                printf("Linear Search Time\t\t%lf Seconds\tIndex - Not Found\n", (endTime - startTime) / CLOCKS_PER_SEC);
                break;
            }
            
            printf("Linear Search Time\t\t%lf Seconds\tIndex - %d\n", (endTime - startTime) / CLOCKS_PER_SEC, index);
            break;
        case 1:
            startTime = clock();
            index = binarySearch(arr, ARRAY_SIZE, val);
            endTime = clock();
            if (index == -1)
            {
                printf("Binary Search Time\t\t%lf Seconds\tIndex - Not Found\n", (endTime - startTime) / CLOCKS_PER_SEC);
                break;
            }
            printf("Binary Search Time\t\t%lf Seconds\tIndex - %d\n", (endTime - startTime) / CLOCKS_PER_SEC, index);
            break;
        case 2:
            startTime = clock();
            index = interpolationSearch(arr, ARRAY_SIZE, val);
            endTime = clock();
            if (index == -1)
            {
                printf("Interpolation Search Time\t%lf Seconds\tIndex - Not Found\n", (endTime - startTime) / CLOCKS_PER_SEC);
                break;
            }
            printf("Interpolation Search Time\t%lf Seconds\tIndex - %d\n", (endTime - startTime) / CLOCKS_PER_SEC, index);
            break;
        case 3:
            startTime = clock();
            index = jumpSearch(arr, ARRAY_SIZE, val);
            endTime = clock();
            if (index == -1)
            {
                printf("Jump Search Time\t\t%lf Seconds\tIndex - Not Found\n", (endTime - startTime) / CLOCKS_PER_SEC);
                break;
            }
            printf("Jump Search Time\t\t%lf Seconds\tIndex - %d\n", (endTime - startTime) / CLOCKS_PER_SEC, index);
            break;

        default:
            break;
        }
    }

    printf("\n");

    serviceEndTime = clock();
    printf("Sort & Search Time\t%lf Seconds", (serviceEndTime - serviceStartTime) / CLOCKS_PER_SEC);

    printf("\n\n\n");
    processEndTime = clock();
    printf("Process Total Time\t%lf Seconds", (processEndTime-processStartTime)/CLOCKS_PER_SEC);
    printf("\n\n\n");
    return 0;
}
