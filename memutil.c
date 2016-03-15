#include "memutil.h"
#include "include/hardware.h"
#include "include/yalnix.h"

/**
 * Invalidate a page table
 */
struct pte* invalidate_page_table(struct pte* page_table){
    int i;
    for (i = 0; i < PAGE_TABLE_LEN; i++) {
        (page_table + i) -> valid = 0;
        (page_table + i) -> kprot = PROT_NONE;
        (page_table + i) -> uprot = PROT_NONE;
        (page_table + i) -> pfn = PFN_INVALID;
    }
    return page_table;
}

/**
 * Initialize a page table for region 0
 */
struct pte* initialize_page_table_region0(struct pte* page_table) {
    int i;
    int limit = GET_PFN(KERNEL_STACK_LIMIT);
    int base = GET_PFN(KERNEL_STACK_BASE);

    for(i = base; i <= limit; i++) {
        (page_table + i) -> valid = 1;
        (page_table + i) -> pfn = i;
        (page_table + i) -> kprot = (PROT_READ|PROT_WRITE);
        (page_table + i) -> uprot = (PROT_NONE);
    }
    return page_table;
}