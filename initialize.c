#include "include/hardware.h"
#include "include/yalnix.h"
#include <stdbool.h>
#include <stdio.h>

#include "initialize.h"
#include "trap.h"
#include "memutil.h"

// whether we have enabled virtual memory
bool vm_enable = false;

// page tables
struct pte* kernel_page_table;
struct pte* user_page_table;
struct pte* init_page_table;

// kernel brk
void *kernel_brk;

int SetKernelBrk(void *addr) {
    if (VM_ENABLE) {
        // TODO
    } else {
        TracePrintf(1, "virtual memory is not enabled.");
        kernel_brk = addr;
    }
    return 0;
}

void KernelStart(ExceptionStackFrame *frame,
    unsigned int pmem_size, void *orig_brk, char **cmd_args){

    InitTrapVector();
    // init page tables
    KERNEL_HEAP_LIMIT =orig_brk;

    // before initialize page table, initialize a frame list
    int num_of_free_frames = pmem_size/PAGESIZE;
    initialize_frames(num_of_free_frames);
    TracePrintf(1, "There are %d free frames available", num_of_free_frames);
    TracePrintf(1, "Frame queue length: %d", num_free_frames);

    // initialize page table
    InitPageTable();
}



int InitPageTable(){
    int i;
    kernel_page_table = make_page_table();
    user_page_table = make_page_table();
    init_page_table = make_page_table();

    int kernel_heap_limit = GET_PFN(KERNEL_HEAP_LIMIT);
    int kernel_text_limit = GET_PFN(&_etext);

    TracePrintf(1, "Kernel heap limit %d", kernel_heap_limit);
    TracePrintf(1, "Kernel text limit %d", kernel_text_limit);

    TracePrintf(1, "Kernel Page Table Layout");
    for(i=kernel_text_limit; i <= kernel_heap_limit + 1; i++) {
        TracePrintf(1, "%d", kernel_page_table + i - KERNEL_TABLE_OFFSET);
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->valid = 1;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->pfn = i;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->kprot = (PROT_READ | PROT_WRITE);
        set_frame(i, false);
    }
    for(i=GET_PFN(VMEM_1_BASE); i < kernel_text_limit; i++) {
        TracePrintf(1, "%d", kernel_page_table + i - KERNEL_TABLE_OFFSET);
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->valid = 1;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->pfn = i;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->kprot = (PROT_READ | PROT_EXEC);
        set_frame(i, FRAME_NOT_FREE);
    }

    // User page table
    user_page_table = initialize_user_page_table(user_page_table);

    WriteRegister( REG_PTR0, (RCS421RegVal) user_page_table);
    WriteRegister( REG_PTR1, (RCS421RegVal) kernel_page_table);


    WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);
    vm_enable = true;

}
