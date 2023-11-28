/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Tenth Edition
 * Copyright John Wiley & Sons - 2018
 */

#include "utility.h"
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define __AMPERSAND "&"
#define __EXIT_CMD "exit"
#define __REDO_CMD "!!"
#define __PROMPT "osh> "
#define __NO_HISOTRY_CMD_WARN "No commands in history."
#define __NULL_CMD                                                             \
  {                                                                            \
    NULL, NULL, NULL,                                                          \
  }

struct __command
{
  char in_background;
  struct __vec_str args;
};

void
__print_command(struct __command* history);

int
__fork_exec(struct __command* command);

int
main(void)
{
  int should_run = 1;

  struct __command command = { 0, __NULL_CMD };

  for (;;) {

    // read args from stdin
    printf(__PROMPT);
    fflush(stdout);
    struct __vec_str args = parse_stdin_cmd();

    /**
     * Memory management of args:
     * It is cleared
     */

    // check for exit
    char** first = __VEC_FIRST(args);
    if (first != NULL && strcmp(*first, __EXIT_CMD) == 0) {
      // exit
      free_vec_str(&args);
      break;
    }

    // check for !!
    char redo = first != NULL && strcmp(*first, __REDO_CMD) == 0;
    if (redo) {
      if (command.args._start != NULL) {
        // print history command
        printf(__PROMPT);
        __print_command(&command);
        fflush(stdout);
      } else {
        // found no history
        printf(__NO_HISOTRY_CMD_WARN);
        fflush(stdout);
      }
      // args freed
      free_vec_str(&args);
    } else {
      // clear history and store new command
      // check for &
      char** last = __VEC_LAST(args);
      // move args
      free_vec_str(&command.args);
      command.args = args;
      command.in_background = last != NULL && strcmp(*last, __AMPERSAND) == 0;
      // ensure args end with NULL for execvp
      if (command.in_background) {
        free(*last);
        *last = NULL;
      } else {
        __VEC_INSERT(command.args, NULL);
      }
    }
    // lifetime of args ends at the block (either freed, or moved)

    // fork
    if (command.args._start != NULL) {
      __fork_exec(&command);
    }
  }

  free_vec_str(&command.args);

  return 0;
}

int
__fork_exec(struct __command* command)
{
  int pid = fork();
  if (pid == 0) {
    int code = execvp(*command->args._start, command->args._start);
    _exit(code);
  } else if (pid > 0) {
    // parent
    if (!command->in_background) {
      int status;
      waitpid(pid, &status, 0);
      printf("process %i exited with code  %i.\n", pid, status);
      fflush(stdout);
    }
  } else {
    // fatal error, directly exit
    exit(-pid);
  }
}

void
__print_command(struct __command* history)
{
  char** cmd;
  assert(*history->args._end == NULL);
  for (cmd = history->args._start; cmd < history->args._end - 1; cmd++) {
    printf("%s ", *cmd);
  }
  printf("\n");
}
