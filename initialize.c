#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <comp421/loadinfo.h>

#include <stdbool.h>
#include <stdio.h>

#include "memutil.h"
#include "trap.h"
#include "initialize.h"
#include "ctxswitch.h"

// whether we have enabled virtual memory
extern bool vm_enable = false;

// pcb
struct pcb* init_pcb; 

// kernel brk
void *kernel_brk;

char **args;


int SetKernelBrk(void *addr) {
    int numNeededPages, pfn, vpn, i;

    if (vm_enable) {
        
        if (addr < kernel_brk || addr > VMEM_1_LIMIT) {
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
                vpn = ((int)(kernel_brk - VMEM_1_BASE)>>PAGESHIFT) + i;
                kernel_page_table[vpn].valid = 1;
                kernel_page_table[vpn].pfn = pfn;
                kernel_page_table[vpn].kprot = (PROT_READ | PROT_WRITE);
                   
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

    args = cmd_args;
    kernel_brk = (void *) UP_TO_PAGE(orig_brk);


    InitTrapVector();

    // before initialize page table, initialize a frame list
    int num_of_free_frames = pmem_size/PAGESIZE;
    initializeFrames(num_of_free_frames);
    TracePrintf(1, "There are %d free frames available\n", num_of_free_frames);
    TracePrintf(1, "Frame list length: %d\n", num_free_frames);

    // initialize page table
    InitPageTable();

    // VM enabled

    initializeQueues();

    // initialize pcb
    idle_pcb = makePCB(NULL, idle_page_table);
    init_pcb = makePCB(NULL, init_page_table);

    TracePrintf(1, "[DEBUG]init %x (%x)\n", init_pcb->page_table, init_pcb->physical_page_table);
    TracePrintf(1, "[DEBUG]idle %x (%x)\n", idle_pcb->page_table, idle_pcb->physical_page_table);
    // Since init's pcb is initialize after idle's, the current
    // temporary
    TracePrintf(1, "%s\n", cmd_args[1]);
    init_pcb = MakeProcess(cmd_args[0], frame, cmd_args, init_pcb);
    current_pcb = init_pcb;

    // init a SavedContext for idle
    ContextSwitch(MySwitchFunc, idle_pcb->context, idle_pcb, NULL);
    // idle_pcb = MakeProcess("idle", frame, cmd_args, idle_pcb);

}



int InitPageTable(){
    int i;
    // These are physical addresses
    kernel_page_table = makeKernelPageTable();
    TracePrintf(1, "Made kernel page table.\n");

    int kernel_heap_limit = GET_VPN(kernel_brk);
    int kernel_text_limit = GET_VPN(&_etext);

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
    for(i=GET_VPN(VMEM_1_BASE); i < kernel_text_limit; i++) {
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->valid = 1;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->pfn = i;
        (kernel_page_table + i - KERNEL_TABLE_OFFSET)->kprot = (PROT_READ | PROT_EXEC);
        setFrame(i, false);
    }

    init_page_table = makePageTable();
    idle_page_table = makePageTable();

    TracePrintf(1, "Made page tables\n");

    init_page_table = initializeInitPageTable(init_page_table);
    idle_page_table = initializeUserPageTable(idle_page_table);

    TracePrintf(1, "Made init page table at %x\n", init_page_table);

    WriteRegister( REG_PTR0, (RCS421RegVal) init_page_table);
    WriteRegister( REG_PTR1, (RCS421RegVal) kernel_page_table);


    WriteRegister( REG_VM_ENABLE, (RCS421RegVal) 1);
    vm_enable = true;

    return 0;
}

struct pcb* MakeProcess(char* name, ExceptionStackFrame *frame, char **cmd_args, struct pcb* process_pcb){
    //Before loading the program, switch in its page table.
    //WARNING, This is a hack that only works for idle and init initialization!!!
    process_pcb->page_table = mapToTemp((void*)process_pcb->physical_page_table, kernel_temp_vpn);
    TracePrintf(1, "%x\n", process_pcb->physical_page_table);

    WriteRegister(REG_PTR0, (RCS421RegVal) (process_pcb->physical_page_table));
    TracePrintf(1, "Flush!\n");
    // WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) TLB_FLUSH_ALL);

    TracePrintf(1, "Finished creating PCB for pid %d, psr: %d\n", process_pcb->pid, process_pcb->psr);

    // Load the program.
    if(LoadProgram(name, cmd_args, process_pcb, frame) != 0) {
        return NULL;
    }

    process_pcb->process_state = LOADED;
    process_pcb->psr = frame->psr;

    if (vm_enable) {
        WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    }
    TracePrintf(1, "Process initialized at pid %d\n", process_pcb->pid);
    
    return process_pcb;
}

struct pcb* MakeIdle(ExceptionStackFrame *frame, struct pcb* process_pcb){
    //Before loading the program, switch in its page table.
    //WARNING, This is a hack that only works for idle and init initialization!!!
    mapToTemp((void*)process_pcb->physical_page_table, kernel_temp_vpn);
    WriteRegister(REG_PTR0, (RCS421RegVal) process_pcb->physical_page_table);
    TracePrintf(1, "Flush!\n");
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    TracePrintf(1, "Finished creating PCB for pid %d, psr: %d\n", process_pcb->pid, process_pcb->psr);

    // Load the program.
    if(LoadProgram("idle", args, process_pcb, frame) != 0) {
        return NULL;
    }
    process_pcb->process_state = LOADED;
    process_pcb->psr = frame->psr;

    if (vm_enable) {
        WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    }
    TracePrintf(1, "Process initialized at pid %d\n", process_pcb->pid);
    
    return process_pcb;
}
