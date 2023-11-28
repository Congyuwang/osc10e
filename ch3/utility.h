#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef _UTILITY_H
#define _UTILITY_H 1

#define __ARGS_INIT_SIZE 4
#define __STR_INIT_SIZE 8
#define __ERROR_CODE 1
#define __END '\0'
#define __SPACE ' '
#define __ESC '\\'
#define __NEWLINE '\n'
#define __RETURN '\r'
#define __ESC_NEWLINE 'n'
#define __ESC_RETURN 'r'

// check malloc error (exit on error)
#define __CHECKED_MALLOC(_ptr, _init_size)                                     \
  _ptr = (typeof(_ptr))malloc(sizeof(*_ptr) * _init_size);                     \
  if (_ptr == NULL) {                                                          \
    exit(__ERROR_CODE);                                                        \
  }

// check realloc error (exit on error)
#define __CHECKED_REALLOC(_ptr, _size)                                         \
  void* _new_ptr = realloc(_ptr, sizeof(*_ptr) * _size);                       \
  if (_new_ptr == NULL) {                                                      \
    free(_ptr);                                                                \
    exit(__ERROR_CODE);                                                        \
  }                                                                            \
  _ptr = (typeof(_ptr))_new_ptr

// initialize vector _vec with initial capacity _init_size.
#define __VEC_INIT(_vec, _init_size)                                           \
  __CHECKED_MALLOC(_vec._start, _init_size);                                   \
  _vec._end = _vec._start;                                                     \
  _vec._mem_end = _vec._start + _init_size

// push back _element into vector _vec.
#define __VEC_INSERT(_vec, _element)                                           \
  if (_vec._end == _vec._mem_end) {                                            \
    size_t _offset = _vec._end - _vec._start;                                  \
    size_t _size = (_vec._mem_end - _vec._start) << 1;                         \
    __CHECKED_REALLOC(_vec._start, _size);                                     \
    _vec._end = _vec._start + _offset;                                         \
    _vec._mem_end = _vec._start + _size;                                       \
  }                                                                            \
  *(_vec._end++) = _element;

// check if vector is empty
#define __VEC_EMPTY(_vec) (_vec._start == _vec._end)

// return the length of the vector
#define __VEC_LEN(_vec) (_vec._end - _vec._start)

// return a pointer to the first element of the vector (null if empty)
#define __VEC_FIRST(_vec) ((__VEC_LEN(_vec) > 0) ? _vec._start : NULL)

// return a pointer to the last element of the vector (null if empty)
#define __VEC_LAST(_vec) ((__VEC_LEN(_vec) > 0) ? (_vec._end - 1) : NULL)

// growable vector of strings
struct __vec_str
{
  char** _start;
  char** _end;
  char** _mem_end;
};

// growable string
struct __str
{
  char* _start;
  char* _end;
  char* _mem_end;
};

/**
 * Parse a line of input into args.
 *
 * Escape is available for '\ ', '\n' and '\r'.
 * Otherwise, ' ' is treated as delimiter,
 * and '\n' or '\r' as end of command.
 */
struct __vec_str
parse_stdin_cmd()
{
  // init containers
  struct __vec_str args;
  struct __str str;
  __VEC_INIT(args, __ARGS_INIT_SIZE);
  __VEC_INIT(str, __STR_INIT_SIZE);

  // parser states
  char escape = 0;

  // start to parse
  for (;;) {
    int c = getchar();
    switch (c) {

      case EOF:
      case __NEWLINE:
      case __RETURN:
        if (!__VEC_EMPTY(str)) {
          // add '\0' to finish the str
          __VEC_INSERT(str, __END);
          // insert this str
          __VEC_INSERT(args, str._start);
        }
        // return args
        return args;

      case __ESC:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __ESC);
        } else {
          escape = 1;
        }
        break;

      case __SPACE:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __SPACE);
        } else if (!__VEC_EMPTY(str)) {
          // add '\0' to finish the str
          __VEC_INSERT(str, __END);
          // insert this str
          __VEC_INSERT(args, str._start);
          // init a new string
          __VEC_INIT(str, __STR_INIT_SIZE);
        }
        break;

      case __ESC_NEWLINE:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __NEWLINE);
        } else {
          __VEC_INSERT(str, c);
        }
        break;

      case __ESC_RETURN:
        if (escape) {
          escape = 0;
          __VEC_INSERT(str, __RETURN);
        } else {
          __VEC_INSERT(str, c);
        }
        break;

      default:
        __VEC_INSERT(str, c);
        break;
    }
  }
}

/**
 * Release __vec_str memory.
 */
void
free_vec_str(struct __vec_str* vec_str)
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

#endif
