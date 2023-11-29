/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Tenth Edition
 * Copyright John Wiley & Sons - 2018
 */

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
#define __SYN_ARGS(args) __SYN(__syn_args, args)
#define __SYN_PIPE __SYN(__syn_pipe, __NULL_VEC)
#define __SYN_TO_FILE __SYN(__syn_to_file, __NULL_VEC)
#define __SYN_FROM_FILE __SYN(__syn_from_file, __NULL_VEC)
#define __SYN_AMPERSAND __SYN(__syn_ampersand, __NULL_VEC)

enum __syn_enum
{
  __syn_args = 0,
  __syn_pipe = 10,
  __syn_to_file = 11,
  __syn_from_file = 12,
  __syn_ampersand = 13,
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

struct __parse_result
{
  char ok;
  struct __vec_syn _vec_syn;
  struct __str _err;
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
    struct __vec_str vec_str = s->data;
    __free_vec_str(&vec_str);
  }
  free(vec_syn->_start);
}

struct __vec_syn
__parse_stdin_cmd()
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

void
__print_args(struct __vec_str args, char sep)
{
  char** cmd;
  assert(*args._end == NULL);
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

#endif
