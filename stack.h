#ifndef STACK_H
#define STACK_H

/*
 * ─── Stack (Pure C DSA) ───────────────────────────────────────────────────────
 * Generic array-based stack using void* elements.
 * Used for: Move undo history (stores GameState snapshots).
 *
 * Operations:
 *   stack_create()   → allocate a new stack
 *   stack_push()     → push element (copies elementSize bytes)
 *   stack_pop()      → pop top element (copies into out buffer)
 *   stack_peek()     → peek at top element without removing
 *   stack_is_empty() → check if stack is empty
 *   stack_size()     → number of elements
 *   stack_clear()    → remove all elements
 *   stack_destroy()  → free all memory
 */

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void*  data;         /* raw byte buffer */
    int    top;          /* index of top element (-1 = empty) */
    int    capacity;     /* max elements before realloc */
    size_t elementSize;  /* size of each element in bytes */
} Stack;

/* Create a new stack for elements of given size */
Stack* stack_create(size_t elementSize, int initialCapacity);

/* Push a copy of element onto stack. Returns 1 on success, 0 on failure. */
int stack_push(Stack* s, const void* element);

/* Pop top element into 'out' buffer. Returns 1 on success, 0 if empty. */
int stack_pop(Stack* s, void* out);

/* Peek at top element without removing. Returns pointer to element, or NULL if empty. */
const void* stack_peek(const Stack* s);

/* Check if stack is empty */
int stack_is_empty(const Stack* s);

/* Get number of elements */
int stack_size(const Stack* s);

/* Remove all elements (does not free the stack itself) */
void stack_clear(Stack* s);

/* Free all memory used by the stack */
void stack_destroy(Stack* s);

#ifdef __cplusplus
}
#endif

#endif /* STACK_H */
