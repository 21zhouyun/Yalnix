#define DEFAULT_QUEUE_SIZE 8

typedef struct Node{
    struct Node* next;
    void* value;
} node;

typedef struct Queue{
    node* head;
    node* tail;
    unsigned int length;
    unsigned int size;
} queue;

queue* makeQueue(int size);
int enqueue(queue* q, void* value);
node* dequeue(queue* q);

