#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#include <trap.h>

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