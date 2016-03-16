#include "include/yalnix.h"
#include "include/hardware.h"

int SetKernelBrk(void *addr);

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args);

int InitFrames(unsigned int pmem_size);

int InitPageTable();

