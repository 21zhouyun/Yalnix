#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int
main(int argc, char **argv)
{
    TracePrintf(1, "In Delay test\n");
    Delay(atoi(argv[1]));

    Exit(0);
}
