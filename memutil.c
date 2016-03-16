#include "include/hardware.h"
#include "include/yalnix.h"
#include "memutil.h"
#include <stdlib.h>

/* Globals */
int PID = 0;

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
        (page_table + i) -> valid = 0;
        (page_table + i) -> kprot = PROT_NONE;
        (page_table + i) -> uprot = PROT_NONE;
        (page_table + i) -> pfn = PFN_INVALID;
    }
    return page_table;
}

/**
 * Initialize a page table for region 0
 */
struct pte* initializeUserPageTable(struct pte* page_table) {
    int i;
    int limit = GET_PFN(KERNEL_STACK_LIMIT);
    int base = GET_PFN(KERNEL_STACK_BASE);

    for(i = base; i <= limit; i++) {
        (page_table + i) -> valid = 1;
        (page_table + i) -> pfn = i;
        (page_table + i) -> kprot = (PROT_READ|PROT_WRITE);
        (page_table + i) -> uprot = (PROT_NONE);
    }
    return page_table;
}

/**
 * initialize a new pcb.
 */
struct pcb* makePCB(struct pcb* parent){
    struct pcb* pcb_ptr;
    struct pte* pte_ptr = makePageTable();

    // Initiate pcb
    pcb_ptr = (struct pcb *)malloc(sizeof(struct pcb));

    // Initialize page table
    pte_ptr = invalidatePageTable(pte_ptr);
    pte_ptr = initializeUserPageTable(pte_ptr);

    pcb_ptr->pid = PID++;
    pcb_ptr->process_state = 0;
    pcb_ptr->context = NULL;
    pcb_ptr->parent = parent;
    pcb_ptr->children = makeQueue(10);
    pcb_ptr->frame = NULL;
    pcb_ptr->pc_next = NULL;

    return pcb_ptr;
}

/**
 * Initialize a list of frames
 */
int initialize_frames(int num_of_free_frames){
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
int set_frame(int index, bool state){
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
 */
int get_free_frame(){
    int i;
    for (i = 0; i < num_frames; i++){
        if (free_frames[i].free == true){
            set_frame(i, false);
            return i;
        }
    }
    return -1;
}
