#ifndef YALNIX_MEMUTIL_H
#define YALNIX_MEMUTIL_H

#include "include/hardware.h"
#include "include/yalnix.h"

#define GET_PFN(addr) (((long) addr & PAGEMASK) >> PAGESHIFT)

#define PFN_INVALID -1

struct pte* invalidate_page_table(struct pte* page_table);

struct pte* initialize_page_table_region0(struct pte* page_table);

#endif //YALNIX_MEMUTIL_H
