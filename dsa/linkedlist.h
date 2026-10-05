#ifndef LINKEDLIST_H
#define LINKEDLIST_H

/*
 * ─── Linked List (Pure C DSA) ────────────────────────────────────────────────
 * Singly linked list with void* data pointers.
 * Used for: Trivia questions storage.
 *
 * Operations:
 *   ll_create()      → allocate a new empty linked list
 *   ll_append()      → add element to tail (copies dataSize bytes)
 *   ll_get_at()      → get data pointer at index
 *   ll_size()        → number of nodes
 *   ll_destroy()     → free all nodes and data
 */

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LLNode {
    void*          data;   /* pointer to copied data */
    struct LLNode* next;   /* next node */
} LLNode;

typedef struct {
    LLNode* head;
    LLNode* tail;
    int     count;
    size_t  dataSize;  /* size of each element in bytes */
} LinkedList;

/* Create a new empty linked list for elements of given size */
LinkedList* ll_create(size_t dataSize);

/* Append a copy of data to the tail. Returns 1 on success, 0 on failure. */
int ll_append(LinkedList* list, const void* data);

/* Get pointer to data at index. Returns NULL if out of range. */
void* ll_get_at(const LinkedList* list, int index);

/* Get number of nodes */
int ll_size(const LinkedList* list);

/* Free all nodes and their data */
void ll_destroy(LinkedList* list);

#ifdef __cplusplus
}
#endif

#endif /* LINKEDLIST_H */
