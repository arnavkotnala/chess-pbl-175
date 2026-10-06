#ifndef SORT_H
#define SORT_H

/*
 * ─── Sorting (Pure C DSA) ────────────────────────────────────────────────────
 * Generic merge sort implementation for arrays of any element type.
 * Used for: Leaderboard sorting (by EXP, points, etc.)
 *
 * Uses a comparison function pointer for flexible sorting criteria.
 *
 * Operations:
 *   merge_sort() → sort array in-place using merge sort algorithm
 */

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Comparison function type.
 * Returns negative if a < b, 0 if equal, positive if a > b.
 */
typedef int (*CompareFunc)(const void* a, const void* b);

/*
 * Merge sort: sorts 'arr' of 'n' elements, each of 'elemSize' bytes.
 * Uses 'cmp' function for comparisons.
 * Stable sort — preserves relative order of equal elements.
 */
void merge_sort(void* arr, int n, size_t elemSize, CompareFunc cmp);

/*
 * Helper: merge two sorted halves.
 * Internal — called by merge_sort.
 */
void merge(void* arr, int left, int mid, int right, size_t elemSize, CompareFunc cmp);

#ifdef __cplusplus
}
#endif

#endif /* SORT_H */
