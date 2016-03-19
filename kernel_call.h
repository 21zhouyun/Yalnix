#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int KernelFork(void);
int KernelExec(char *filename, char **argvec);
void KernelExit(int status) __attribute__ ((noreturn));
int KernelWait(int *status_ptr);
int KernelGetPid(void);
int KernelBrk(void *addr);
int KernelDelay(int clock_ticks, ExceptionStackFrame *frame);
int KernelTtyRead(int tty_id, void *buf, int len);
int KernelTtyWrite(int tty_id, void *buf, int len);