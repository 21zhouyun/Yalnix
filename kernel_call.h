#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int ForkHandler(void);
int ExecHandler(ExceptionStackFrame *frame, char *filename, char **argvec);
void ExitHandler(int status);
int WaitHandler(int *status_ptr);
int GetPidHandler(void);
int BrkHandler(void *addr);
int DelayHandler(int clock_ticks, ExceptionStackFrame *frame);
int TtyReadHandler(int tty_id, void *buf, int len);
int TtyWriteHandler(int tty_id, void *buf, int len);

