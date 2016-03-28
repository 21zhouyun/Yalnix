#include <stdlib.h>
#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "trap.h"
#include "kernel_call.h"
#include "memutil.h"
#include "queue.h"
#include "initialize.h"
#include "ctxswitch.h"


int ticks = 0;

void *trap_vector[TRAP_VECTOR_SIZE];


void InitTrapVector() {
    int i;
    for (i = 0; i < TRAP_VECTOR_SIZE; i++)
        trap_vector[i] = NULL;

    trap_vector[TRAP_KERNEL] = KernelCallHandler;
    trap_vector[TRAP_CLOCK] = ClockHandler;
    trap_vector[TRAP_ILLEGAL] = IllegalHandler;
    trap_vector[TRAP_MEMORY] = MemoryHandler;
    trap_vector[TRAP_MATH] = MathHandler;
    trap_vector[TRAP_TTY_RECEIVE] = TtyReceiveHandler;
    trap_vector[TRAP_TTY_TRANSMIT] = TtyTransmitHandler;

    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) &trap_vector);
    TracePrintf(0, "Done initializing trap vector\n");
}

void KernelCallHandler(ExceptionStackFrame *frame){
    /**
     * arguments beginnint in regs[1]
     * return value in regs[0]
     */
    TracePrintf(0, "KernelHandler\n");
    switch(frame->code){
        case YALNIX_FORK:
            TracePrintf(1, "FORK\n");
            frame->regs[0] = ForkHandler();
            break;
        case YALNIX_EXEC:
            TracePrintf(1, "EXEC\n");
            frame->regs[0] = ExecHandler(frame, (char *) frame->regs[1], (char **) frame->regs[2]);
            break;
        case YALNIX_EXIT:
            TracePrintf(1, "EXIT\n");
            ExitHandler(frame->regs[1]);
            break;
        case YALNIX_WAIT:
            TracePrintf(1, "WAIT\n");
            frame->regs[0] = WaitHandler((int *) frame->regs[1]);
            break;
        case YALNIX_GETPID:
            TracePrintf(1, "GET PID\n");
            frame->regs[0] = GetPidHandler();
            break;
        case YALNIX_DELAY:
            TracePrintf(1, "DELAY PID %d\n", current_pcb->pid);
            frame->regs[0] = DelayHandler(frame->regs[1], frame);
            break;
        case YALNIX_BRK:
            TracePrintf(1, "BRK PID 0x%x.\n", current_pcb->pid);
            frame->regs[0] = BrkHandler((void *) frame->regs[1]);
            break;
        case YALNIX_TTY_READ:
            TracePrintf(1, "TTY_READ\n");
            TracePrintf(1, "id %d, buf %s, len %d\n", frame->regs[1], frame->regs[2], frame->regs[3]);
            frame->regs[0] = TtyReadHandler(frame->regs[1], (void *) frame->regs[2], frame->regs[3]);
            break;
        case YALNIX_TTY_WRITE:
            TracePrintf(1, "TTY_WRITE\n");
            TracePrintf(1, "id %d, buf %s, len %d\n", frame->regs[1], frame->regs[2], frame->regs[3]);
            frame->regs[0] = TtyWriteHandler(frame->regs[1], (void *) frame->regs[2], frame->regs[3]);
            break;
        default:
            TracePrintf(1, "Unknow kernel call!");
    }
}

void ClockHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "ClockHandler\n");
    // for each process in delay queue, decrement delay count
    node* current = delay_q->head;
    struct pcb* cpcb;
    while (current != NULL){
        cpcb = (struct pcb*)current->value;
        TracePrintf(0, "Checking delayed pid %d (%d)\n", cpcb->pid, cpcb->delay_remain);
        cpcb->delay_remain--;
        if (cpcb->delay_remain <= 0){
            // move this to ready queue
            pop(delay_q, current);
            TracePrintf(0, "Move pid %d to ready queue. deplay queue length: %d\n", cpcb->pid, delay_q->length);
            enqueue_ready(cpcb);
        }

        current = current->next;
    }

    // see if we are in idle
    if ((current_pcb->pid == 0 && ready_q->length > 0) || 
        (current_pcb->pid > 0 && (++ticks % 2 == 0))){ // round robin algo
        SwitchToNextProc(RUNNING);
    }
}

void IllegalHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "IllegalHandler\n");
    fprintf(stderr, "TRAP_ILLEGAL for pid: %d, error code: %d\n", current_pcb->pid, frame->code);
    current_pcb->process_state = TERMINATED;
    current_pcb->exit_status = ERROR;
    freeProcess(current_pcb);
}

void MemoryHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "MemoryHandler\n");

    int numPagesToGrow, i, pfn, vpn;
    void *addr = DOWN_TO_PAGE(frame->addr);
    bool stackGrew = false;
    TracePrintf(2, "Memory address that caused the TRAP_MEMORY: %p\n", addr);
    if (addr < current_pcb->user_stack_limit && addr > current_pcb->current_brk) {
        TracePrintf(2, "MemoryHandler: Trying to grow user stack for process %d.\n", current_pcb->pid);
        stackGrew = true;
        numPagesToGrow = ((int)(current_pcb->user_stack_limit - addr)) >> PAGESHIFT;
        vpn = (int) current_pcb->user_stack_limit >> PAGESHIFT;

        /* Make sure we don't grow into the "red zone". */
        if (current_pcb->page_table[vpn - 1].valid == 0) {
            for (i = 0; i < numPagesToGrow; ++i) {
                --vpn;
                if (current_pcb->page_table[vpn - 1].valid == 1) {
                    TracePrintf(2, "MemoryHandler: Keep growing would be going to red zone. Stop. \n");
                    stackGrew = false;
                    break;
                }
                pfn = getFreeFrame();
                if (pfn == -1) {
                    TracePrintf(2, "MemoryHandler: Not enough physical memory for user stack to grow.\n");
                    stackGrew = false;
                    break;
                }
                current_pcb->user_stack_limit -= PAGESIZE;
                current_pcb->page_table[vpn].pfn = pfn;
                current_pcb->page_table[vpn].uprot = PROT_ALL;
                current_pcb->page_table[vpn].kprot = PROT_ALL;
                current_pcb->page_table[vpn].valid = 1;
            }
        } else {
            stackGrew = false;
            TracePrintf(2, "If we grow this stack we're in redzone.\n");

        }
        
    }

    if (!stackGrew) {
        // Terminate the current running process.
        fprintf(stderr, "TRAP_MEMORY: MemoryHandler terminating process pid %d,"
            "type of disallowed memory access: %d\n", current_pcb->pid,
            frame->code);
        current_pcb -> process_state = TERMINATED;
        current_pcb->exit_status = ERROR;
        freeProcess(current_pcb);
    }
}

void MathHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "MathHandler\n");
    fprintf(stderr, "TRAP_MATH for pid: %d, error code: %d\n", current_pcb->pid, frame->code);
    current_pcb->process_state = TERMINATED;
    current_pcb->exit_status = ERROR;
    freeProcess(current_pcb);
}

void TtyReceiveHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "TtyReceiveHandler\n");
    int tty_id = frame->code;
    struct tty* terminal = &terminals[tty_id];
    int read_len;

    char *buf = (char*)malloc(sizeof(char) * TERMINAL_MAX_LINE);
    //read_len includes '\n'
    read_len = TtyReceive(tty_id, (void*)buf, TERMINAL_MAX_LINE);

    struct read_buf* input_buf = (struct read_buf*)malloc(sizeof(struct read_buf));
    input_buf->buf = buf;
    input_buf->len = read_len;

    enqueue(terminal->read_buf_q, (void*)input_buf);

    // unblock a blocked read process
    if (terminal->read_q->length > 0){
        node* n = dequeue(terminal->read_q);
        struct pcb* process_pcb = (struct pcb*)(n->value);
        enqueue_ready(process_pcb);
        free(n);
    }
}

void TtyTransmitHandler(ExceptionStackFrame *frame){
    TracePrintf(0, "TtyTransmitHandler from tty %d\n", frame->code);
    int tty_id = frame->code;
    struct tty* terminal = &terminals[tty_id];

    // unblock the current writing process
    struct pcb* process_pcb = terminal->write_pcb;
    terminal->write_pcb = NULL;
    
    free(terminal->write_buf);
    TracePrintf(1, "Finish write request for pid %d\n", process_pcb->pid);
    enqueue_ready(process_pcb);

    // unblock a blocked write process
    if (terminal->write_q->length > 0){
        node* n = dequeue(terminal->write_q);
        enqueue_ready((struct pcb*)(n->value));
        free(n);
    }
}


