#include "utility.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef _PARSER_H
#define _PARSER_H 1

// parser consts
#define __ARGS_INIT_SIZE 4
#define __STR_INIT_SIZE 8
#define __END '\0'
#define __ARGS_END NULL
#define __SPACE ' '
#define __ESC '\\'
#define __NEWLINE '\n'
#define __RETURN '\r'
#define __ESC_NEWLINE 'n'
#define __ESC_RETURN 'r'
#define __PIPE '|'
#define __TO_FILE '>'
#define __FROM_FILE '<'
#define __AMPERSAND '&'
#define __NULL_VEC                                                             \
  {                                                                            \
    NULL, NULL, NULL,                                                          \
  }
#define __SYN(type, data)                                                      \
  {                                                                            \
    type, data                                                                 \
  }
#define __P_RESULT(_e, _bg, _vc)                                               \
  {                                                                            \
    _e, _bg, _vc                                                               \
  }
#define __CMD_INIT                                                             \
  {                                                                            \
    NULL, NULL, NULL,                                                          \
  }
#define __SYN_ARGS(args) __SYN(__syn_args, args)
#define __SYN_PIPE __SYN(__syn_pipe, __NULL_VEC)
#define __SYN_TO_FILE __SYN(__syn_to_file, __NULL_VEC)
#define __SYN_FROM_FILE __SYN(__syn_from_file, __NULL_VEC)
#define __SYN_AMPERSAND __SYN(__syn_ampersand, __NULL_VEC)
#define __P_RESULT_INIT __P_RESULT(NULL, 0, __NULL_VEC)
#define __EXPECTING_ARGS 1
#define __EXPECTING_IN_FILE 2
#define __EXPECTING_OUT_FILE 3
#define __P_ERR(_result, _e)                                                   \
  _result._err = _e;                                                           \
  goto cleanup;

enum __syn_enum
{
  __null_value = 0,
  __syn_args = 10,
  __syn_pipe = 100,
  __syn_to_file = 101,
  __syn_from_file = 102,
  __syn_ampersand = 103,
};

struct __syn
{
  enum __syn_enum type;
  struct __vec_str data;
};

struct __vec_syn
{
  struct __syn* _start;
  struct __syn* _end;
  struct __syn* _mem_end;
};

struct __command
{
  char* _in_file;  // nullable
  char* _out_file; // nullable
  char** args;     // non-nullable
};

struct __vec_command
{
  struct __command* _start;
  struct __command* _end;
  struct __command* _mem_end;
};

struct __parse_result
{
  char* _err; // NULL if ok, check first
  char background;
  struct __vec_command commands;
};

void
__free_vec_str(struct __vec_str* vec_str)
{
  if (vec_str->_start != NULL) {
    char** p;
    for (p = vec_str->_start; p < vec_str->_end; p++) {
      if (*p != NULL) {
        free(*p);
      }
    }
    free(vec_str->_start);
  }
}

void
__free_vec_syn(struct __vec_syn* vec_syn)
{
  struct __syn* s;
  for (s = vec_syn->_start; s < vec_syn->_end; s++) {
    assert(s != NULL); // we don't directly move syn
    struct __vec_str vec_str = s->data;
    __free_vec_str(&vec_str);
  }
  free(vec_syn->_start);
}

void
__free_commands(struct __vec_command* commands)
{
  if (commands->_start == NULL) {
    return;
  }
  struct __command* cmd;
  for (cmd = commands->_start; cmd < commands->_end; cmd++) {
    if (cmd->_in_file != NULL) {
      free(cmd->_in_file);
    }
    if (cmd->_out_file != NULL) {
      free(cmd->_out_file);
    }
    char** args = cmd->args;
    assert(args != NULL);
    char** s;
    for (s = args; *s != NULL; s++) {
      free(*s);
    }
    free(args);
  }
  free(commands->_start);
}

void
__free_parsed_result(struct __parse_result* parsed)
{
  // the err string is static and should not be freed
  __free_commands(&parsed->commands);
}

