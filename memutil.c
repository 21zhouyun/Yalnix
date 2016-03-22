#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "memutil.h"
#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/* Globals */
int PID = 0;

extern const long kernel_temp_vpn = GET_VPN(VMEM_1_LIMIT) - GET_VPN(VMEM_1_BASE) - 1;
extern const long kernel_temp_vpn2 = GET_VPN(VMEM_1_LIMIT) - GET_VPN(VMEM_1_BASE) - 2;
extern const long kernel_copy_vpn = GET_VPN(VMEM_1_LIMIT) - GET_VPN(VMEM_1_BASE) - 3;

struct pte* free_to_take_page_table = NULL;

struct pte* makeKernelPageTable(){

    kernel_page_table = (struct pte*)malloc(sizeof(struct pte) * PAGE_TABLE_LEN);

    invalidatePageTable(kernel_page_table);

    return kernel_page_table;
}

/**
* make a new invalidated page table
*/
struct pte* makePageTable(){
    // new_page_table points to the physical adderss of a page table.
    // DO NOT USE IT DIRECTLY. Map it to Region 1's virtual page 511,
    // and then use that instead.
    struct pte* new_page_table;

    if (free_to_take_page_table != NULL){
        new_page_table = free_to_take_page_table;
        free_to_take_page_table = NULL;
    } else {
        long free_frame = getFreeFrame();
        // make the free frame into two page tables
        new_page_table = (struct pte*)(free_frame << PAGESHIFT);
        free_to_take_page_table = new_page_table + PAGE_TABLE_SIZE;
        TracePrintf(1, "Created new page table at %x and %x\n", new_page_table, free_to_take_page_table);
    }

    struct pte* page_table_ptr = (struct pte*)mapToTemp((void*)new_page_table, kernel_temp_vpn);
    invalidatePageTable(page_table_ptr);

    return new_page_table;
}

/**
 * Invalidate a page table.
 * Assume the given pointer is in virtual address
 */
struct pte* invalidatePageTable(struct pte* page_table){
    long i;
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
 * Assume the given pointer is in PHYSICAL address
 */
struct pte* initializeInitPageTable(struct pte* page_table) {
    long i;
    long limit = GET_VPN(KERNEL_STACK_LIMIT);
    long base = GET_VPN(KERNEL_STACK_BASE);

