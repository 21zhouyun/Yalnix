#include <stdlib.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "ctxswitch.h"
#include "memutil.h"

SavedContext *MySwitchFunc(SavedContext *ctxp, void *p1, void *p2)
{
	if (p2 == NULL) {
		// Trying to just get a copy of the current SavedContext and 
		// kernel stack, without necessarily switching to a new process.
		return ctxp;
	} else {
		// Doing context switch
		struct pcb *pcb1 = (struct pcb *) p1;
		struct pcb *pcb2 = (struct pcb *) p2;
		TracePrintf(1, "Switch to page table at %d\n", pcb2->page_table);
		// pcb1->process_state = READY;
		// pcb2->process_state = RUNNING;

		WriteRegister(REG_PTR0, (RCS421RegVal) pcb2->page_table);
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

		//update currnet pcb
		current_pcb = pcb2;
		return ctxp;
	}
}