/**
 * c implementation of a queue, based on a list approach
 * source here:
 * https://codereview.stackexchange.com/questions/141238/implementing-a-generic-queue-in-c 
 * visited 13-6-2019
 * adapted to this particular project
 */

#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

#include "types.h"

typedef struct Node
{
  tlv_request_t *data;
  struct Node *next;
} Node;

typedef struct QueueList
{
    int sizeOfQueue;
    size_t memSize;
    Node *head;
    Node *tail;
} Queue;

void queueInit(Queue *q, size_t memSize);
int enqueue(Queue *, const void *);
void dequeue(Queue *, void *);
void queuePeek(Queue *, void *);
void clearQueue(Queue *);
int getQueueSize(Queue *);
bool isEmptyQueue(Queue *);

#endif /* QUEUE_H_INCLUDED */