void
__debug_print_syn(struct __vec_syn* vec_syn);

void
__debug_print_parsed(struct __parse_result* parsed);

struct __vec_syn
__tokenize_stdin(char debug)
{
  // init containers
  struct __vec_syn syn;
  struct __vec_str args;
  struct __str str;
  __VEC_INIT(syn, __ARGS_INIT_SIZE);
  __VEC_INIT(args, __ARGS_INIT_SIZE);
  __VEC_INIT(str, __STR_INIT_SIZE);

  // parser states
  char escape = 0;

  // start to parse
  for (;;) {
    int c = getchar();

    // char insertion
    switch (c) {
      case EOF:
      case __NEWLINE:
      case __RETURN:
        break;

      case __SPACE:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __SPACE);
          continue;
        }
        break;

      case __PIPE:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __PIPE);
          continue;
        }
        break;

      case __FROM_FILE:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __FROM_FILE);
          continue;
        }
        break;

      case __AMPERSAND:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __AMPERSAND);
          continue;
        }
        break;

      case __TO_FILE:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __TO_FILE);
          continue;
        }
        break;

      case __ESC:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __ESC);
        } else {
          escape = 1;
        }
        continue;

      case __ESC_NEWLINE:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __NEWLINE);
        } else {
          __VEC_INSERT(str, c);
        }
        continue;

      case __ESC_RETURN:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __RETURN);
        } else {
          __VEC_INSERT(str, c);
        }
        continue;

      default:
        __VEC_INSERT(str, c);
        continue;
    }

    // str / args insertions
    switch (c) {
      // should return
      case EOF:
      case __NEWLINE:
      case __RETURN:
        if (!__VEC_EMPTY(str)) {
          __VEC_INSERT(str, __END);
          __VEC_INSERT(args, str._start); // moved
        } else {
          free(str._start); // freed
        }
        if (!__VEC_EMPTY(args)) {
          __VEC_INSERT(args, __ARGS_END);
          __VEC_INSERT(syn, __SYN_ARGS(args)); // moved
        } else {
          free(args._start); // freed
        }
        if (debug) {
          __debug_print_syn(&syn);
        }
        return syn;

      // should continue
      case __SPACE:
        // str -> args syn for ' '
        if (!__VEC_EMPTY(str)) {
          __VEC_INSERT(str, __END);
          __VEC_INSERT(args, str._start);   // moved
          __VEC_INIT(str, __STR_INIT_SIZE); // init
        }
        break;
      case __PIPE:
      case __FROM_FILE:
      case __TO_FILE:
      case __AMPERSAND:
        // str -> args -> syn for '|','>','<','&'
        if (!__VEC_EMPTY(str)) {
          __VEC_INSERT(str, __END);
          __VEC_INSERT(args, str._start);   // moved
          __VEC_INIT(str, __STR_INIT_SIZE); // init
        }
        if (!__VEC_EMPTY(args)) {
          __VEC_INSERT(args, __ARGS_END);
          __VEC_INSERT(syn, __SYN_ARGS(args)); // moved
          __VEC_INIT(args, __ARGS_INIT_SIZE);  // init
        }
        switch (c) {
          case __PIPE:
            __VEC_INSERT(syn, __SYN_PIPE);
            break;
          case __FROM_FILE:
            __VEC_INSERT(syn, __SYN_FROM_FILE);
            break;
          case __TO_FILE:
            __VEC_INSERT(syn, __SYN_TO_FILE);
            break;
          case __AMPERSAND:
            __VEC_INSERT(syn, __SYN_AMPERSAND);
            break;
          default:
            // unreachable
            exit(EXIT_FAILURE);
        }
        break;

      default:
        // unreachable
        exit(EXIT_FAILURE);
    }
  }
}

