#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "queue.h"
#include <stdbool.h>

#define GET_VPN(addr) (((long) addr & PAGEMASK) >> PAGESHIFT)

#define PFN_INVALID -1
#define KERNEL_TABLE_OFFSET 512

#define MAX_QUEUE_SIZE 1024
#define MAX_NUM_CHILDREN 8
// process states
#define NOT_LOADED -1
#define LOADED 0

struct frame* free_frames;//global array of all free frames
int num_frames;
int num_free_frames;

// page tables
struct pte* kernel_page_table;
struct pte* init_page_table; // page table for init process
struct pte* idle_page_table; // page table for idle process

struct pcb* idle_pcb; //pcb for idle process
struct pcb* current_pcb; //pcb for current process

// process queues
// should be queues of pcbs
queue* ready_q;
queue* waiting_q;
queue* delay_q;

struct pcb{
    unsigned int pid;
    int process_state;
    int delay_remain;
    SavedContext *context;
    struct pcb* parent;
    queue* children;
    unsigned long psr;
    void *current_brk;
    struct pte* page_table; // address space of this process
} pcb;

struct frame{
    bool free;
};

// manage page tables
struct pte* makePageTable();
struct pte* invalidatePageTable(struct pte *page_table);
struct pte* initializeInitPageTable(struct pte *page_table);
int copyKernelStackIntoTable(struct pte *page_table);
struct pte* initializeUserPageTable(struct pte *page_table);

struct pcb* makePCB(struct pcb *parent, struct pte* page_table);

// manage frames
int initializeFrames(int num_free_frames);
int setFrame(int index, bool state);
int getFreeFrame();

// manage process queues
int initializeQueues();
int enqueue_ready(struct pcb* process_pcb);
int enqueue_waiting(struct pcb* process_pcb);
int enqueue_delay(struct pcb* process_pcb);
struct pcb* dequeue_ready();
struct pcb* dequeue_waiting();
struct pcb* dequeue_delay();


