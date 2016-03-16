#ifndef YALNIX_QUEUE_H
#define YALNIX_QUEUE_H

#define DEFAULT_QUEUE_SIZE 8

typedef struct Node{
    struct node* next;
    void* value;
} node;

typedef struct Queue{
    node* head;
    node* tail;
    unsigned int length;
    unsigned int size;
} queue;

queue* make_queue(int size);
int enqueue(queue* q, void* value);
node* dequeue(queue* q);

#endif //YALNIX_QUEUE_H
