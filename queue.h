#ifndef _queue_h
#define _queue_h

#define DEFAULT_QUEUE_SIZE 8

/**
 * NULL--head--(<-previous)--n--(next->)--tail--NULL
 */

typedef struct Node{
    struct Node* previous;
    struct Node* next;
    void* value;
} node;

typedef struct Queue{
    node* head;
    node* tail;
    unsigned int length; // the current length of the queue
    unsigned int size; // the fixed size of the queue
} queue;

queue* makeQueue(int size);
int enqueue(queue* q, void* value);
node* dequeue(queue* q);
int pop(queue* q, node* n);

#endif