#include <stdio.h>
#include <comp421/yalnix.h>

/* exectest progname args... */
/* for fun: exectest exectest exectest exectest exectest progname args... */
int
main(int argc, char **argv)
{
    int i;

    for (i = 0; i < argc; i++) {
	fprintf(stderr, "argv[%d] = %p", i, argv[i]);
	fprintf(stderr, " = '%s'\n", argv[i]);
    }

    TracePrintf(1, "In exectest. Exec %s\n", argv[1]);

    // this makes the read change contents in some physical pages that shouldnt be changing
    Exec(argv[1], argv + 1);

    fprintf(stderr, "Exec did not work!!\n");

    Exit(1);
}
