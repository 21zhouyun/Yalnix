#include "memutil.h"
#include "include/hardware.h"
#include "include/yalnix.h"

int PID = 0;

/**
 * Invalidate a page table
 */
struct pte* invalidate_page_table(struct pte* page_table){
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
struct pte* initialize_page_table_region0(struct pte* page_table) {
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
struct pcb* make_pcb(struct pcb* parent){
    struct pcb* pcb_ptr;
    struct pte page_table[PAGE_TABLE_LEN];
    struct pte* pte_ptr = page_table;

    // Initiate pcb
    pcb_p = (struct pcb *)malloc(sizeof(struct pcb));
    if (pcb_p == NULL) {
        TracePrintf(0, "Error initializing pcb");
    }

    // Initialize page table
    pte_ptr = invalidate_page_table(pte_ptr);
    pte_ptr = initialize_page_table_region0(pte_ptr);

    pcb_ptr->pid = PID++;
    pcb_ptr->process_state = 0;
    pcb_ptr->context = NULL;
    pcb_ptr->parent = parent;
    pcb_ptr->children = make_queue();
    pcb_ptr->frame = NULL;
    pcb_ptr->pc_next = NULL;

    return pcb_ptr;
}