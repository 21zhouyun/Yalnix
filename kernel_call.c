#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "memutil.h"
#include "ctxswitch.h"

int GetPid(void){
    return current_pcb->pid;
}

int Delay(int clock_ticks){
    if (clock_ticks == 0){
        return 0;
    }
    // Delay current process
    current_pcb->delay_remain = clock_ticks;
    enqueue_delay(current_pcb);
    struct pcb* next_pcb = dequeue_ready();
    TracePrintf(1, "Context Switch to pid %d\n", next_pcb->pid);
    TracePrintf(1, "Current context %d\n", current_pcb->context);
    ContextSwitch(MySwitchFunc, next_pcb->context, current_pcb, next_pcb);
    return 0;
}
