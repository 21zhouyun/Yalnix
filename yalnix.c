#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#include "trap.h"

// Just for autocompletion purpose.
#include "yalnix.h"
#include "hardware.h"

void KernelStart(ExceptionStackFrame *frame,
    unsigned int pmem_size, void *orig_brk, char **cmd_args){

    InitTrapVector();
}