struct __parse_result
__parse_cmd(char debug)
{
  // read from stdin
  struct __vec_syn vec_syn = __tokenize_stdin(debug);

  // init structs
  struct __parse_result result = __P_RESULT_INIT;
  {
    struct __vec_command commands;
    __VEC_INIT(commands, __ARGS_INIT_SIZE);
    result.commands = commands;
  }
  struct __command this_command = __CMD_INIT;

  // parser states
  enum __syn_enum prev = __null_value;

  // err string
  char* err = NULL;

  struct __syn* syn;
  for (syn = vec_syn._start; syn < vec_syn._end; syn++) {
    switch (syn->type) {
      case __syn_pipe:
        switch (prev) {
          case __null_value:
            __P_ERR(result, "invalid token near `|`, cannot start with `|`");
          case __syn_pipe:
            __P_ERR(result, "invalid syntax near `|`, double `|`");
          case __syn_from_file:
            __P_ERR(result, "invalid syntax near `|`, `<` followed by `|`");
          case __syn_to_file:
            __P_ERR(result, "invalid syntax near `|`, `>` followed by `|`");
          case __syn_ampersand:
            break; // unreachable
          case __syn_args: {
            assert(this_command.args != NULL);
            __VEC_INSERT(result.commands, this_command); // moved
            // clear this_command
            this_command.args = NULL;
            this_command._in_file = NULL;
            this_command._out_file = NULL;
          }
        }
        prev = __syn_pipe;
        break;
      case __syn_to_file:
        switch (prev) {
          case __null_value:
            __P_ERR(result, "invalid token near `>`, cannot start with `>`");
          case __syn_pipe:
            __P_ERR(result, "invalid syntax near `>`, `|` followed by `>`");
          case __syn_from_file:
            __P_ERR(result, "invalid syntax near `>`, <` followed by `>`");
          case __syn_to_file:
            __P_ERR(result, "invalid syntax near `>`, double `>`");
          case __syn_ampersand: // unreachable
          case __syn_args:      // noop
            break;
        }
        prev = __syn_to_file;
        break;
      case __syn_from_file:
        switch (prev) {
          case __null_value:
            __P_ERR(result, "invalid token near `<`, cannot start with `<`");
          case __syn_pipe:
            __P_ERR(result, "invalid syntax near `<`, `|` followed by `<`");
          case __syn_from_file:
            __P_ERR(result, "invalid syntax near `<`, double `<`");
          case __syn_to_file:
            __P_ERR(result, "invalid syntax near `<`, `>` followed by `<`");
          case __syn_ampersand: // unreachable
          case __syn_args:      // noop
            break;
        }
        prev = __syn_from_file;
        break;
      case __syn_ampersand:
        if ((syn + 1) != vec_syn._end) {
          // '&' can only appear at the end
          __P_ERR(result, "invalid, `&` can only appear at the end of line");
        }
        switch (prev) {
          case __null_value:
            __P_ERR(result, "invalid token near `&`, cannot start with `&`");
          case __syn_pipe:
            __P_ERR(result, "invalid syntax near `&`, `|` followed by `&`");
          case __syn_from_file:
            __P_ERR(result, "invalid syntax near `&`, `<` followed by `&`");
          case __syn_to_file:
            __P_ERR(result, "invalid syntax near `&`, `>` followed by `&`");
          case __syn_ampersand:
            break; // unreachable
          case __syn_args:
            result.background = 1;
            break;
        }
        prev = __syn_ampersand;
        break;
      case __syn_args:
        switch (prev) {
          case __null_value:
          case __syn_pipe:
            if (this_command.args != NULL) {
              __P_ERR(result, "internal error, args already written!");
            }
            this_command.args = syn->data._start;
            syn->data._start = NULL; // move args (char**)
            break;
          case __syn_from_file: {
            size_t arg_len = __VEC_LEN(syn->data);
            assert(arg_len > 1); // ends with NULL + nonempty
            if (arg_len > 2) {
              __P_ERR(result, "too many file args after `<`");
            }
            if (this_command._in_file != NULL) {
              __P_ERR(result, "multiple in-file `<` options");
            }
            assert(*syn->data._start != NULL);
            this_command._in_file = *syn->data._start;
            *syn->data._start = NULL; // move first args (char*)
            break;
          }
          case __syn_to_file: {
            size_t arg_len = __VEC_LEN(syn->data);
            assert(arg_len > 1); // ends with NULL + nonempty
            if (arg_len > 2) {
              __P_ERR(result, "too many file args after `>`");
            }
            if (this_command._out_file != NULL) {
              __P_ERR(result, "multiple out-file `>` options");
            }
            assert(*syn->data._start != NULL);
            this_command._out_file = *syn->data._start;
            *syn->data._start = NULL; // move first args (char*)
            break;
          }
          case __syn_ampersand:
            break; // unreachable
          case __syn_args:
            // unreachable
            __P_ERR(result, "internal error, consecutive args");
        }
        prev = __syn_args;
        break;
      default:
        exit(EXIT_FAILURE); // unreachable
    }
  }

  switch (prev) {
    case __syn_pipe:
      __P_ERR(result, "invalid syntax, cannot end with `|`");
    case __syn_from_file:
      __P_ERR(result, "invalid syntax, cannot end with `<`");
    case __syn_to_file:
      __P_ERR(result, "invalid syntax, cannot end with `>`");
    default:
      break;
  }

  // deal with the last command
  if (this_command.args != NULL) {
    __VEC_INSERT(result.commands, this_command); // moved
    // clear this_command
    this_command.args = NULL;
    this_command._in_file = NULL;
    this_command._out_file = NULL;
  } else {
    assert(this_command._in_file == NULL);
    assert(this_command._out_file == NULL);
  }

cleanup:
  if (result._err != NULL) {
    // cleanup
    __free_commands(&result.commands);
    result.commands._start = NULL;
  }
  // free syn
  __free_vec_syn(&vec_syn);

  if (debug) {
    __debug_print_parsed(&result);
  }
  return result;
}

