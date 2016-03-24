#include <comp421/hardware.h>
#include "memutil.h"
#include "ctxswitch.h"
#include "initialize.h"
#include <stdlib.h>

int GetPidHandler(void){
    return current_pcb->pid;
}

int DelayHandler(int clock_ticks, ExceptionStackFrame *frame){
    if (clock_ticks == 0){
        return 0;
    }
    // Delay current process
    current_pcb->delay_remain = clock_ticks;
    enqueue_delay(current_pcb);
    struct pcb* next_pcb = dequeue_ready();
    TracePrintf(1, "Context Switch to pid %d\n", next_pcb->pid);
    TracePrintf(1, "Current context %d\n", current_pcb->context);

    // if (next_pcb->pid == 0 && next_pcb->process_state == NOT_LOADED){    
    //     // init a SavedContext for idle
    //     ContextSwitch(MySwitchFunc, next_pcb->context, next_pcb, NULL);
    //     TracePrintf(1, "Initialized idle context first time.\n");
    // }

    ContextSwitch(MySwitchFunc, current_pcb->context, current_pcb, next_pcb);

    if (next_pcb->pid == 0 && next_pcb->process_state == NOT_LOADED){
        TracePrintf(1, "Load idle first time.\n");
        next_pcb = MakeIdle(frame, next_pcb);
    }

    return 0;
}

int BrkHandler(void *addr){
    int numPagesToChange, i, vpn, pfn;
    TracePrintf(1, "Brk() for pid %d\n", current_pcb->pid);
    addr = (void *) UP_TO_PAGE(addr);

    // "- PAGESIZE" to make sure heap does not extend to user stack
    if ((addr - VMEM_0_BASE) < MEM_INVALID_SIZE || addr > (current_pcb->user_stack_limit - PAGESIZE)) {
        return ERROR;
    }

    if (addr < current_pcb->current_brk) {
        TracePrintf(1, "Deallocate to addr: %p\n", addr);
        numPagesToChange = ((int)(current_pcb->current_brk - addr)) >> PAGESHIFT;
        vpn = (int)current_pcb->current_brk >> PAGESHIFT;
        for (i = 0; i < numPagesToChange; ++i) {
            --vpn;
            setFrame(current_pcb->page_table[vpn].pfn, true);
            current_pcb->page_table[vpn].valid = 0;
        }
        current_pcb->current_brk = addr;
    } else {
        TracePrintf(1, "Allocate to addr: %p, current: %p\n", addr, current_pcb->current_brk);
        numPagesToChange = ((int)(addr - current_pcb->current_brk)) >> PAGESHIFT;
        vpn = (int) current_pcb->current_brk >> PAGESHIFT;
        for (i = 0; i < numPagesToChange; ++i) {
            pfn = getFreeFrame();
            if (pfn == -1) {
                TracePrintf(1, "Not enough physical memory to grow user heap for pid %d.\n", current_pcb->pid);
                return ERROR;
            }
            current_pcb->page_table[vpn].pfn = pfn;
            //TODO: Check if these prot bits are right
            current_pcb->page_table[vpn].uprot = PROT_ALL;
            current_pcb->page_table[vpn].kprot = PROT_ALL;
            current_pcb->page_table[vpn].valid = 1;
            ++vpn;
        }
        current_pcb->current_brk = addr;
    }
    return 0;
}

int ForkHandler(void){
    // initialize basic page table and pcb for the child process
    struct pte *child_page_table = makePageTable();
    child_page_table = initializeUserPageTable(child_page_table);
    struct pcb *child_pcb = makePCB(current_pcb, child_page_table);
    int child_pid = child_pcb->pid;

    enqueue(current_pcb->children, child_pcb);
    child_pcb->process_state = LOADED;
    TracePrintf(1, "initialized child process %x for current process %x\n",
                child_pcb->pid, current_pcb->pid);

    ContextSwitch(ForkSwitchFunc, current_pcb->context, current_pcb, child_pcb);

    // even after the switch, the kernel stack stays the say, so the pid
    // should be preserved.
    if (current_pcb->pid == child_pid){
        TracePrintf(1, "Return in child\n");
        return 0;
    } else {
        TracePrintf(1, "Return in parent\n");
        return child_pcb->pid;
    }
}

int ExecHandler(ExceptionStackFrame *frame, char *filename, char **argvec) {
    TracePrintf(1, "508 is: %d", current_pcb->page_table[508].pfn);
    LoadProgram(filename, argvec, current_pcb, frame);
    TracePrintf(1, "Exec failed.\n");
    return ERROR;
}


void ExitHandler(int status) {
    struct pcb *child;
    queue* children = current_pcb->children;
    current_pcb->exit_status = status;

    while (children->length > 0) {
        child = dequeue(children)->value;
        child->parent = NULL;
    }

    current_pcb->process_state = TERMINATED;
    TracePrintf(1, "Calling freeProcess");
    freeProcess(current_pcb);

}

int WaitHandler(int *status_ptr) {

    int retVal;
    if (current_pcb->children->length == 0) {
        return ERROR;
    }
    TracePrintf(1, "WaitHandler\n");

    struct pcb *next = dequeue(current_pcb->children)->value;
    TracePrintf(1, "WaitHandler %d\n", next->exit_status);

    while (next->process_state != TERMINATED) {
        enqueue(current_pcb->children, next);
        next = dequeue(current_pcb->children)->value;
    }

    *status_ptr = next->exit_status;
    retVal = next->pid;
    free(next);
    return retVal;
}