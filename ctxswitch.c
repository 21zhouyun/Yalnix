#include <stdlib.h>

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
		pcb *pcb1 = (pcb *) p1;
		pcb *pcb2 = (pcb *) p2;

		pcb1->process_state = READY;
		pcb2->process_state = RUNNING;

		WriteRegister(REG_PTR0, (RCS421RegVal) pcb2->page_table);
		WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
		return pcb2->context;
	}
}