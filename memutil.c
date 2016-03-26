#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "memutil.h"
#include "queue.h"
#include "initialize.h"
#include "ctxswitch.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/* Globals */
int PID = 0;

// this maps the current process' page table to its physical location
extern const long kernel_temp_vpn = GET_VPN(VMEM_1_LIMIT) - GET_VPN(VMEM_1_BASE) - 1;
// use this for any temporary change, no need to change it back
extern const long kernel_temp_vpn2 = GET_VPN(VMEM_1_LIMIT) - GET_VPN(VMEM_1_BASE) - 2;
extern const long copier = GET_VPN(VMEM_1_LIMIT) - GET_VPN(VMEM_1_BASE) - 3;

struct pte* free_to_take_page_table = NULL;

struct pte* makeKernelPageTable(){

    kernel_page_table = (struct pte*)malloc(sizeof(struct pte) * PAGE_TABLE_LEN);

    invalidatePageTable(kernel_page_table);

    return kernel_page_table;
}

/**
 * Take a free physical frame and make two page table out of it.
 * @return the physical address of the page table.
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
        long addr1 = DOWN_TO_PAGE(free_frame << PAGESHIFT);
        long addr2 = addr1 + PAGE_TABLE_SIZE;

        new_page_table = (struct pte*)addr1;
        free_to_take_page_table = (struct pte*)addr2;

        TracePrintf(1, "Created new page table at %x (%x) and %x (%x)\n", new_page_table, GET_PFN(new_page_table),
        free_to_take_page_table, GET_PFN(free_to_take_page_table));
    }

    invalidatePageTable(new_page_table);

    return new_page_table;
}

/**
 * Invalidate a given page table
 * @param physical address of the page table
 * @return the same physical address of the invalidated page table
 */
struct pte* invalidatePageTable(struct pte* page_table){
    TracePrintf(1, "invalidate page table starting at %x\n", page_table);
    long i;
    struct pte* page_table_ptr = (struct pte*)mapToTemp((void*)page_table, kernel_temp_vpn2);
    
    for (i = 0; i < PAGE_TABLE_LEN; i++) {
        page_table_ptr[i].valid = 0;
        page_table_ptr[i].kprot = PROT_NONE;
        page_table_ptr[i].uprot = PROT_NONE;
        page_table_ptr[i].pfn = PFN_INVALID;
    }
    return page_table;
}

/**
 * initialize a page table for init process
 * @param the physical address of the page table
 * @return the same physical address of the page table
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
 * initialize a page table for a user process
 * @param the physical address of the page table
 * @return the same physical address of the page table
 */
struct pte* initializeUserPageTable(struct pte* page_table) {
    long i;
    long limit = GET_VPN(KERNEL_STACK_LIMIT);
    long base = GET_VPN(KERNEL_STACK_BASE);

    struct pte* page_table_ptr = (struct pte*)mapToTemp((void*)page_table, kernel_temp_vpn2);

    for(i = base; i < limit; i++) {
        page_table_ptr[i].valid = 1;
        page_table_ptr[i].pfn = getFreeFrame();
        page_table_ptr[i].kprot = (PROT_READ|PROT_WRITE);
        page_table_ptr[i].uprot = (PROT_NONE);
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
    int i;
    //free page table pfn
    for (i = 0; i < PAGE_TABLE_LEN; i++){
        if (process_pcb->page_table[i].valid == 1){
            setFrame(process_pcb->page_table[i].pfn, true);
        }
    }
    //free page table
    setHalfFrame(process_pcb->physical_page_table, true);

    SwitchToNextProc(TERMINATED);
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
 * @param the physical address of the page table to be filled
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
        copyPage(i, page_table_ptr[i].pfn);
    }

    return 0;
}

/**
 * Copy every valid page table to the given page table
 * @param the physical address of the given page table
 * @return
 */
int copyRegion0IntoTable(struct pte *page_table){
    long i;
    struct pte* parent_page_table = current_pcb->page_table;

    struct pte *page_table_ptr = mapToTemp((void*)page_table, kernel_temp_vpn2);

    for (i = 0; i < PAGE_TABLE_LEN; i++){
        page_table_ptr[i].valid = parent_page_table[i].valid;
        page_table_ptr[i].kprot = parent_page_table[i].kprot;
        page_table_ptr[i].uprot = parent_page_table[i].uprot;

        if (parent_page_table[i].valid == 1){
            // allocate a free frame for the given page table
            page_table_ptr[i].pfn = getFreeFrame();
            TracePrintf(1, "Copy vpn %x into child pfn %x\n", i, page_table_ptr[i].pfn);
            copyPage(i, page_table_ptr[i].pfn);
        }
        
    }

    return 0;
}

/**
 * Copy a page indicated by the given vpn from region0 into the given pfn
 * @param the vpn for the source page from region0
 * @param the pfn for the destination page
 * @return
 */
int copyPage(long vpn, long pfn){
    unsigned int original_valid = kernel_page_table[copier].valid;
    unsigned int original_pfn = kernel_page_table[copier].pfn;
    unsigned int original_kprot = kernel_page_table[copier].kprot;
    unsigned int original_uprot = kernel_page_table[copier].uprot;

    kernel_page_table[copier].valid = 1;
    kernel_page_table[copier].pfn = pfn;
    TracePrintf(1, "Mapped PFN %x to VPN %x\n", pfn, copier);
    //TODO: better protection?
    kernel_page_table[copier].kprot = (PROT_READ|PROT_WRITE);
    kernel_page_table[copier].uprot = (PROT_READ|PROT_WRITE);

    WriteRegister(REG_TLB_FLUSH, VMEM_1_BASE + DOWN_TO_PAGE(copier << PAGESHIFT));

    memcpy((void*)(VMEM_1_BASE + DOWN_TO_PAGE(copier << PAGESHIFT)), (void*)(VMEM_0_BASE + (vpn << PAGESHIFT)), PAGESIZE);

    // restore
    kernel_page_table[copier].valid = original_valid;
    kernel_page_table[copier].pfn = original_pfn;
    kernel_page_table[copier].kprot = original_kprot;
    kernel_page_table[copier].uprot = original_uprot;

    return 0;
}

