#include "queue.h"
#include <stdlib.h>

queue* make_queue(){
    queue *q = (queue*)malloc(sizeof(queue));
    q->length=0;
    q.size=DEFAULT_QUEUE_SIZE;
    return q;
}

int enqueue(queue* q, void* value){
    // reached maximum size
    if (q->length >= q->size){
        return -1;
    }

    node* n = (node*)malloc(sizeof(node));
    n->value = value;
    n->next = NULL;

    if (q->length == 0){
        q->head = n;
        q->tail = n;
    } else{
        q->tail->next = n;
        q->tail = n;
    }
    q->length ++;

    return 0;
}

node* dequeue(queue* q){
    if (q == NULL || q->length == 0){
        return NULL;
    }

    node* n = q->head;

    q->head = q->head->next;
    q->length--;
    return n;
}