    //validate kernel stack
    for(i = base; i < limit; i++) {
        TracePrintf(1, "Validate Kernel stack at vpn %x\n", i);
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
 * Assume the given pointer is in virtual address
 */
struct pte* initializeUserPageTable(struct pte* page_table) {
    long i;
    long limit = GET_VPN(KERNEL_STACK_LIMIT);
    long base = GET_VPN(KERNEL_STACK_BASE);

    for(i = base; i < limit; i++) {
        page_table[i].valid = 1;
        page_table[i].pfn = getFreeFrame();
        page_table[i].kprot = (PROT_READ|PROT_WRITE);
        page_table[i].uprot = (PROT_NONE);
    }

    return page_table;
}

/**
 * Mark the frame in the given page table as free when a process is 
 * terminated or exited
 * @param
 * @return
 */
int freeProcess(struct pcb *process_pcb){
    //TODO: implement this shit.
    return 0;
}

/**
 * Copy the kernel stack of the current process into the given page table.
 * Assume the given page_table is PHYSICAL address.
 * 
 * Rationale:
 * Since this function is only used to copy the current process' page table
 * which is mapped at kernel_temp_vpn. We also need to map the physical page
 * table of the new process to some place in order to write into it.
 */
int copyKernelStackIntoTable(struct pte *page_table){
    long i;
    long limit = GET_VPN(KERNEL_STACK_LIMIT);
    long base = GET_VPN(KERNEL_STACK_BASE);

    struct pte *page_table_ptr = mapToTemp((void*)page_table, kernel_temp_vpn2);

    TracePrintf(1, "Copy current process stack from %x (%x) to %x (%x)\n", 
        current_pcb->physical_page_table, current_pcb->page_table,
        page_table, page_table_ptr);

    // copy the current process's kernel stack
    for(i = base; i < limit; i++) {
        copyPage(i, page_table_ptr);
    }

    return 0;
}

/**
 * Copy every valid page table to the given page table
 * @param
 * @return
 */
int copyRegion0IntoTable(struct pte *page_table){
    //Assume the no pfn is allocated for the given page table
    //TODO: check this
    long i;
    struct pte* parent_page_table = current_pcb->page_table;

    for (i = 0; i < PAGE_TABLE_LEN; i++){
        page_table[i].valid = parent_page_table[i].valid;
        page_table[i].kprot = parent_page_table[i].kprot;
        page_table[i].uprot = parent_page_table[i].uprot;

        if (parent_page_table[i].valid == 1){
            // allocate a free frame for the given page table
            page_table[i].pfn = getFreeFrame();
            TracePrintf(1, "Copy vpn %x into child pfn %x\n", i, page_table[i].pfn);
            copyPage(i, page_table);
        }
        
    }

    return 0;
}

/**
 * Copy a page indicated by the given vpn from the current process to the given page table.
 * @param
 * @param
 * @return
 */
int copyPage(long vpn, struct pte *page_table){
    unsigned int original_valid = kernel_page_table[kernel_copy_vpn].valid;
    unsigned int original_pfn = kernel_page_table[kernel_copy_vpn].pfn;
    unsigned int original_kprot = kernel_page_table[kernel_copy_vpn].kprot;
    unsigned int original_uprot = kernel_page_table[kernel_copy_vpn].uprot;

    kernel_page_table[kernel_copy_vpn].valid = 1;
    kernel_page_table[kernel_copy_vpn].pfn = page_table[vpn].pfn;
    TracePrintf(1, "Mapped PFN %x to VPN %x\n", page_table[vpn].pfn, kernel_copy_vpn);
    //TODO: better protection?
    kernel_page_table[kernel_copy_vpn].kprot = (PROT_READ|PROT_WRITE);
    kernel_page_table[kernel_copy_vpn].uprot = (PROT_READ|PROT_WRITE);

    memcpy((void*)(VMEM_1_BASE + (kernel_copy_vpn << PAGESHIFT)), (void*)(vpn << PAGESHIFT), PAGESIZE);

    // debug info
    // TracePrintf(1, "DEBUG COPY PAGE\n");
    // long offset;
    // for (offset = 0; offset < PAGESIZE; offset++){
    //     void* from_addr = (void*)((vpn << PAGESHIFT) + offset);
    //     void* to_addr = (void*)(VMEM_1_BASE + (kernel_copy_vpn << PAGESHIFT) + offset);

    //     int from_value = *(int*)from_addr;
    //     int to_value = *(int*)to_addr;
    //     //check each byte
    //     if (from_value != to_value){
    //         TracePrintf(1, "[ERROR] value mismatch %d (%x) %d (%x)[%x]\n", from_value, from_addr, to_value, to_addr, page_table + (vpn << PAGESHIFT) + offset);
    //     }
    // }

    // restore
    kernel_page_table[kernel_copy_vpn].valid = original_valid;
    kernel_page_table[kernel_copy_vpn].pfn = original_pfn;
    kernel_page_table[kernel_copy_vpn].kprot = original_kprot;
    kernel_page_table[kernel_copy_vpn].uprot = original_uprot;

    return 0;
}

/**
 * initialize a new pcb.
 * Assume the page_table, if not NULL, is already initialize
 * in the correct way.
 * Assume page_table is the physical address
 */
struct pcb* makePCB(struct pcb* parent, struct pte* page_table){
    struct pcb* pcb_ptr;
    struct pte* page_table_ptr = (struct pte*)mapToTemp((void*)page_table, kernel_temp_vpn);

    pcb_ptr = (struct pcb *)malloc(sizeof(struct pcb));

    pcb_ptr->pid = PID++;
    pcb_ptr->process_state = NOT_LOADED;
    pcb_ptr->delay_remain = 0;
    pcb_ptr->context = malloc(sizeof(SavedContext));
    pcb_ptr->parent = parent;
    pcb_ptr->children = makeQueue(MAX_NUM_CHILDREN);
    pcb_ptr->user_stack_limit = USER_STACK_LIMIT;
    pcb_ptr->physical_page_table = page_table;
    pcb_ptr->page_table = page_table_ptr;

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
 * TODO: add back invalid pages after VM is enabled
 */
long getFreeFrame(){
    long i;
    for (i = MEM_INVALID_PAGES; i < num_frames; i++){
        if (free_frames[i].free == true){
            setFrame(i, false);
            return i;
        }
    }
    return -1;
}

/**
 * Map the given pfn in physical address to the highest virtual page in Region1.
 * return the virtual address of the given physical address
 * @param
 */
void* mapToTemp(void* addr, long temp_vpn){
    long pfn = GET_PFN(addr);
    long offset = GET_OFFSET(addr);

    kernel_page_table[temp_vpn].valid = 1;
    kernel_page_table[temp_vpn].pfn = pfn;
    TracePrintf(1, "Mapped PFN %x to VPN %x temporarily\n", pfn, temp_vpn);
    //TODO: better protection?
    kernel_page_table[temp_vpn].kprot = (PROT_READ|PROT_WRITE);
    kernel_page_table[temp_vpn].uprot = (PROT_READ|PROT_WRITE);

    return (void*)((VMEM_1_BASE + (temp_vpn << PAGESHIFT)) + offset);
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


//debug
void debugPageTable(struct pte *page_table){
    int i;
    for(i = 0; i < PAGE_TABLE_LEN; i++){
        TracePrintf(1, "[DEBUG]vpn %x: valid %x, kprot %x, uprot %x, pfn %x\n",
            i, page_table[i].valid, page_table[i].kprot, page_table[i].uprot,
            page_table[i].pfn);
    }
}
