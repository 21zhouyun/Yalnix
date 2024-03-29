#include <comp421/hardware.h>
#include <comp421/yalnix.h>

SavedContext *MySwitchFunc(SavedContext *ctxp, void *p1, void *p2);

SavedContext *ForkSwitchFunc(SavedContext *ctxp, void *p1, void *p2);

void SwitchToNextProc(int state);