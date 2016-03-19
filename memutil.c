#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "memutil.h"
#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/* Globals */
int PID = 0;

/**
* make a new invalidated page table
*/
struct pte* makePageTable(){
    struct pte* new_page_table = (struct pte*)malloc(sizeof(struct pte) * PAGE_TABLE_LEN);
    new_page_table = invalidatePageTable(new_page_table);
    return new_page_table;
}

/**
 * Invalidate a page table
 */
struct pte* invalidatePageTable(struct pte* page_table){
    int i;
    for (i = 0; i < PAGE_TABLE_LEN; i++) {
        page_table[i].valid = 0;
        page_table[i].kprot = PROT_NONE;
        page_table[i].uprot = PROT_NONE;
        page_table[i].pfn = PFN_INVALID;
    }
    return page_table;
}

/**
 * Initialize a page table for Init process
 */
struct pte* initializeInitPageTable(struct pte* page_table) {
    int i;
    int limit = GET_VPN(KERNEL_STACK_LIMIT);
    int base = GET_VPN(KERNEL_STACK_BASE);

    //validate kernel stack
    for(i = base; i < limit; i++) {
        page_table[i].valid = 1;
        // VM shouldn't be enabled for this.
        page_table[i].pfn = i;
        page_table[i].kprot = (PROT_READ|PROT_WRITE);
        page_table[i].uprot = (PROT_NONE);
        setFrame(i, false);
    }
    return page_table;
}

/**
 * Copy the current process' kernel stack to the given page table
 */
struct pte* initializeUserPageTable(struct pte* page_table) {
    int i;
    int limit = GET_VPN(KERNEL_STACK_LIMIT);
    int base = GET_VPN(KERNEL_STACK_BASE);

    for(i = base; i < limit; i++) {
        page_table[i].valid = 1;
        page_table[i].pfn = getFreeFrame();
        page_table[i].kprot = (PROT_READ|PROT_WRITE);
        page_table[i].uprot = (PROT_NONE);
    }

    //copyKernelStackIntoTable(page_table);

    return page_table;
}

// copy the kernel stack of the current process into the given page table
int copyKernelStackIntoTable(struct pte *page_table){
    int i;
    int limit = GET_VPN(KERNEL_STACK_LIMIT);
    int base = GET_VPN(KERNEL_STACK_BASE);

    TracePrintf(1, "Copy current process stack\n");

    for(i = base; i < limit ; i++) {
        //HACK: map the pfn to the top of Region 1
        kernel_page_table[i - GET_VPN(VMEM_0_BASE)].valid = 1;
        kernel_page_table[i - GET_VPN(VMEM_0_BASE)].pfn = page_table[i].pfn;
        TracePrintf(1, "Mapped PFN %x to VPN %x\n", page_table[i].pfn, i - GET_VPN(VMEM_0_BASE));
        //TODO: better protection?
        kernel_page_table[i - GET_VPN(VMEM_0_BASE)].kprot = (PROT_READ|PROT_WRITE);
        kernel_page_table[i - GET_VPN(VMEM_0_BASE)].uprot = (PROT_READ|PROT_WRITE);
    }

    // copy the current process's kernel stack
    for(i = base; i < limit; i++) {
        TracePrintf(1, "Memcpy from %x to %x\n", i << PAGESHIFT, PAGE_TABLE_LEN * PAGESIZE + (i << PAGESHIFT));
        memcpy(PAGE_TABLE_LEN * PAGESIZE + (i << PAGESHIFT), i << PAGESHIFT, PAGESIZE);
    }

    for(i = base; i < limit; i++) {
        kernel_page_table[i].valid = 0;
    }

    return 0;
}

/**
 * initialize a new pcb.
 * Assume the page_table, if not NULL, is already initialize
 * in the correct way.
 */
struct pcb* makePCB(struct pcb* parent, struct pte* page_table){
    struct pcb* pcb_ptr;

    // THIS IS PROBABLY WRONG!
    // if (page_table == NULL){
    //     page_table = makePageTable();
    //     page_table = initializeUserPageTable(page_table);
    // }

    // Initiate pcb
    pcb_ptr = (struct pcb *)malloc(sizeof(struct pcb));

    pcb_ptr->pid = PID++;
    pcb_ptr->process_state = NOT_LOADED;
    pcb_ptr->delay_remain = 0;
    pcb_ptr->context = malloc(sizeof(SavedContext));
    pcb_ptr->parent = parent;
    pcb_ptr->children = makeQueue(MAX_NUM_CHILDREN);
    pcb_ptr->user_stack_limit = USER_STACK_LIMIT;
    pcb_ptr->current_brk = VMEM_0_BASE;
    pcb_ptr->page_table = page_table;

    return pcb_ptr;
}

/**
 * Initialize a list of frames
 */
int initializeFrames(int num_of_free_frames){
    free_frames = (struct frame*)malloc(sizeof(struct frame) * num_of_free_frames);
    num_frames = num_of_free_frames;
    num_free_frames = num_of_free_frames;
    int i;

    for (i = 0; i < num_free_frames; i++){
        free_frames[i].free = true;
    }

    return 0;
}

/**
 * Set the free state of the frame
 */
int setFrame(int index, bool state){
    free_frames[index].free = state;
    if (state == true){
        num_free_frames++;
    } else {
        num_free_frames--;
    }
    return num_free_frames;
}

/**
 * Greedily get the first free frame
 * TODO: change this back to i=0
 */
int getFreeFrame(){
    int i;
    for (i = MEM_INVALID_PAGES; i < num_frames; i++){
        if (free_frames[i].free == true){
            setFrame(i, false);
            return i;
        }
    }
    return -1;
}

int initializeQueues(){
    ready_q = makeQueue(MAX_QUEUE_SIZE);
    waiting_q = makeQueue(MAX_QUEUE_SIZE);
    delay_q = makeQueue(MAX_QUEUE_SIZE);
    return 0;
}

int enqueue_ready(struct pcb* process_pcb){
    enqueue(ready_q, process_pcb);
    TracePrintf(1, "Enqueued pid %d into ready queue (%d)\n", process_pcb->pid, ready_q->length);
    return 0;
}
int enqueue_waiting(struct pcb* process_pcb){
    enqueue(waiting_q, process_pcb);
    TracePrintf(1, "Enqueued pid %d into waiting queue (%d)\n", process_pcb->pid, ready_q->length);
    return 0;
}
int enqueue_delay(struct pcb* process_pcb){
    enqueue(delay_q, process_pcb);
    TracePrintf(1, "Enqueued pid %d into delay queue (%d)\n", process_pcb->pid, ready_q->length);
    return 0;
}
struct pcb* dequeue_ready(){
    if (ready_q->length == 0){
        return idle_pcb;
    }
    return (struct pcb*)(dequeue(ready_q)->value);
}
struct pcb* dequeue_waiting(){
    return (struct pcb*)(dequeue(waiting_q)->value);
}
struct pcb* dequeue_delay(){
    return (struct pcb*)(dequeue(delay_q)->value);
}
