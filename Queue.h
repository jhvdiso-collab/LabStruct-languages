#ifndef QUEUE_H
#define QUEUE_H

struct QueueNode {
    int data;
    QueueNode* next;
};

struct Queue {
    QueueNode* head;
    QueueNode* tail;
};

void initQueue(Queue& q);
void pushQueue(Queue& q, int value);
int popQueue(Queue& q);
bool isQueueEmpty(const Queue& q);
void freeQueue(Queue& q);

#endif