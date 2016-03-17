#include <comp421/hardware.h>
#include <comp421/yalnix.h>

int SetKernelBrk(void *addr);

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args);

int InitFrames(unsigned int pmem_size);
int InitPageTable();
int MakeInitProcess(ExceptionStackFrame *frame, char **cmd_args);

