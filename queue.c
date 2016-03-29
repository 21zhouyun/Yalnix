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

void* dequeue(queue* q){
    if (q == NULL || q->length == 0){
        return NULL;
    }

    node* n = q->head;
    q->head->previous = NULL;
    q->head = q->head->next;
    q->length--;

    void* value = n->value;
    free(n);

    return value;
}

/**
 * Pop a node from q.
 * n must be in q. Otherwise this function messed up!!
 */
int pop(queue* q, node* n){
    if (q->head == n && q->tail == n){
        //n is the only node in q
        TracePrintf(1, "ONLY NODE\n");
        q->head = NULL;
        q->tail = NULL;
        q->length --;
    } else if (q->head == n){
        //n is the head
        TracePrintf(1, "DEQUEUE\n");
        dequeue(q);
    } else if (q->tail == n){
        //n is the tail
        TracePrintf(1, "TAIL\n");
        n->previous->next = NULL;
        q->tail = n->previous;

        n->previous = NULL;
        q->length --;
    } else{
        // a node in between
        TracePrintf(1, "IN BETWEEN\n");
        n->previous->next = n->next;
        n->next->previous = n->previous;

        n->previous = NULL;
        n->next = NULL;
        q->length --;
    }
    return 0;
}
