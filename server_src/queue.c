#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "queue.h"

void queueInit(Queue *q, size_t memSize)
{
   q->sizeOfQueue = 0;
   q->memSize = memSize;
   q->head = q->tail = NULL;
}

int enqueue(Queue *q, const void *data)
{
    Node *newNode = (Node *)malloc(sizeof(Node));

    if(newNode == NULL)
    {
        return -1;
    }

    newNode->data = malloc(q->memSize);

    if(newNode->data == NULL)
    {
        free(newNode);
        return -1;
    }

    newNode->next = NULL;

    memcpy(newNode->data, data, q->memSize);

    if(q->sizeOfQueue == 0)
    {
        q->head = q->tail = newNode;
    }
    else
    {
        q->tail->next = newNode;
        q->tail = newNode;
    }

    q->sizeOfQueue++;
    return 0;
}

void dequeue(Queue *q, void *data)
{
    if(q->sizeOfQueue > 0)
    {
        Node *temp = q->head;
        memcpy(data, temp->data, q->memSize);

        if(q->sizeOfQueue > 1)
        {
            q->head = q->head->next;
        }
        else
        {
            q->head = NULL;
            q->tail = NULL;
        }

        q->sizeOfQueue--;
        free(temp->data);
        free(temp);
    }
}

void queuePeek(Queue *q, void *data)
{
    if(q->sizeOfQueue > 0)
    {
       Node *temp = q->head;
       memcpy(data, temp->data, q->memSize);
    }
}

void clearQueue(Queue *q)
{
  Node *temp;

  while(q->sizeOfQueue > 0)
  {
      temp = q->head;
      q->head = temp->next;
      free(temp->data);
      free(temp);
      q->sizeOfQueue--;
  }

  q->head = q->tail = NULL;
}

int getQueueSize(Queue *q)
{
    return q->sizeOfQueue;
}

bool isEmptyQueue(Queue *q) {
    return q->sizeOfQueue == 0;
}