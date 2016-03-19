#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    while(1)
    {   
        TracePrintf(1, "In idle...\n");
        Pause();
    }
}