#ifndef QUEUE_H
#define QUEUE_H

/*
 * ─── Queue (Pure C DSA) ──────────────────────────────────────────────────────
 * Circular array-based queue storing fixed-size string entries.
 * Used for: Move notation log (FIFO order).
 *
 * Operations:
 *   queue_create()    → allocate a new queue
 *   queue_enqueue()   → add string to rear
 *   queue_dequeue()   → remove string from front
 *   queue_front()     → peek at front string
 *   queue_is_empty()  → check if queue is empty
 *   queue_size()      → number of elements
 *   queue_get_at()    → get element at index (for iteration)
 *   queue_remove_last() → remove last element (for undo)
 *   queue_clear()     → remove all elements
 *   queue_destroy()   → free all memory
 */

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QUEUE_STR_LEN 64   /* max length of each move string */

typedef struct {
    char*  data;        /* array of strings (each QUEUE_STR_LEN bytes) */
    int    front;       /* front index */
    int    rear;        /* rear index (next insert position) */
    int    count;       /* current number of elements */
    int    capacity;    /* max elements before realloc */
} Queue;

/* Create a new queue */
Queue* queue_create(int initialCapacity);

/* Add string to rear. Returns 1 on success, 0 on failure. */
int queue_enqueue(Queue* q, const char* str);

/* Remove and copy front string into 'out'. Returns 1 on success, 0 if empty. */
int queue_dequeue(Queue* q, char* out);

/* Peek at front string without removing. Returns pointer or NULL if empty. */
const char* queue_front(const Queue* q);

/* Check if queue is empty */
int queue_is_empty(const Queue* q);

/* Get number of elements */
int queue_size(const Queue* q);

/* Get element at logical index (0 = front). Returns pointer or NULL. */
const char* queue_get_at(const Queue* q, int index);

/* Remove the last (most recently enqueued) element. Returns 1 on success. */
int queue_remove_last(Queue* q);

/* Remove all elements */
void queue_clear(Queue* q);

/* Free all memory */
void queue_destroy(Queue* q);

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H */
