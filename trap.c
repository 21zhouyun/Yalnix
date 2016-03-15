#include <include/yalnix.h>
#include <include/hardware.h>
#include "trap.h"


void *trapVector[TRAP_VECTOR_SIZE];

void InitTrapVector() {
    for (int i = 0; i < TRAP_VECTOR_SIZE; i++)
        trapVector[i] = &InvalidTrapHandler;

    trapVector[TRAP_KERNEL] = KernelCallHandler;
    trapVector[TRAP_CLOCK] = ClockHandler;
    trapVector[TRAP_ILLEGAL] = IllegalHandler;
    trapVector[TRAP_MEMORY] = MemoryHandler;
    trapVector[TRAP_MATH] = MathHandler;
    trapVector[TRAP_TTY_RECEIVE] = TtyReceiveHandler;
    trapVector[TRAP_TTY_TRANSMIT] = TtyTransmitHandler;

    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) &trapVector);
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