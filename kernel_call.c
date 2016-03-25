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

    ContextSwitch(MySwitchFunc, current_pcb->context, current_pcb, next_pcb);
    
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
    TracePrintf(1, "508 is: %d\n", current_pcb->page_table[508].pfn);
    return MakeProcess(filename, frame, argvec, current_pcb);
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
    TracePrintf(1, "Calling freeProcess\n");

    freeProcess(current_pcb);

}

int WaitHandler(int *status_ptr) {
    if (current_pcb->children->length == 0) {
        return ERROR;
    }
    TracePrintf(1, "WaitHandler\n");

    while (current_pcb->children->length > 0){
        node* current = current_pcb->children->head;
        struct pcb* child_pcb;
        // loop over each children
        while (current != NULL){
            child_pcb = (struct pcb*)current->value;
            if (child_pcb->process_state == TERMINATED){
                pop(current_pcb->children, current);
                *status_ptr = child_pcb->exit_status;
                free(child_pcb);
                return child_pcb->pid;
            }
            current = current->next;
        }

        // Context switch to another process
        struct pcb* next_pcb = dequeue_ready();
        TracePrintf(1, "Context Switch to pid %d\n", next_pcb->pid);

        ContextSwitch(MySwitchFunc, current_pcb->context, current_pcb, next_pcb);

    }

}