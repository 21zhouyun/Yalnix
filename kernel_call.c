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
    
}