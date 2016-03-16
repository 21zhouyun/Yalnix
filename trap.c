#include <stdlib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "trap.h"


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

void KernelCallHandler(ExceptionStackFrame *state){
    TracePrintf(0, "KernelHandler\n");
    Halt();
}

void ClockHandler(ExceptionStackFrame *state){
    TracePrintf(0, "ClockHandler\n");
    Halt();
}

void IllegalHandler(ExceptionStackFrame *state){
    TracePrintf(0, "IllegalHandler\n");
    Halt();
}

void MemoryHandler(ExceptionStackFrame *state){
    TracePrintf(0, "MemoryHandler\n");
    Halt();
}

void MathHandler(ExceptionStackFrame *state){
    TracePrintf(0, "MathHandler\n");
    Halt();
}

void TtyReceiveHandler(ExceptionStackFrame *state){
    TracePrintf(0, "TtyReceiveHandler\n");
    Halt();
}

void TtyTransmitHandler(ExceptionStackFrame *state){
    TracePrintf(0, "TtyTransmitHandler\n");
    Halt();
}


