/*
 * ─── Linked List Implementation (Pure C) ─────────────────────────────────────
 * Singly linked list with dynamic node allocation.
 * Each node stores a copy of the data (deep copy of dataSize bytes).
 */

#include "linkedlist.h"
#include <stdio.h>

LinkedList* ll_create(size_t dataSize) {
    LinkedList* list = (LinkedList*)malloc(sizeof(LinkedList));
    if (!list) return NULL;

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->dataSize = dataSize;
    return list;
}

int ll_append(LinkedList* list, const void* data) {
    if (!list || !data) return 0;

    /* Allocate new node */
    LLNode* node = (LLNode*)malloc(sizeof(LLNode));
    if (!node) return 0;

    /* Allocate and copy data */
    node->data = malloc(list->dataSize);
    if (!node->data) {
        free(node);
        return 0;
    }
    memcpy(node->data, data, list->dataSize);
    node->next = NULL;

    /* Append to tail */
    if (!list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    list->count++;
    return 1;
}

void* ll_get_at(const LinkedList* list, int index) {
    if (!list || index < 0 || index >= list->count) return NULL;

    LLNode* curr = list->head;
    for (int i = 0; i < index && curr; i++) {
        curr = curr->next;
    }
    return curr ? curr->data : NULL;
}

int ll_size(const LinkedList* list) {
    if (!list) return 0;
    return list->count;
}

void ll_destroy(LinkedList* list) {
    if (!list) return;

    LLNode* curr = list->head;
    while (curr) {
        LLNode* tmp = curr;
        curr = curr->next;
        if (tmp->data) free(tmp->data);
        free(tmp);
    }
    free(list);
}
