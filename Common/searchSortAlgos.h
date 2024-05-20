#ifndef SEARCHSORTALGOS_H
#define SEARCHSORTALGOS_H

#include <stdlib.h>
#include <math.h>

int linearSearch(int arr[], int size, int key);
int binarySearch(int arr[], int size, int key);
int jumpSearch(int arr[], int size, int key);
int interpolationSearch(int arr[], int size, int key);

void bubbleSort(int arr[], int size);
void merge(int arr[], int left, int mid, int right); // Helper function for mergeSort
void mergeSort(int arr[], int left, int right);
int partition(int arr[], int low, int high); // Helper function for quickSort
void swap(int *a, int *b);                   // Helper function for quickSort
void quickSort(int arr[], int low, int high);
void selectionSort(int arr[], int size);

#endif