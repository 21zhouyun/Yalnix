#include <stdio.h>
#include <stdlib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
int
main()
{
    // INIT TEST
    int i = 1;
    while (i > 0){
        TracePrintf(1, "In init round %d\n", i++);
        Delay(3);
    }


    // BRK TEST
	// void * a = malloc (4096 * 10);
	// void * b = malloc (4096 * 10);
	// char * stringA = malloc (sizeof(char) * 10);
	// char * stringB = "123456789";

	// strncpy(stringA, stringB, 10);
	// TracePrintf(1, "String written: %s.\n", stringA);
	// TracePrintf(1, "Address allocated, a: %p, b: %p\n", a, b);
	// TracePrintf(1, "Writing to a number 5.\n");
	// *(int *)a = 5;
	// TracePrintf(1, "Check if content equals: 5 == %d \n", *(int*)a);
	

	// free(b);
	// TracePrintf(1, "b freed: %p\n", b);
	// //Uncomment the following two lines should cause error
	// //TracePrintf(1, "Now try to write to an invalid page, should see implicit user stack growth, but in redzone:\n");
	// //*(int *)((int)b+4096) = 5;

	// //TracePrintf(1, "Now try to access an invalid page, should see error: %p\n", *(int *) ((int)b + 4096));
	// void * c = malloc (4096 * 10);
	// TracePrintf(1, "Address allocated, c: %p\n", c);


    // FORK TEST
    // TracePrintf(1, "In init main. Do fork.\n");

    // int pid = Fork();

    // if (pid == 0){
    //     TracePrintf(1, "In child of init\n");
    //     Delay(3);
    // } else {
    //     TracePrintf(1, "Spawned child with pid %x\n", pid);
    //     TracePrintf(1, "in init (%x)\n", GetPid());
    //     //printf("My pid is %d\n", GetPid());
    //     Delay(3);
    // }

    return 0;
}