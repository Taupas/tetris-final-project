#ifndef TETRIS_QUEUE_H
#define TETRIS_QUEUE_H

/* 環形佇列（加分項目：queue）— 用於 7-bag 出塊 */

typedef struct IntQueue {
    int *data;
    int capacity;
    int head;
    int size;
} IntQueue;

IntQueue *queue_create(int capacity);
void queue_destroy(IntQueue *q);
int queue_push(IntQueue *q, int value);
int queue_pop(IntQueue *q);
int queue_is_empty(const IntQueue *q);
void queue_clear(IntQueue *q);

#endif
