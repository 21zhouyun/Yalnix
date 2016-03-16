#ifndef YALNIX_MEMUTIL_H
#define YALNIX_MEMUTIL_H

#include "include/hardware.h"
#include "include/yalnix.h"
#include "queue.h"
#include <stdbool.h>

#define GET_PFN(addr) (((long) addr & PAGEMASK) >> PAGESHIFT)

#define PFN_INVALID -1
#define KERNEL_TABLE_OFFSET 512

struct frame* free_frames;//global array of all free frames
int num_frames;
int num_free_frames;

struct pcb{
    unsigned int pid;
    int process_state;
    SavedContext *context;
    struct pcb* parent;
    queue* children;
    ExceptionStackFrame *frame;
    void *pc_next; //program counter

    struct pte* page_table_ptr; // address space of this process
} pcb;

struct frame{
    bool free;
};

struct pte* make_page_table();
struct pte* invalidate_page_table(struct pte* page_table);
struct pte* initialize_user_page_table(struct pte* page_table);

struct pcb* make_pcb(struct pcb* parent);

int initialize_frames(int num_free_frames);
int set_frame(int index, bool state);
int get_free_frame();

#endif //YALNIX_MEMUTIL_H
