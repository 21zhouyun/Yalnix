#include <stdlib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "trap.h"
#include "kernel_call.h"
#include "memutil.h"
#include "queue.h"
#include "initialize.h"
#include "ctxswitch.h"


void *trap_vector[TRAP_VECTOR_SIZE];


void InitTrapVector() {
    int i;
    for (i = 0; i < TRAP_VECTOR_SIZE; i++)
        trap_vector[i] = NULL;

    trap_vector[TRAP_KERNEL] = KernelCallHandler;
    trap_vector[TRAP_CLOCK] = ClockHandler;
    trap_vector[TRAP_ILLEGAL] = IllegalHandler;
    trap_vector[TRAP_MEMORY] = MemoryHandler;
    trap_vector[TRAP_MATH] = MathHandler;
    trap_vector[TRAP_TTY_RECEIVE] = TtyReceiveHandler;
    trap_vector[TRAP_TTY_TRANSMIT] = TtyTransmitHandler;

    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) &trap_vector);
    TracePrintf(0, "Done initializing trap vector\n");
}

void KernelCallHandler(ExceptionStackFrame *frame){
    /**
     * arguments beginnint in regs[1]
     * return value in regs[0]
     */
    TracePrintf(0, "KernelHandler\n");
    switch(frame->code){
        case YALNIX_FORK:
            TracePrintf(1, "FORK\n");
            frame->regs[0] = ForkHandler();
            break;
        case YALNIX_EXEC:
            break;
        case YALNIX_EXIT:
            break;
        case YALNIX_WAIT:
            break;
        case YALNIX_GETPID:
            TracePrintf(1, "GET PID\n");
            frame->regs[0] = GetPidHandler();
            break;
        case YALNIX_DELAY:
            TracePrintf(1, "DELAY PID %d\n", current_pcb->pid);
            frame->regs[0] = DelayHandler(frame->regs[1], frame);
            break;
        case YALNIX_BRK:
            TracePrintf(1, "BRK PID 0x%x.\n", current_pcb->pid);
            frame->regs[0] = BrkHandler(frame->regs[1]);
            break;
        case YALNIX_TTY_READ:
            break;
        case YALNIX_TTY_WRITE:
            break;
        default:
            TracePrintf(1, "Unknow kernel call!");
    }
}

void ClockHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "ClockHandler\n");
    // for each process in delay queue, decrement delay count
    node* current = delay_q->head;
    struct pcb* cpcb;
    while (current != NULL){
        cpcb = (struct pcb*)current->value;
        TracePrintf(1, "Checking delayed pid %d (%d)\n", cpcb->pid, cpcb->delay_remain);
        cpcb->delay_remain--;
        if (cpcb->delay_remain <= 0){
            // move this to ready queue
            pop(delay_q, current);
            enqueue_ready(cpcb);
        }

        current = current->next;
    }

    // see if we are in idle
    if (current_pcb->pid == 0 && ready_q->length > 0){
        struct pcb* next_pcb = dequeue_ready();
        TracePrintf(1, "Found ready pid %d, swith to it.\n", next_pcb->pid);
        ContextSwitch(MySwitchFunc, current_pcb->context, current_pcb, next_pcb);
    }
}

void IllegalHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "IllegalHandler\n");
    Halt();
}

void MemoryHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "MemoryHandler\n");

    int numPagesToGrow, i, pfn, vpn;
    void *addr = frame->addr;
    bool stackGrew = false;
    TracePrintf(2, "Memory address that caused the TRAP_MEMORY: %p\n", addr);
    if (addr < current_pcb->user_stack_limit && addr > current_pcb->current_brk) {
        TracePrintf(2, "MemoryHandler: Trying to grow user stack for process %d.\n", current_pcb->pid);
        stackGrew = true;
        numPagesToGrow = ((int)(current_pcb->user_stack_limit - addr)) >> PAGESHIFT;
        vpn = (int) current_pcb->user_stack_limit >> PAGESHIFT;

        /* Make sure we don't grow into the "red zone". */
        if (current_pcb->page_table[vpn - 1].valid == 0) {
            for (i = 0; i < numPagesToGrow; ++i) {
                --vpn;
                if (current_pcb->page_table[vpn - 1].valid == 1) {
                    TracePrintf(2, "MemoryHandler: Keep growing would be going to red zone. Stop. \n");
                    stackGrew = false;
                    break;
                }
                pfn = getFreeFrame();
                if (pfn == -1) {
                    TracePrintf(2, "MemoryHandler: Not enough physical memory for user stack to grow.\n");
                    stackGrew = false;
                    break;
                }
                
                current_pcb->page_table[vpn].pfn = pfn;
                current_pcb->page_table[vpn].uprot = PROT_ALL;
                current_pcb->page_table[vpn].kprot = PROT_ALL;
                current_pcb->page_table[vpn].valid = 1;
            }
        } else {
            stackGrew = false;
            TracePrintf(2, "If we grow this stack we're in redzone.\n");

        }
        
    }

    if (!stackGrew) {
        // TODO: see if there's a better way to do this?
        // TODO: free the physical memory of this thing?

        // Terminate the current running process.
        TracePrintf(2, "TRAP_MEMORY: MemoryHandler terminating process pid %d\n", current_pcb->pid);
        current_pcb -> process_state = TERMINATED;

        // Run the next ready process.
        struct pcb* next_pcb = dequeue_ready();
        TracePrintf(1, "Context Switch to pid %d\n", next_pcb->pid);

        if (next_pcb->pid == 0 && next_pcb->process_state == NOT_LOADED){    
            // init a SavedContext for idle
            ContextSwitch(MySwitchFunc, next_pcb->context, next_pcb, NULL);
        }

        ContextSwitch(MySwitchFunc, current_pcb->context, current_pcb, next_pcb);

        if (next_pcb->pid == 0 && next_pcb->process_state == NOT_LOADED){
            TracePrintf(1, "Load idle first time.\n");
            next_pcb = MakeIdle(frame, next_pcb);
        }
        
    }
}

void MathHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "MathHandler\n");
    Halt();
}

void TtyReceiveHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "TtyReceiveHandler\n");
    Halt();
}

void TtyTransmitHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "TtyTransmitHandler\n");
    Halt();
}


