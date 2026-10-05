/*
 * ─── Queue Implementation (Pure C) ───────────────────────────────────────────
 * Circular array-based queue with dynamic resizing.
 * Stores fixed-size strings for move notation logging.
 */

#include "queue.h"
#include <stdio.h>

Queue* queue_create(int initialCapacity) {
    if (initialCapacity <= 0) initialCapacity = 128;

    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (!q) return NULL;

    q->data = (char*)malloc(QUEUE_STR_LEN * initialCapacity);
    if (!q->data) {
        free(q);
        return NULL;
    }
    memset(q->data, 0, QUEUE_STR_LEN * initialCapacity);

    q->front = 0;
    q->rear = 0;
    q->count = 0;
    q->capacity = initialCapacity;
    return q;
}

/* Helper: get pointer to string at raw index */
static char* get_slot(Queue* q, int rawIndex) {
    return q->data + (rawIndex % q->capacity) * QUEUE_STR_LEN;
}
static const char* get_slot_const(const Queue* q, int rawIndex) {
    return q->data + (rawIndex % q->capacity) * QUEUE_STR_LEN;
}

/* Helper: grow the queue by reorganizing into a new linear buffer */
static int queue_grow(Queue* q) {
    int newCap = q->capacity * 2;
    char* newData = (char*)malloc(QUEUE_STR_LEN * newCap);
    if (!newData) return 0;
    memset(newData, 0, QUEUE_STR_LEN * newCap);

    /* Copy elements in logical order to new buffer */
    for (int i = 0; i < q->count; i++) {
        int srcIdx = (q->front + i) % q->capacity;
        memcpy(newData + i * QUEUE_STR_LEN,
               q->data + srcIdx * QUEUE_STR_LEN,
               QUEUE_STR_LEN);
    }

    free(q->data);
    q->data = newData;
    q->front = 0;
    q->rear = q->count;
    q->capacity = newCap;
    return 1;
}

int queue_enqueue(Queue* q, const char* str) {
    if (!q || !str) return 0;

    if (q->count >= q->capacity) {
        if (!queue_grow(q)) return 0;
    }

    char* slot = get_slot(q, q->rear);
    strncpy(slot, str, QUEUE_STR_LEN - 1);
    slot[QUEUE_STR_LEN - 1] = '\0';

    q->rear = (q->rear + 1) % q->capacity;
    q->count++;
    return 1;
}

int queue_dequeue(Queue* q, char* out) {
    if (!q || q->count <= 0) return 0;

    if (out) {
        const char* slot = get_slot_const(q, q->front);
        strncpy(out, slot, QUEUE_STR_LEN);
    }

    q->front = (q->front + 1) % q->capacity;
    q->count--;
    return 1;
}

const char* queue_front(const Queue* q) {
    if (!q || q->count <= 0) return NULL;
    return get_slot_const(q, q->front);
}

int queue_is_empty(const Queue* q) {
    return (!q || q->count <= 0);
}

int queue_size(const Queue* q) {
    if (!q) return 0;
    return q->count;
}

const char* queue_get_at(const Queue* q, int index) {
    if (!q || index < 0 || index >= q->count) return NULL;
    int rawIdx = (q->front + index) % q->capacity;
    return get_slot_const(q, rawIdx);
}

int queue_remove_last(Queue* q) {
    if (!q || q->count <= 0) return 0;
    q->rear = (q->rear - 1 + q->capacity) % q->capacity;
    q->count--;
    return 1;
}

void queue_clear(Queue* q) {
    if (!q) return;
    q->front = 0;
    q->rear = 0;
    q->count = 0;
}

void queue_destroy(Queue* q) {
    if (!q) return;
    if (q->data) free(q->data);
    free(q);
}
