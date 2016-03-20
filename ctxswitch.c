#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "ctxswitch.h"
#include "memutil.h"

SavedContext *MySwitchFunc(SavedContext *ctxp, void *p1, void *p2)
{
	if (p2 == NULL) {
		// Trying to just get a copy of the current SavedContext and 
		// kernel stack, without necessarily switching to a new process.

		// p1 is pcb for a new process, copy the kernel stack here.
		copyKernelStackIntoTable(((struct pcb *) p1)->page_table);
		return ctxp;
	} else {
		// Doing context switch
		struct pcb *pcb1 = (struct pcb *) p1;
		struct pcb *pcb2 = (struct pcb *) p2;

		WriteRegister(REG_PTR0, (RCS421RegVal) pcb2->page_table);
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

		//update currnet pcb
		current_pcb = pcb2;
		TracePrintf(1, "Switch to page table at %d\n", pcb2->page_table);

		//TODO: make sure this is the one we want to return??
		return pcb2->context;
	}
}

SavedContext *ForkSwitchFunc(SavedContext *ctxp, void *p1, void *p2){
	struct pcb *parent_pcb = (struct pcb *) p1;
	struct pcb *child_pcb = (struct pcb *) p2;
	assert(parent_pcb->pid == current_pcb->pid);

    //copy parent saved context
    //since both contexts are allocated in region1, we can directly
    //copy it over using its virtual address.
    memcpy(child_pcb->context, ctxp, sizeof(SavedContext));

    //copy parent region0 to child, including kernel stack
    copyRegion0IntoTable(child_pcb->page_table);

    return child_pcb->context;
}