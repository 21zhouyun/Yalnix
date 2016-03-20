#include "queue.h"
#include <stdlib.h>
#include <comp421/hardware.h>

queue* makeQueue(int size){
    if (size < DEFAULT_QUEUE_SIZE){
        size = DEFAULT_QUEUE_SIZE;
    }

    queue *q = (queue*)malloc(sizeof(queue));
    q->length=0;
    q->size=size;
    return q;
}

int enqueue(queue* q, void* value){
    // reached maximum size
    if (q->length >= q->size){
        return -1;
    }

    node* n = (node*)malloc(sizeof(node));
    n->value = value;
    n->previous = NULL;
    n->next = NULL;

    if (q->length == 0){
        q->head = n;
        q->tail = n;
    } else{
        n->previous = q->tail;
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
    TracePrintf(1, "dequeue node with previous %d, next %d\n", n->previous, n->next);
    q->head->previous = NULL;
    q->head = q->head->next;
    q->length--;
    return n;
}

/**
 * Pop a node from q.
 * n must be in q. Otherwise this function messed up!!
 */
int pop(queue* q, node* n){
    if (n->previous == NULL && n->next == NULL){
        //n is the only node in q
        q->head = NULL;
        q->tail = NULL;
    } else if (n->previous == NULL){
        //n is the head
        dequeue(q);
    } else if (n->next == NULL){
        //n is the tail
        n->previous->next = NULL;
        q->tail = n->previous;

        n->previous = NULL;
    } else{
        // a node in between
        n->previous->next = n->next;
        n->next->previous = n->previous;

        n->previous = NULL;
        n->next = NULL;
    }

    q->length --;
    return 0;
}
