#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int ForkHandler(void);
int ExecHandler(char *filename, char **argvec);
void ExitHandler(int status) __attribute__ ((noreturn));
int WaitHandler(int *status_ptr);
int GetPidHandler(void);
int BrkHandler(void *addr);
int DelayHandler(int clock_ticks);
int TtyReadHandler(int tty_id, void *buf, int len);
int TtyWriteHandler(int tty_id, void *buf, int len);