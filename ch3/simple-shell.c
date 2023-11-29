/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Tenth Edition
 * Copyright John Wiley & Sons - 2018
 */

#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// shell consts
#define __EXIT_CMD "exit"
#define __REDO_CMD "!!"
#define __PROMPT "osh> "
#define __NO_HISOTRY_CMD_WARN "No commands in history.\n"

int
__exec(struct __parse_result* command);

int
main(void)
{
  struct __parse_result command = __P_RESULT_INIT;

  for (;;) {
    // read args from stdin
    printf(__PROMPT);
    fflush(stdout);
    struct __parse_result parsed = __parse_cmd(0);

    // first check parse err
    if (parsed._err != NULL) {
      printf("error: %s\n", parsed._err);
      __free_parsed_result(&parsed);
      continue;
    }

    // peek first
    struct __command* first = __VEC_FIRST(parsed.commands);
    if (first != NULL) {
      assert(first->args != NULL && *first->args != NULL);
      // exit
      if (strcmp(*first->args, __EXIT_CMD) == 0) {
        __free_parsed_result(&parsed);
        break;
      }
      // redo
      if (strcmp(*first->args, __REDO_CMD) == 0) {
        if (command.commands._start != NULL) {
          printf(__PROMPT);
          printf("%s\n", command.input);
          fflush(stdout);
        } else {
          printf(__NO_HISOTRY_CMD_WARN);
          fflush(stdout);
        }
        // free parsed, retain old command
        __free_parsed_result(&parsed);
      } else if (__VEC_EMPTY(parsed.commands)) {
        // free parsed, retain old command
        __free_parsed_result(&parsed);
      } else {
        // replace old command with new one
        __free_parsed_result(&command);
        command = parsed;
        struct __parse_result empty = __P_RESULT_INIT;
        parsed = empty;
      }
    }
    // lifetime of parsed ended (moved or freed)

    if (command.commands._start != NULL) {
      assert(command.input != NULL);
      __exec(&command);
    }
  }
  __free_parsed_result(&command);
  return 0;
}

int __exec(struct __parse_result* command) {
  printf("%s\n", command->input);
  return 0;
}
