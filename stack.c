/*
 * ─── Stack Implementation (Pure C) ────────────────────────────────────────────
 * Array-based generic stack with dynamic resizing.
 * All memory management uses malloc/realloc/free.
 */

#include "stack.h"
#include <stdio.h>

Stack* stack_create(size_t elementSize, int initialCapacity) {
    if (initialCapacity <= 0) initialCapacity = 16;

    Stack* s = (Stack*)malloc(sizeof(Stack));
    if (!s) return NULL;

    s->data = malloc(elementSize * initialCapacity);
    if (!s->data) {
        free(s);
        return NULL;
    }

    s->top = -1;
    s->capacity = initialCapacity;
    s->elementSize = elementSize;
    return s;
}

int stack_push(Stack* s, const void* element) {
    if (!s || !element) return 0;

    /* Grow if needed (double capacity) */
    if (s->top + 1 >= s->capacity) {
        int newCap = s->capacity * 2;
        void* newData = realloc(s->data, s->elementSize * newCap);
        if (!newData) return 0;
        s->data = newData;
        s->capacity = newCap;
    }

    s->top++;
    /* Copy element bytes into the data buffer at top position */
    memcpy((char*)s->data + s->top * s->elementSize, element, s->elementSize);
    return 1;
}

int stack_pop(Stack* s, void* out) {
    if (!s || s->top < 0) return 0;

    if (out) {
        memcpy(out, (char*)s->data + s->top * s->elementSize, s->elementSize);
    }
    s->top--;
    return 1;
}

const void* stack_peek(const Stack* s) {
    if (!s || s->top < 0) return NULL;
    return (const char*)s->data + s->top * s->elementSize;
}

int stack_is_empty(const Stack* s) {
    return (!s || s->top < 0);
}

int stack_size(const Stack* s) {
    if (!s) return 0;
    return s->top + 1;
}

void stack_clear(Stack* s) {
    if (s) s->top = -1;
}

void stack_destroy(Stack* s) {
    if (!s) return;
    if (s->data) free(s->data);
    free(s);
}
