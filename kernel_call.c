#include <comp421/hardware.h>
#include "memutil.h"
#include "ctxswitch.h"
#include "initialize.h"
#include <stdlib.h>
#include <string.h>

int GetPidHandler(void){
    return current_pcb->pid;
}

int DelayHandler(int clock_ticks, ExceptionStackFrame *frame){
    if (clock_ticks == 0){
        return 0;
    }
    // Delay current process
    current_pcb->delay_remain = clock_ticks;
    enqueue_delay(current_pcb);
    SwitchToNextProc(DELAYED);
    return 0;
}

int BrkHandler(void *addr){
    int numPagesToChange, i, vpn, pfn;
    TracePrintf(1, "Brk() for pid %d\n", current_pcb->pid);
    addr = (void *) UP_TO_PAGE(addr);

    // "- PAGESIZE" to make sure heap does not extend to user stack
    if ((addr - VMEM_0_BASE) < MEM_INVALID_SIZE || addr > (current_pcb->user_stack_limit - PAGESIZE)) {
        return ERROR;
    }

    if (addr < current_pcb->current_brk) {
        TracePrintf(1, "Deallocate to addr: %p\n", addr);
        numPagesToChange = ((int)(current_pcb->current_brk - addr)) >> PAGESHIFT;
        vpn = (int)current_pcb->current_brk >> PAGESHIFT;
        for (i = 0; i < numPagesToChange; ++i) {
            --vpn;
            setFrame(current_pcb->page_table[vpn].pfn, true);
            current_pcb->page_table[vpn].valid = 0;
        }
        current_pcb->current_brk = addr;
    } else {
        TracePrintf(1, "Allocate to addr: %p, current: %p\n", addr, current_pcb->current_brk);
        numPagesToChange = ((int)(addr - current_pcb->current_brk)) >> PAGESHIFT;
        vpn = (int) current_pcb->current_brk >> PAGESHIFT;
        for (i = 0; i < numPagesToChange; ++i) {
            pfn = getFreeFrame();
            if (pfn == -1) {
                TracePrintf(1, "Not enough physical memory to grow user heap for pid %d.\n", current_pcb->pid);
                return ERROR;
            }
            TracePrintf(1, "map pfn %x to vpn %x\n", pfn, vpn);
            current_pcb->page_table[vpn].pfn = pfn;
            //TODO: Check if these prot bits are right
            current_pcb->page_table[vpn].uprot = PROT_ALL;
            current_pcb->page_table[vpn].kprot = PROT_ALL;
            current_pcb->page_table[vpn].valid = 1;
            ++vpn;
        }
        current_pcb->current_brk = addr;
    }
    return 0;
}

int ForkHandler(void){
    // initialize basic page table and pcb for the child process
    struct pte *child_page_table = makePageTable();
    child_page_table = initializeUserPageTable(child_page_table);
    struct pcb *child_pcb = makePCB(current_pcb, child_page_table);
    int child_pid = child_pcb->pid;

    enqueue(current_pcb->children, child_pcb);
    TracePrintf(1, "initialized child process %x for current process %x\n",
                child_pcb->pid, current_pcb->pid);

    ContextSwitch(ForkSwitchFunc, current_pcb->context, current_pcb, child_pcb);

    // even after the switch, the kernel stack stays the say, so the pid
    // should be preserved.
    if (current_pcb->pid == child_pid){
        TracePrintf(1, "Return in child\n");
        return 0;
    } else {
        TracePrintf(1, "Return in parent\n");
        return child_pcb->pid;
    }
}

int ExecHandler(ExceptionStackFrame *frame, char *filename, char **argvec) {
    TracePrintf(1, "508 is: %d\n", current_pcb->page_table[508].pfn);
    int return_val = LoadProgram(filename, argvec, current_pcb, frame);
    if (return_val == -1){
        return -1;
    } else if (return_val == -2){
        current_pcb->process_state = TERMINATED;
        current_pcb->exit_status = ERROR;
        TracePrintf(1, "ERROR cannot exec. Terminated current process!\n");
        freeProcess(current_pcb);
    }

    return 0;
}


void ExitHandler(int status) {
    struct pcb *child;
    queue* children = current_pcb->children;
    current_pcb->exit_status = status;

    while (children->length > 0) {
        child = (struct pcb*)dequeue(children);
        child->parent = NULL;
    }

    current_pcb->process_state = TERMINATED;
    TracePrintf(1, "Calling freeProcess\n");

    freeProcess(current_pcb);

}

int WaitHandler(int *status_ptr) {
    if (current_pcb->children->length == 0) {
        return ERROR;
    }
    TracePrintf(1, "WaitHandler\n");

    while (current_pcb->children->length > 0){
        node* current = current_pcb->children->head;
        struct pcb* child_pcb;
        // loop over each children
        while (current != NULL){
            child_pcb = (struct pcb*)current->value;
            if (child_pcb->process_state == TERMINATED){
                TracePrintf(1, "Pop child %x from children (%d)\n", child_pcb->pid, current_pcb->children->length);
                pop(current_pcb->children, current);
                TracePrintf(1, "Done pop from children (%d)\n", current_pcb->children->length);
                *status_ptr = child_pcb->exit_status;
                free(child_pcb->context);
                free(child_pcb);
                return child_pcb->pid;
            }
            current = current->next;
        }
        SwitchToNextProc(RUNNING);
    }
    return ERROR;
}

int TtyReadHandler(int tty_id, void *buf, int len){
    int retval;

    struct tty* terminal = &terminals[tty_id];
    if (terminal->read_buf_q->length == 0){
        //nothing to read yet, block current process
        TracePrintf(1, "No available input from tty %d\n", tty_id);
        enqueue(terminal->read_q, (void*)current_pcb);
        SwitchToNextProc(BLOCKED);
    }

    node* head = terminal->read_buf_q->head;
    struct read_buf* input_buffer = (struct read_buf*) head->value;

    if (input_buffer->len <= len){
        // the requested length is longer than the first line
        // of the input buffer
        retval = input_buffer->len;
        if (dequeue(terminal->read_buf_q) == NULL){
            return ERROR;
        }
        strncpy((char*)buf, input_buffer->buf, len);

        free(input_buffer);
    } else {
        // the requested length is shorter than the first line
        // of the input buffer
        retval = len;
        strncpy((char*)buf, input_buffer->buf, len);

        // update input buf
        input_buffer->buf = &(input_buffer->buf[len]);
        input_buffer->len = input_buffer->len - len;
    }

    return retval;
}

int TtyWriteHandler(int tty_id, void *buf, int len){
    struct tty* terminal = &terminals[tty_id];
    char* buffer = (char*)buf;
    // if there is a process writing to the terminal, block the current process
    if (terminal->write_pcb != NULL){
        enqueue(terminal->write_q, current_pcb);
        SwitchToNextProc(BLOCKED);
    }
    // let the current process have the write lock
    terminal->write_pcb = current_pcb;

    // put a copy of the input in region1
    terminal->write_buf = (char*)malloc(sizeof(char) * len);
    strcpy(terminal->write_buf, buffer);

    TracePrintf(1, "Writting %s to terminal %x\n", terminal->write_buf, tty_id);
    TtyTransmit(tty_id, terminal->write_buf, len);

    SwitchToNextProc(BLOCKED);

    return len;

}