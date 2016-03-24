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
		copyKernelStackIntoTable(((struct pcb *) p1)->physical_page_table);
		return ctxp;
	} else {
		// Doing context switch
		struct pcb *pcb1 = (struct pcb *) p1;
		struct pcb *pcb2 = (struct pcb *) p2;

        pcb2->page_table = mapToTemp((void*)pcb2->physical_page_table, kernel_temp_vpn);
		WriteRegister(REG_PTR0, (RCS421RegVal) pcb2->physical_page_table);
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

		//update currnet pcb
		if (current_pcb->pid != 0 && current_pcb->process_state == RUNNING){
			enqueue_ready(current_pcb);
		}
		current_pcb = pcb2;
        // the process we switch to must be in the state RUNNING afterwards
        current_pcb->process_state = RUNNING;
		TracePrintf(1, "Switch to page table at %x (%x)\n", pcb2->physical_page_table, pcb2->page_table);

		//TODO: make sure this is the one we want to return??
		return pcb2->context;
	}
}

SavedContext *ForkSwitchFunc(SavedContext *ctxp, void *p1, void *p2){
	struct pcb *parent_pcb = (struct pcb *) p1;
	struct pcb *child_pcb = (struct pcb *) p2;
	assert(parent_pcb->pid == current_pcb->pid);

	TracePrintf(1, "Copy saved context from parent proces to child process.\n");
    //copy parent saved context
    //since both contexts are allocated in region1, we can directly
    //copy it over using its virtual address.
    memcpy(child_pcb->context, ctxp, sizeof(SavedContext));

	TracePrintf(1, "Copy reigon0 from parent proces to child process.\n");
    //copy parent region0 to child, including kernel stack
    copyRegion0IntoTable(child_pcb->physical_page_table);

    // debugPageTable(child_pcb->page_table);

    TracePrintf(1, "Switch to child's page table.\n");
    //switch to child process
    mapToTemp((void*)child_pcb->physical_page_table, kernel_temp_vpn);
    WriteRegister(REG_PTR0, (RCS421RegVal) child_pcb->physical_page_table);
	WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

	if (current_pcb->pid != 0 && current_pcb->process_state != TERMINATED){
		enqueue_ready(current_pcb);
	}
    current_pcb = child_pcb;
    current_pcb->process_state = RUNNING;

    return child_pcb->context;
}