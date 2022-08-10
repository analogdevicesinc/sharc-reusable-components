// Additional shell commands

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"

/***********************************************************************
 * NOTE: New commands added here must also be added to the appropriate
 *       locations in shell.c
 **********************************************************************/

/***********************************************************************
 * CMD: hello
 **********************************************************************/
const char shell_help_hello[] = "<arg1> <arg2> ... <argX>\n"
  "  argX - Arguments to echo back\n";
const char shell_help_summary_hello[] = "Echoes back command line arguments";

void shell_hello(SHELL_CONTEXT *ctx, int argc, char **argv)
{
    int i;
    for (i = 0; i < argc; i++) {
        printf("Arg %d: %s\n", i, argv[i]);
    }
}
