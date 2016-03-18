#include <stdlib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "trap.h"
#include "kernel_call.h"
#include "memutil.h"
#include "queue.h"
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
        case YALNIX_GETPID:
            TracePrintf(1, "GET PID\n");
            frame->regs[0] = GetPid();
            break;
        case YALNIX_DELAY:
            TracePrintf(1, "DELAY PID %d\n", current_pcb->pid);
            frame->regs[0] = Delay(frame->regs[1]);
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
        ContextSwitch(MySwitchFunc, current_pcb->context, current_pcb, next_pcb);
    }
}

void IllegalHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "IllegalHandler\n");
    Halt();
}

void MemoryHandler(ExceptionStackFrame *frame){
    //TracePrintf(0, "MemoryHandler\n");
    // Halt();
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


