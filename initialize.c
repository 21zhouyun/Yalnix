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
struct pte* init_page_table; // page table for init process
struct pte* idle_page_table; // page table for idle process

// pcb
struct pcb* init_pcb; 

// kernel brk
void *kernel_brk;

// externs
extern int LoadProgram(char *name, char **args, struct pcb* program_pcb);

int SetKernelBrk(void *addr) {
    int numNeededPages, pfn, vpn, i;

    if (vm_enable) {
        
        if (addr < kernel_brk) {
            return -1;
        } else if (addr == kernel_brk) {
            return 0;
        } else {
            addr = (void *) UP_TO_PAGE(addr);
            numNeededPages = (addr - kernel_brk) / PAGESIZE;
            for (i = 0; i < numNeededPages; ++i) {
                pfn = getFreeFrame();
                if (pfn == -1)
                    return -1;
                vpn = ((kernel_brk - VMEM_1_BASE)>>PAGESHIFT) + i;
                kernel_page_table[vpn]->valid = 1;
                kernel_page_table[vpn]->pfn = pfn;
                kernel_page_table[vpn]->kprot = (PROT_READ | PROT_WRITE);
                   
            }
            kernel_brk = addr;
            return 0;
        }
    } else {
        TracePrintf(1, "virtual memory is not enabled.\n");
        kernel_brk = (void *) UP_TO_PAGE(addr);
    }
    return 0;
}

void KernelStart(ExceptionStackFrame *frame,
    unsigned int pmem_size, void *orig_brk, char **cmd_args){

    kernel_brk = (void *) UP_TO_PAGE(orig_brk);

    InitTrapVector();

    // before initialize page table, initialize a frame list
    int num_of_free_frames = pmem_size/PAGESIZE;
    initializeFrames(num_of_free_frames);
    TracePrintf(1, "There are %d free frames available\n", num_of_free_frames);
    TracePrintf(1, "Frame list length: %d\n", num_free_frames);

    // initialize page table
    InitPageTable();

    // load idle first so we run init first
    // the idle page table is alligned
    idle_pcb = MakeProcess("idle", frame, cmd_args, idle_page_table);
    
    // the init page table is just a normal user page table.
    init_page_table = makePageTable();
    init_page_table = initializeUserPageTable(init_page_table);
    init_pcb = MakeProcess("init", frame, cmd_args, init_page_table);
    
    initializeQueues();
    enqueue_ready(init_pcb);
    // enqueue_ready(idle_pcb);

    current_pcb = dequeue_ready();
    TracePrintf(1, "Dequeued process with pid %d from ready queue\n", current_pcb->pid);
}



int InitPageTable(){
    int i;
    kernel_page_table = makePageTable();
    idle_page_table = makePageTable();
    idle_page_table = initializeUserPageTable(idle_page_table);

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

    WriteRegister( REG_PTR0, (RCS421RegVal) idle_page_table);
    WriteRegister( REG_PTR1, (RCS421RegVal) kernel_page_table);


    WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);
    vm_enable = true;

    return 0;
}

struct pcb* MakeProcess(char* name, ExceptionStackFrame *frame, char **cmd_args, struct pte* page_table){
    struct pcb* process_pcb = makePCB(NULL, page_table);
    process_pcb->pc = frame->pc;
    process_pcb->sp = frame->sp;
    process_pcb->psr = frame->psr;
    process_pcb->frame = frame;

    TracePrintf(1, "Finished creating PCB for pid %d with pc: %d, sp: %d, psr: %d\n", process_pcb->pid, process_pcb->pc, process_pcb->sp, process_pcb->psr);
    //Before loading the program, switch in its page table.
    //WARNING, This is a hack that only works for idle and init initialization!!!
    WriteRegister( REG_PTR0, (RCS421RegVal) page_table);

    // Load the program.
    if(LoadProgram(name, cmd_args, process_pcb) != 0) {
        return NULL;
    }
    if (vm_enable) {
        WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    }
    TracePrintf(1, "Process initialized at pid %d\n", process_pcb->pid);
    
    return process_pcb;
}
