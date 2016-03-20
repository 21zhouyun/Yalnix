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

    if (next_pcb->pid == 0 && next_pcb->process_state == NOT_LOADED){    
        // init a SavedContext for idle
        ContextSwitch(MySwitchFunc, next_pcb->context, next_pcb, NULL);
    }

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