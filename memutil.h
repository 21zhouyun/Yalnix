#ifndef YALNIX_MEMUTIL_H
#define YALNIX_MEMUTIL_H

#include "include/hardware.h"
#include "include/yalnix.h"
#include "queue.h"

#define GET_PFN(addr) (((long) addr & PAGEMASK) >> PAGESHIFT)

#define PFN_INVALID -1
#define PAGE_TABLE1_OFFSET 512

struct pcb{
    unsigned int pid;
    int process_state;
    SavedContext *context;
    struct pcb* parent;
    queue* children;
    ExceptionStackFrame *frame;
    void *pc_next; //program counter

    struct pte* page_table_ptr; // address space of this process
};

struct pte* invalidate_page_table(struct pte* page_table);

struct pte* initialize_page_table_region0(struct pte* page_table);

struct pcb* make_pcb(struct pcb* parent);

#endif //YALNIX_MEMUTIL_H
