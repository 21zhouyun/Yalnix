#include <comp421/yalnix.h>
#include <comp421/hardware.h>
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