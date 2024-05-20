#include "utilities.h"
#include <stdio.h>

void dispArray(int arr[], int arrSize)
{

    for (int i = 0; i < arrSize; i++)
    {
        if (i > 20)
        {
            printf("...");
            break;
        }
        
        printf("%d ", arr[i]);
    }

    printf("\n");
}