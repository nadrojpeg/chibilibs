/* sorting.h
 *
 * Copyright (c) 2025 Paolo Giordano
 * Licensed under the MIT License. See the LICENSE at the end of this file for details
 *
 * This library provides means to sort array of generic data types in C.
 */
#ifndef SORTING_H
#define SORTING_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>

/* Copy function, substitution of memcpy.
 * Arguments:
 * - sizeof the destination and source array
 * - destination array
 * - source array
 * Outputs:
 * - no output, the vector are simply copied
 */
static inline void copy(char *dest, char *source, size_t dim)
{
  for(size_t i = 0; i < dim; ++i)
    {
      dest[i] = source[i];
    }
}

/* Insertion Sort.
 * Arguments:
 * - size of vector type
 * - the vector to sort
 * - the dimension of the vector
 * - a pointer to an ordering function
 * Output:
 * - no output, the vector is sorted in loco
 */
void insertion_sort(size_t size_of,
		    void *input,
		    size_t dim,
		    bool (*order)(const void *lhs, const void *rhs) )
{
  char *start = (char *)input;
  for( size_t i = 1; i < dim; ++i)
    {
      char *key = (char *)malloc(size_of);
      copy(key, start + i * size_of, size_of);
      size_t j = i - 1;
      while( (j != SIZE_MAX) && (*order)(key, start + j * size_of) )
	{
	  copy(start + (j + 1)*size_of, start + j * size_of, size_of);
	  --j;
	}
      copy(start + (j + 1)*size_of, key, size_of);
    }
}

/* Selection Sort
 * Arguments:
 * - size of vector type
 * - the vector
 * - the dimension of the vector
 * - a pointer to an ordering function
 * Output:
 * - no output, the vector is sorted in loco
 */
void selection_sort(size_t size_of,
		    void *input,
		    size_t dim,
		    bool (*order)(const void *lhs, const void *rhs))
{
  char *start = (char *)input;
  for (size_t i = 0; i < dim - 1; ++i)
    {
      size_t sel_idx = i;           // index of the selected item
      for (size_t j = i + 1; j < dim; ++j)
	{
	  if ( order(start + size_of * j, start + size_of * sel_idx) )
	    {
	      sel_idx = j;
	    }
	}
      if ( sel_idx != i)
	{
	  char *temp = (char *)malloc(size_of);
	  copy(temp, start + i * size_of, size_of);
	  copy(start + i * size_of, start + sel_idx * size_of, size_of);
	  copy(start + sel_idx * size_of, temp, size_of);
	}
    }
}

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