/**
 * initialize a new pcb.
 * Assume the page_table, if not NULL, is already initialize
 * in the correct way.
 * @param the parent pcb
 * @param the physical address of the page table for the current process
 */
struct pcb* makePCB(struct pcb* parent, struct pte* page_table){
    struct pcb* pcb_ptr;
    struct pte* page_table_ptr = (struct pte*)mapToTemp((void*)page_table, kernel_temp_vpn2);

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
 * @param the number of free frames
 */
int initializeFrames(int num_of_free_frames){
    free_frames = (struct frame*)malloc(sizeof(struct frame) * num_of_free_frames);
    num_frames = num_of_free_frames;
    num_free_frames = num_of_free_frames;
    int i;

    for (i = 0; i < num_free_frames; i++){
        free_frames[i].free = true;
        free_frames[i].upper = true;
        free_frames[i].lower = true;
    }

    return 0;
}

/**
 * Set the free state of the frame
 * @param the index of the frame
 * @param the state of the frame
 */
int setFrame(int index, bool state){
    free_frames[index].free = state;
    free_frames[index].upper = state;
    free_frames[index].lower = state;
    if (state == true){
        num_free_frames++;
        TracePrintf(1, "Mark pfn %x as free\n", index);
    } else {
        num_free_frames--;
        TracePrintf(1, "Mark pfn %x as not free\n", index);
    }
    return num_free_frames;
}

/**
 * Given a physical address, set the upper/lower half of the frame
 * to state
 * @param  addr  physical address
 * @param  state boolean state
 */
int setHalfFrame(long addr, bool state){
    long pfn = GET_PFN(addr);
    long offset = GET_OFFSET(addr);

    if (offset < PAGE_TABLE_SIZE){
        free_frames[pfn].upper = state;
    } else {
        free_frames[pfn].lower = state;
    }

    if (free_frames[pfn].upper == free_frames[pfn].lower){
        // if both the upper and lower half of the frame have the same state
        // the frame itself must share the same state
        setFrame(pfn, state);
    }
}

/**
 * Greedily get the first free frame
 */
long getFreeFrame(){
    long i = MEM_INVALID_PAGES;
    if (vm_enable == true){
        i = 0;
    }
    for (; i < num_frames; i++){
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
 * @param physical address
 * @param vpn of the temporary pte
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

    if (vm_enable == true){
        TracePrintf(1, "FLUSH!\n");
        WriteRegister(REG_TLB_FLUSH, (void*)(VMEM_1_BASE + (temp_vpn << PAGESHIFT)));
    }

    return (void*)((VMEM_1_BASE + (temp_vpn << PAGESHIFT)) + offset);
}

int initializeQueues(){
    ready_q = makeQueue(MAX_QUEUE_SIZE);
    delay_q = makeQueue(MAX_QUEUE_SIZE);
    return 0;
}

int enqueue_ready(struct pcb* process_pcb){
    process_pcb->process_state = READY;
    enqueue(ready_q, process_pcb);
    TracePrintf(1, "Enqueued pid %d into ready queue (%d)\n", process_pcb->pid, ready_q->length);
    return 0;
}
int enqueue_delay(struct pcb* process_pcb){
    process_pcb->process_state = DELAYED;
    enqueue(delay_q, process_pcb);
    TracePrintf(1, "Enqueued pid %d into delay queue (%d)\n", process_pcb->pid, ready_q->length);
    return 0;
}
struct pcb* dequeue_ready(){
    struct pcb* result;
    if (ready_q->length == 0){
        return idle_pcb;
    }
    node* n =dequeue(ready_q);
    result = (struct pcb*)n->value;
    free(n);
    return result;
}
struct pcb* dequeue_delay(){
    struct pcb* result;
    if (delay_q->length == 0){
        return idle_pcb;
    }
    node* n =dequeue(delay_q);
    result = (struct pcb*)n->value;
    free(n);
    return result;
}


void initializeTerminals(){
    int i;
    terminals = (struct tty*)malloc(sizeof(struct tty) * NUM_TERMINALS);
    for (i = 0; i < NUM_TERMINALS; i++){
        terminals[i].write_pcb = NULL;
        terminals[i].write_q = makeQueue(MAX_QUEUE_SIZE);
    }
}


/**
 * print debug info of the page table.
 * @param page_table virtual address of the page table
 */
void debugPageTable(struct pte *page_table){
    int i;
    for(i = 0; i < PAGE_TABLE_LEN; i++){
        TracePrintf(1, "[DEBUG]vpn %x: valid %x, kprot %x, uprot %x, pfn %x\n",
            i, page_table[i].valid, page_table[i].kprot, page_table[i].uprot,
            page_table[i].pfn);
    }
}

/**
 * print debug info of the kernel stack
 * @param page_table virtual address of the page table
 */
void debugKernelStack(struct pte *page_table){
    long i;
    long limit = GET_VPN(KERNEL_STACK_LIMIT);
    long base = GET_VPN(KERNEL_STACK_BASE);

    for(i = base; i < limit; i++){
        TracePrintf(1, "[DEBUG]vpn %x: valid %x, kprot %x, uprot %x, pfn %x\n",
            i, page_table[i].valid, page_table[i].kprot, page_table[i].uprot,
            page_table[i].pfn);
    }
}
