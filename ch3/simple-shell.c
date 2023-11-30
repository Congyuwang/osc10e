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

#define __DEBUG 0

// shell consts
#define __EXIT_CMD "exit"
#define __REDO_CMD "!!"
#define __PROMPT "osh> "
#define __NO_HISOTRY_CMD_WARN "No commands in history.\n"

void
dup_file_fd(const char* file, const char* file_mode, int io_fd)
{
  // open file
  FILE* in_file = fopen(file, file_mode);
  if (in_file == NULL) {
    perror("open file");
    _exit(EXIT_FAILURE);
  }
  // dup file fd
  if (dup2(fileno(in_file), io_fd) == -1) {
    perror("dup2 file");
    _exit(EXIT_FAILURE);
  }
  // close file fd
  if (close(fileno(in_file)) != 0) {
    perror("close file");
    _exit(EXIT_FAILURE);
  }
}

int*
__init_pipes(int total_proc)
{
  int* pipes = NULL;
  int n_pipes = total_proc - 1;
  if (n_pipes > 0) {
    pipes = (int*)malloc(sizeof(int) * n_pipes * 2);
    if (pipes == NULL) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }
  }
  // create pipes: [r/w r/w r/w ...]
  int n;
  int* p = pipes;
  for (n = 0; n < n_pipes; n++) {
    if (pipe(p) < 0) {
      perror("failure creating pipe");
      exit(EXIT_FAILURE);
    }
    p += 2;
  }
  return pipes;
}

void
__close_pipes(const int* pipes, int total_proc)
{
  int n;
  int n_pipes = (total_proc - 1) * 2;
  for (n = 0; n < n_pipes; n++) {
    if (close(*pipes) != 0) {
      perror("error close");
      _exit(EXIT_FAILURE);
    }
    pipes++;
  }
}

static inline const int*
__get_pipe(const int* pipes, int n_proc, int total_proc, char read)
{
  const int* read_pos = pipes + (n_proc - 1) * 2;
  const int* write_pos = read_pos + 3;
  char can_read = 1 <= n_proc && n_proc <= (total_proc - 1);
  char can_write = 0 <= n_proc && n_proc <= (total_proc - 2);
  size_t readable = read && can_read;
  size_t writable = !read && can_write;
  return (int*)(readable * (size_t)read_pos + writable * (size_t)write_pos);
}

void
dup_pipe(const int* pipes, int n_proc, int total_proc, char read, int io_fd)
{
  // use pipe as stdin
  const int* p = __get_pipe(pipes, n_proc, total_proc, read);
  if (p != NULL) {
    if (dup2(*p, io_fd) == -1) {
      perror("dup2 pipe");
      _exit(EXIT_FAILURE);
    }
  }
}

void
__fork_child(struct __command* cmd,
           const int* pipes,
           int n_proc,
           int total_proc,
           int* pids)
{
  int pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  if (pid == 0) {
    // dup fd to stdin / stdout
    if (cmd->_in_file != NULL) {
      dup_file_fd(cmd->_in_file, "r", STDIN_FILENO);
    } else {
      dup_pipe(pipes, n_proc, total_proc, 1, STDIN_FILENO);
    }
    if (cmd->_out_file != NULL) {
      dup_file_fd(cmd->_out_file, "w", STDOUT_FILENO);
    } else {
      dup_pipe(pipes, n_proc, total_proc, 0, STDOUT_FILENO);
    }
    // close all pipes fd after dup
    __close_pipes(pipes, total_proc);
    // execv
    int code = execvp(*cmd->args, cmd->args);
    perror("exec");
    _exit(code);
  } else {
    // parent: record pid
    pids[n_proc] = pid;
  }
}

/**
 *
*/
void
__exec(struct __parse_result* command)
{
  int total_proc = __VEC_LEN(command->commands);

  // pids
  int* pids = (int*)malloc(sizeof(int) * total_proc);

  {
    // alloc / open pipes, alloc pids
    int* pipes = __init_pipes(total_proc);

    // fork child processes
    struct __command* cmd;
    int n_proc = 0;
    for (cmd = command->commands._start; cmd < command->commands._end; cmd++) {
      __fork_child(cmd, pipes, n_proc, total_proc, pids);
      n_proc++;
    }

    // close all pipes after finished forking
    __close_pipes(pipes, total_proc);
    free(pipes);
  }

  // wait for finish
  if (!command->background) {
    int status = 0;
    int n;
    for (n = 0; n < total_proc; n++) {
      waitpid(*(pids + n), &status, 0);
    }
  }

  // free memory
  free(pids);
}

int
main(void)
{
  struct __parse_result command = __P_RESULT_INIT;

  for (;;) {
    // read args from stdin
    printf(__PROMPT);
    fflush(stdout);
    struct __parse_result parsed = __parse_cmd(__DEBUG);

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
          __print_parsed(&command);
          fflush(stdout);
        } else {
          printf(__NO_HISOTRY_CMD_WARN);
          fflush(stdout);
        }
        // free parsed, retain old command
        __free_parsed_result(&parsed);
      } else {
        // replace old command with new one
        __free_parsed_result(&command);
        command = parsed;
        struct __parse_result empty = __P_RESULT_INIT;
        parsed = empty;
      }
    } else {
      // empty commands, free parsed
      __free_parsed_result(&parsed);
      continue;
    }
    // lifetime of parsed ended (moved or freed)

    if (command.commands._start != NULL) {
      __exec(&command);
    }
  }
  __free_parsed_result(&command);
  return 0;
}
