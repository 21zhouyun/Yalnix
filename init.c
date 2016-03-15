#include "include/hardware.h"
#include "include/yalnix.h"

#include "init.h"
#include "trap.h"
#include "util/memutil.h"

// whether we have enabled virtual memory
bool VM_ENABLE = false;

struct pte page_table_region0[PAGE_TABLE_LEN];
struct pte page_table_region1[PAGE_TABLE_LEN];

void *KERNEL_HEAP_LIMIT;

void KernelStart(ExceptionStackFrame *frame,
    unsigned int pmem_size, void *orig_brk, char **cmd_args){

    InitTrapVector();
    // init page tables
    KERNEL_HEAP_LIMIT =orig_brk;
}

void InitPageTable(){
    // initialize page table for region 1 to be invalid
    page_table_region1 = invalidate_page_table(page_table_region1);

    /**
     * Map region 1 to virtual memory
     * See hangout p22-23 for detailed mapping
     */
    kernel_heap_limit_pfn = GET_PFN(KERNEL_HEAP_LIMIT);
    kernel_text_limit_pfn = GET_PFN(&_etext);

    int i;
    // text section
    for (i = GET_PFN(VMEM_1_BASE); i < kernel_text_limit_pfn; i++) {
        page_table_region1[i].valid = 1;
        page_table_region1[i].pfn = i;
        page_table_region1[i].kprot = (PROT_READ|PROT_EXEC);
    }
    //data, bss, heap
    for (i = kernel_text_limit_pfn; i <= kernel_heap_limit_pfn; i++) {
        page_table_region1[i].valid = 1;
        page_table_region1[i].pfn = i;
        page_table_region1[i].kprot = (PROT_READ|PROT_WRITE);
    }

    //initialize a page table for region 0
    page_table_region0 = invalidate_page_table(page_table_region0);
    page_table_region0 = initialize_page_table_region0(page_table_region0);

    WriteRegister(REG_PTR0, (RCS421RegVal)page_table_region0);
    WriteRegister(REG_PTR1, (RCS421RegVal)page_table_region1);
    WriteRegister(REG_VM_ENABLE, (RCS421RegVal)1);
    TracePrintf(1, "Enabled virtual memory.");
}