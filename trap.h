#include <comp421/yalnix.h>
#include <comp421/hardware.h>

extern void *trapVector[];

/* Trap vector initializer and trap handlers */
extern void InitTrapVector(void);
extern void KernelCallHandler(UserContext *);
extern void ClockHandler(UserContext *);
extern void IllegalHandler(UserContext *);
extern void MemoryHandler(UserContext *);
extern void MathHandler(UserContext *);
extern void TtyReceiveHandler(UserContext *);
extern void TtyTransmitHandler(UserContext *);
extern void InvalidTrapHandler(UserContext *);