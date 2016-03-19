#include <stdio.h>
#include <stdlib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>

int
main()
{
	void * a = malloc (4096 * 10);
	void * b = malloc (4096 * 10);
	TracePrintf(1, "Address allocated, a: %p, b: %p\n", a, b);
	TracePrintf(1, "Writing to a letters 'aaa'.\n");
	*(char *)a = "aaa";
	TracePrintf(1, "Check if content equals: 'aaa' == '%s' \n", *(char*)a);
	free(b);
	TracePrintf(1, "b freed: %p\n", b);
	//TracePrintf(1, "Now try to access an invalid page, should see error: %p\n", *(char *) ((int)b + 4096));
	void * c = malloc (4096 * 10);
	TracePrintf(1, "Address allocated, c: %p\n", c);

    while (1){
        TracePrintf(1, "in init...\n");
        //printf("My pid is %d\n", GetPid());
        Delay(3);
    }
}