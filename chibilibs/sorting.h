/* sorting.h
 *
 * Copyright (c) 2025 Paolo Giordano
 * Licensed under the MIT License. See the LICENSE at the end of this file for details
 *
 * This library provides means to sort an array of generic data types in C.
 */
#ifndef SORTING_H
#define SORTING_H
#define SORTING_IMPLEMENTATIONS

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif

/* Copy function.
 * Arguments:
 * - destination array
 * - source array
 * - size of the destination and source array
 */
static inline void s__copy(char *dest, char *source, size_t dim);

/* Insertion Sort.
 * Arguments:
 * - the vector to sort
 * - the dimension of the vector
 * - size of vector type
 * - a pointer to an ordering function
 * Return:
 * - the length of the array on success or -1 on failure
 */
int64_t s_insertion(void *input, size_t dim, size_t size, bool (*order)(const void *lhs, const void *rhs));

/* Selection Sort
 * Arguments:
 * - the vector to sort
 * - the dimension of the vector
 * - size of vector type
 * - a pointer to an ordering function
 * Return:
 * - the length of the array on success or -1 on failure
 */
int64_t s_selection(void *input, size_t dim, size_t size, bool (*order)(const void *lhs, const void *rhs));


#ifdef SORTING_IMPLEMENTATIONS

static inline void s__copy(char *dest, char *source, size_t dim) {
  for(size_t i = 0; i < dim; ++i) {
      dest[i] = source[i];
  }
}

// for increasing order, lhs < rhs. For decreasing order rhs < lhs
int64_t s_insertion(void *input, size_t dim, size_t size, bool (*order)(const void *lhs, const void *rhs)) {
  char *start = (char *)input;
  char *key = (char *)malloc(size);
  if (key == NULL) {
	return -1;
  }
  for( size_t i = 1; i < dim; ++i) {
      s__copy(key, start + i * size, size);
      size_t j = i - 1;
      while( (j != SIZE_MAX) && (*order)(key, start + j * size) ) {
		s__copy(start + (j + 1)*size, start + j * size, size);
		--j;
	  }
      s__copy(start + (j + 1)*size, key, size);
  }

  free(key);
  return (int64_t) dim;
}

// for increasing order, lhs < rhs. For decreasing order rhs < lhs
int64_t s_selection(void *input, size_t dim, size_t size, bool (*order)(const void *lhs, const void *rhs)) {
  char *start = (char *)input;

  char *temp = (char *) malloc(size);
  if (temp == NULL) {
	return -1;
  }

  for (size_t i = 0; i < dim - 1; ++i) {
	  size_t min_idx = i;
	  for (size_t j = i + 1; j < dim; j++) {
		if (order(start + j * size, start + min_idx * size)) {
			min_idx = j;
		}
	  }
	  s__copy(temp, start + min_idx * size, size);
	  s__copy(start + min_idx * size, start + i * size, size);
	  s__copy(start + i * size, temp, size);

  }

  free(temp);
  return (int64_t) dim;
}

#endif

#ifdef __cplusplus
}
#endif

#endif

/* 
MIT License

Copyright (c) 2025 Paolo Giordano

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

