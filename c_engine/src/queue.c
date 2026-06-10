#include "../include/queue.h"
#include <stdlib.h>

IntQueue *queue_create(int capacity) {
    IntQueue *q;
    if (capacity <= 0) {
        return NULL;
    }
    q = (IntQueue *)malloc(sizeof(IntQueue));
    if (!q) {
        return NULL;
    }
    q->data = (int *)malloc((size_t)capacity * sizeof(int));
    if (!q->data) {
        free(q);
        return NULL;
    }
    q->capacity = capacity;
    q->head = 0;
    q->size = 0;
    return q;
}

void queue_destroy(IntQueue *q) {
    if (!q) {
        return;
    }
    free(q->data);
    free(q);
}

void queue_clear(IntQueue *q) {
    if (q) {
        q->head = 0;
        q->size = 0;
    }
}

int queue_is_empty(const IntQueue *q) {
    return !q || q->size == 0;
}

int queue_push(IntQueue *q, int value) {
    int tail;
    if (!q || q->size >= q->capacity) {
        return 0;
    }
    tail = (q->head + q->size) % q->capacity;
    q->data[tail] = value;
    q->size++;
    return 1;
}

int queue_pop(IntQueue *q) {
    int value;
    if (queue_is_empty(q)) {
        return -1;
    }
    value = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return value;
}
