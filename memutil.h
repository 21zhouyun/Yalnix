#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "queue.h"
#include <stdbool.h>
#include <stdlib.h>

// These are the same thing. Keep both for legacy reason...
#define GET_VPN(addr) (((long)addr & PAGEMASK) >> PAGESHIFT)
#define GET_PFN(addr) (((long)addr & PAGEMASK) >> PAGESHIFT)
#define GET_OFFSET(addr) ((long)addr & PAGEOFFSET)

#define PFN_INVALID 0
#define KERNEL_TABLE_OFFSET 512

#define MAX_QUEUE_SIZE 1024
#define MAX_NUM_CHILDREN 8
// process states
#define NOT_LOADED -1 //default
#define RUNNING 0 //take care by ContextSwitch
#define DELAYED 1 //in delay queue 
#define TERMINATED 2 //take care by exit handler
#define READY 3 //take care by ready queue
#define EXITED 4 //take care by exit handler

struct frame* free_frames;//global array of all free frames
int num_frames;
int num_free_frames;

// The highest vpn in region1. Used as a temporary pte for user page tables.
const long kernel_temp_vpn;
const long kernel_temp_vpn2;

// Second highest vpn in region1. Used as a temporary pte for copying purpose.
const long kernel_copy_vpn;

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
    int exit_status;
    unsigned long psr;
    void *user_stack_limit;
    void *current_brk;
    struct pte* physical_page_table; // physical address to the page table (never change)
    struct pte* page_table; // virtual address to the page table
} pcb;

struct frame{
    bool free;
};

// manage page tables
struct pte* makeKernelPageTable();
struct pte* makePageTable();
struct pte* invalidatePageTable(struct pte *page_table);
struct pte* initializeInitPageTable(struct pte *page_table);
int copyKernelStackIntoTable(struct pte *page_table);
int copyRegion0IntoTable(struct pte *page_table);
int copyPage(long vpn, long pfn);
struct pte* initializeUserPageTable(struct pte *page_table);
int freeProcess(struct pcb *process_pcb);

struct pcb* makePCB(struct pcb *parent, struct pte* page_table);

// manage frames
int initializeFrames(int num_free_frames);
int setFrame(int index, bool state);
long getFreeFrame();
void* mapToTemp(void* addr, long temp_vpn);

// manage process queues
int initializeQueues();
int enqueue_ready(struct pcb* process_pcb);
int enqueue_waiting(struct pcb* process_pcb); //TODO: waiting queue is just useless?
int enqueue_delay(struct pcb* process_pcb);
//TODO: free node after dequeue
struct pcb* dequeue_ready();
struct pcb* dequeue_waiting();
struct pcb* dequeue_delay();

//debug
void debugPageTable(struct pte *page_table);
void debugKernelStack(struct pte *page_table);


