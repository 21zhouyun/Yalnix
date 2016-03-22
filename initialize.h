#include <comp421/hardware.h>
#include <comp421/yalnix.h>

bool vm_enable;

int SetKernelBrk(void *addr);

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args);

int InitFrames(unsigned int pmem_size);
int InitPageTable();

struct pcb* MakeProcess(char* name, ExceptionStackFrame *frame, char **cmd_args, struct pcb* process_pcb);
struct pcb* MakeIdle(ExceptionStackFrame *frame, struct pcb* process_pcb);