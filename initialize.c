#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>

#include <stdbool.h>
#include <stdio.h>

#include "memutil.h"
#include "trap.h"
#include "initialize.h"

// whether we have enabled virtual memory
bool vm_enable = false;

// page tables
struct pte* kernel_page_table;
struct pte* user_page_table; // page table for init process

// kernel brk
void *kernel_brk;

// externs
extern int LoadProgram(char *name, char **args, struct pcb* program_pcb);

int SetKernelBrk(void *addr) {
    if (vm_enable) {
        // TODO
    } else {
        TracePrintf(1, "virtual memory is not enabled.\n");
        kernel_brk = addr;
    }
    return 0;
}

void KernelStart(ExceptionStackFrame *frame,
    unsigned int pmem_size, void *orig_brk, char **cmd_args){

    kernel_brk = orig_brk;

    InitTrapVector();

    // before initialize page table, initialize a frame list
    int num_of_free_frames = pmem_size/PAGESIZE;
    initializeFrames(num_of_free_frames);
    TracePrintf(1, "There are %d free frames available\n", num_of_free_frames);
    TracePrintf(1, "Frame list length: %d\n", num_free_frames);

    // initialize page table
    InitPageTable();
    MakeInitProcess(frame, cmd_args);
}



int InitPageTable(){
    int i;
    kernel_page_table = makePageTable();
    user_page_table = makePageTable();
    user_page_table = initializeUserPageTable(user_page_table);

    int kernel_heap_limit = GET_PFN(kernel_brk);
    int kernel_text_limit = GET_PFN(&_etext);

    TracePrintf(1, "Kernel heap limit %d\n", kernel_heap_limit);
    TracePrintf(1, "Kernel text limit %d\n", kernel_text_limit);

    // set up everything in region 1 except text section
    for(i=kernel_text_limit; i <= kernel_heap_limit + 1; i++) {
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->valid = 1;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->pfn = i;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->kprot = (PROT_READ | PROT_WRITE);
        setFrame(i, false);
    }

    // set up text section in region 1 
    for(i=GET_PFN(VMEM_1_BASE); i < kernel_text_limit; i++) {
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->valid = 1;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->pfn = i;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->kprot = (PROT_READ | PROT_EXEC);
        setFrame(i, false);
    }

    WriteRegister( REG_PTR0, (RCS421RegVal) user_page_table);
    WriteRegister( REG_PTR1, (RCS421RegVal) kernel_page_table);


    WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);
    vm_enable = true;

    return 0;
}

int MakeInitProcess(ExceptionStackFrame *frame, char **cmd_args){
    //create init program pcb
    //TODO: is this the correct way to init?
    struct pcb* init_pcb = makePCB(NULL, user_page_table);
    init_pcb->pc = frame->pc;
    init_pcb->sp = frame->sp;
    init_pcb->psr = frame->psr;
    init_pcb->frame = frame;

    TracePrintf(1, "Finished creating PCB for init process with pc: %d, sp: %d, psr: %d\n", init_pcb->pc, init_pcb->sp, init_pcb->psr);
    // Load the program.
    // TODO: load an idle program here and then load the init program
    // after that, test context switch.
    if(LoadProgram("init", cmd_args, init_pcb) != 0) {
        return -1;
    }
    if (vm_enable) {
        WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    }
    TracePrintf(1, "Init process initialized at pid %d\n", init_pcb->pid);
    TracePrintf(1, "New pc for init process %d\n", init_pcb->frame->pc);
    return 0;
}
