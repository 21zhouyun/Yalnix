#include <stdio.h>
#include <stdlib.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>

int
main()
{
    while (1){
        TracePrintf(1, "in init...\n");
        //printf("My pid is %d\n", GetPid());
        Delay(2);
    }
}