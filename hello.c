#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int i = 0;
    while(++i<1000000)
    {   
        TracePrintf(1, "helloword%d\n", i);
    }
}