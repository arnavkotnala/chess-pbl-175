/*
 * ─── Sorting Implementation (Pure C) ─────────────────────────────────────────
 * Generic merge sort — stable, O(n log n) time, O(n) auxiliary space.
 * Works with any element type via void* and element size.
 */

#include "sort.h"
#include <stdio.h>

/* Helper: get pointer to element at index i */
static void* elem_at(void* arr, int i, size_t elemSize) {
    return (char*)arr + i * elemSize;
}

void merge(void* arr, int left, int mid, int right, size_t elemSize, CompareFunc cmp) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    /* Allocate temp arrays */
    void* leftArr  = malloc(n1 * elemSize);
    void* rightArr = malloc(n2 * elemSize);
    if (!leftArr || !rightArr) {
        if (leftArr)  free(leftArr);
        if (rightArr) free(rightArr);
        return;
    }

    /* Copy data to temp arrays */
    memcpy(leftArr,  elem_at(arr, left, elemSize),    n1 * elemSize);
    memcpy(rightArr, elem_at(arr, mid + 1, elemSize), n2 * elemSize);

    /* Merge temp arrays back */
    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        void* a = (char*)leftArr  + i * elemSize;
        void* b = (char*)rightArr + j * elemSize;
        if (cmp(a, b) <= 0) {
            memcpy(elem_at(arr, k, elemSize), a, elemSize);
            i++;
        } else {
            memcpy(elem_at(arr, k, elemSize), b, elemSize);
            j++;
        }
        k++;
    }

    /* Copy remaining elements */
    while (i < n1) {
        memcpy(elem_at(arr, k, elemSize), (char*)leftArr + i * elemSize, elemSize);
        i++; k++;
    }
    while (j < n2) {
        memcpy(elem_at(arr, k, elemSize), (char*)rightArr + j * elemSize, elemSize);
        j++; k++;
    }

    free(leftArr);
    free(rightArr);
}

/* Internal recursive merge sort */
static void merge_sort_recursive(void* arr, int left, int right, size_t elemSize, CompareFunc cmp) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        merge_sort_recursive(arr, left, mid, elemSize, cmp);
        merge_sort_recursive(arr, mid + 1, right, elemSize, cmp);
        merge(arr, left, mid, right, elemSize, cmp);
    }
}

void merge_sort(void* arr, int n, size_t elemSize, CompareFunc cmp) {
    if (!arr || n <= 1 || !cmp) return;
    merge_sort_recursive(arr, 0, n - 1, elemSize, cmp);
}