void
__print_args(struct __vec_str args, char sep)
{
  char** cmd;
  assert(*(args._end - 1) == NULL);
  for (cmd = args._start; cmd < args._end - 1; cmd++) {
    printf("%s", *cmd);
    if (cmd < args._end - 2) {
      printf("%c", sep);
    }
  }
}

void
__debug_print_syn(struct __vec_syn* vec_syn)
{
  struct __syn* syn;
  int cnt = 0;
  for (syn = vec_syn->_start; syn < vec_syn->_end; syn++) {
    printf("%i: ", cnt++);
    switch (syn->type) {
      case __syn_pipe:
        printf("PIPE\n");
        break;
      case __syn_to_file:
        printf("TO_FILE\n");
        break;
      case __syn_from_file:
        printf("FROM_FILE\n");
        break;
      case __syn_ampersand:
        printf("AMPERSAND\n");
        break;
      case __syn_args:
        printf("ARGS(");
        __print_args(syn->data, ',');
        printf(")\n");
        break;
    }
  }
}

void
__debug_print_parsed(struct __parse_result* parsed)
{
  printf("{\n");
  printf("  background: %i,\n", parsed->background);
  if (parsed->_err != NULL) {
    printf("  err: %s\n", parsed->_err);
  } else {
    printf("  commands: [\n");
    struct __command* cmd;
    for (cmd = parsed->commands._start; cmd < parsed->commands._end; cmd++) {
      printf("    {\n");
      if (cmd->_in_file != NULL) {
        printf("      in_file: %s,\n", cmd->_in_file);
      }
      if (cmd->_out_file != NULL) {
        printf("      out_file: %s,\n", cmd->_out_file);
      }
      char** arg;
      int arg_cnt = 0;
      for (arg = cmd->args; *arg != NULL; arg++) {
        printf("      arg_%i: %s,\n", arg_cnt++, *arg);
      }
      printf("    },\n");
    }
    printf("  ]\n");
  }
  printf("}\n");
}

#endif
