#ifndef YALNIX_TRAP_H
#define YALNIX_TRAP_H

#include "include/hardware.h"
#include "include/yalnix.h"

#include "trap.h"

extern void *trapVector[];

/* Trap vector initializer and trap handlers */
extern void InitTrapVector(void);
extern void KernelCallHandler(ExceptionStackFrame *state);
extern void ClockHandler(ExceptionStackFrame *state);
extern void IllegalHandler(ExceptionStackFrame *state);
extern void MemoryHandler(ExceptionStackFrame *state);
extern void MathHandler(ExceptionStackFrame *state);
extern void TtyReceiveHandler(ExceptionStackFrame *state);
extern void TtyTransmitHandler(ExceptionStackFrame *state);
extern void InvalidTrapHandler(ExceptionStackFrame *state);

#endif //YALNIX_TRAP_H
