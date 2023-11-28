/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Tenth Edition
 * Copyright John Wiley & Sons - 2018
 */

#include "utility.h"
#include <stdio.h>
#include <unistd.h>

int
main(void)
{
  int should_run = 1;

  while (should_run) {
    printf("osh> ");
    fflush(stdout);
    struct CmdArgs args = parse_stdin_cmd();
    int count = 0;
    char** arg;
    for (arg = args.start; arg < args.end; arg++) {
      printf("%i: %s\n", count++, *arg);
    }
    // free_cmd_args(args);
    /**
     * After reading user input, the steps are:
     * (1) fork a child process
     * (2) the child process will invoke execvp()
     * (3) if command included &, parent will invoke wait()
     */
  }

  return 0;
}
