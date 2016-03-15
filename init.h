#ifndef YALNIX_INIT_H
#define YALNIX_INIT_H

#include "include/yalnix.h"
#include "include/hardware.h"
#include "trap.h"

int SetKernelBrk(void *addr);

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args);

void InitPageTable();

#endif //YALNIX_INIT_H
