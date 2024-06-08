#include "searchSortAlgos.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int linearSearch(int arr[], int size, int key)
{
    int count = 0;
    int firstIndex = -1;
    for (int i = 0; i < size; i++)
    {
        if (arr[i] == key)
        {
            if (firstIndex == -1)
            {
                firstIndex = i;
            }
            count++;
        }
    }
    printf("Number of occurrences of %d - Using Linear Search Algorithm: %d\n", key, count);
    return firstIndex;
}

int binarySearch(int arr[], int size, int key)
{
    int count = 0;
    int firstIndex = -1;
    int low = 0, high = size - 1;
    while (low <= high)
    {
        int mid = low + (high - low) / 2;
        if (arr[mid] == key)
        {
            firstIndex = mid;
            count = 1;

            // Count occurrences to the left of mid
            for (int i = mid - 1; i >= 0 && arr[i] == key; i--)
            {
                if (firstIndex == mid)
                {
                    firstIndex = i;
                }
                count++;
            }

            // Count occurrences to the right of mid
            for (int i = mid + 1; i < size && arr[i] == key; i++)
            {
                count++;
            }

            printf("Number of occurrences of %d - Using Binary Search Algorithm: %d\n", key, count);
            return firstIndex;
        }
        else if (arr[mid] < key)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }
    printf("Number of occurrences of %d - Using Binary Search Algorithm: %d\n", key, count);
    return firstIndex;
}

int min(int a, int b)
{
    return (a < b) ? a : b;
}

int jumpSearch(int arr[], int size, int key)
{
    int count = 0;
    int firstIndex = -1;
    int step = sqrt(size);
    int prev = 0;

    while (arr[min(step, size) - 1] < key)
    {
        prev = step;
        step += sqrt(size);
        if (prev >= size)
        {
            printf("Number of occurrences of %d - Using Jump Search Algorithm: %d\n", key, count);
            return firstIndex;
        }
    }

    for (int i = prev; i < min(step, size); i++)
    {
        if (arr[i] == key)
        {
            if (firstIndex == -1)
            {
                firstIndex = i;
            }
            count++;
        }
    }

    printf("Number of occurrences of %d - Using Jump Search Algorithm: %d\n", key, count);
    return firstIndex;
}

int interpolationSearch(int arr[], int size, int key)
{
    int count = 0;
    int firstIndex = -1;
    int low = 0, high = size - 1;

    while (low <= high && key >= arr[low] && key <= arr[high])
    {
        if (low == high)
        {
            if (arr[low] == key)
            {
                firstIndex = low;
                count = 1;
            }
            printf("Number of occurrences of %d - Using Interpolation Search Algorithm: %d\n", key, count);
            return firstIndex;
        }

        int pos = low + (((double)(high - low) / (arr[high] - arr[low])) * (key - arr[low]));

        if (arr[pos] == key)
        {
            firstIndex = pos;
            count = 1;

            // Count occurrences to the left of pos
            for (int i = pos - 1; i >= 0 && arr[i] == key; i--)
            {
                if (firstIndex == pos)
                {
                    firstIndex = i;
                }
                count++;
            }

            // Count occurrences to the right of pos
            for (int i = pos + 1; i < size && arr[i] == key; i++)
            {
                count++;
            }

            printf("Number of occurrences of %d - Using Interpolation Search Algorithm: %d\n", key, count);
            return firstIndex;
        }

        if (arr[pos] < key)
        {
            low = pos + 1;
        }
        else
        {
            high = pos - 1;
        }
    }
    printf("Number of occurrences of %d - Using Interpolation Search Algorithm: %d\n", key, count);
    return firstIndex;
}

// Sorting

void bubbleSort(int arr[], int size)
{
    for (int i = 0; i < size - 1; i++)
    {
        for (int j = 0; j < size - i - 1; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

void merge(int arr[], int left, int mid, int right)
{
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    for (i = 0; i < n1; i++)
    {
        L[i] = arr[left + i];
    }
    for (j = 0; j < n2; j++)
    {
        R[j] = arr[mid + 1 + j];
    }

    i = 0;
    j = 0;
    k = left;
    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
        {
            arr[k] = L[i];
            i++;
        }
        else
        {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1)
    {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2)
    {
        arr[k] = R[j];
        j++;
        k++;
    }

    free(L);
    free(R);
}

void mergeSort(int arr[], int left, int right)
{
    if (left < right)
    {
        int mid = left + (right - left) / 2;

        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);

        merge(arr, left, mid, right);
    }
}

void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

int partition(int arr[], int low, int high)
{
    int pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++)
    {
        if (arr[j] < pivot)
        {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void quickSort(int arr[], int low, int high)
{
    if (low < high)
    {
        int pi = partition(arr, low, high);

        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

void selectionSort(int arr[], int size)
{
    for (int i = 0; i < size - 1; i++)
    {
        int minIndex = i;
        for (int j = i + 1; j < size; j++)
        {
            if (arr[j] < arr[minIndex])
            {
                minIndex = j;
            }
        }
        int temp = arr[minIndex];
        arr[minIndex] = arr[i];
        arr[i] = temp;
    }
}