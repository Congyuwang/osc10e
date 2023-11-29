#include <stdlib.h>

#ifndef _UTILITY_H
#define _UTILITY_H 1

struct __vec_str
{
  char** _start;
  char** _end;
  char** _mem_end;
};

struct __str
{
  char* _start;
  char* _end;
  char* _mem_end;
};

// check malloc error (exit on error)
#define __CHECKED_MALLOC(_ptr, _init_size)                                     \
  _ptr = (typeof(_ptr))malloc(sizeof(*_ptr) * _init_size);                     \
  if (_ptr == NULL) {                                                          \
    exit(EXIT_FAILURE);                                                        \
  }

// check realloc error (exit on error)
#define __CHECKED_REALLOC(_ptr, _size)                                         \
  {                                                                            \
    void* _new_ptr = realloc(_ptr, sizeof(*_ptr) * _size);                     \
    if (_new_ptr == NULL) {                                                    \
      free(_ptr);                                                              \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
    _ptr = (typeof(_ptr))_new_ptr;                                             \
  }

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
  {                                                                            \
    typeof(*_vec._start) _ele = _element;                                      \
    *(_vec._end++) = _ele;                                                     \
  }

// check if vector is empty
#define __VEC_EMPTY(_vec) (_vec._start == _vec._end)

// return the length of the vector
#define __VEC_LEN(_vec) (_vec._end - _vec._start)

// return a pointer to the first element of the vector (null if empty)
#define __VEC_FIRST(_vec) ((__VEC_LEN(_vec) > 0) ? _vec._start : NULL)

// return a pointer to the last element of the vector (null if empty)
#define __VEC_LAST(_vec) ((__VEC_LEN(_vec) > 0) ? (_vec._end - 1) : NULL)

#endif
