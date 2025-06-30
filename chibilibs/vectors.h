/* vectors.h
 *
 * Copyright (c) 2025 Paolo Giordano
 * Licensed under the MIT License. See the LICENSE at the end of this file for details.
 * 
 * A single-header, fast and lightweight macro-based implementation of dynamic arrays written in C. 
 * Provides C++-like (std::vector) functionality for creating and manipulating 
 * dynamic arrays.
 * This data structure is implemented by allocating memory for metadata 
 * about the vector immediately before the address of the first element.
 * This approach is inspired by the work of Sean T. Barrett (https://www.nothings.org/), author of "stb_ds.h".
 * 
 * The underlying data structure used by the vector is of the following type:
 * struct { 
 *     size_t capacity;
 *     size_t size;
 * }
 * 
 * The approach of storing metadata before the data array has two key advantages:
 *
 * 1. It encapsulates certain data, preventing the user from directly accessing
 *    or modifying the metadata, ensuring better control over the internal workings
 *    of the vector.
 *
 * 2. It allows the user to access and modify the vector elements using the same syntax as
 *    regular arrays, e.g. "vec[i]".
 * 
 * Public Macros (to be used by the user):
 * - v_free: frees the vector.
 * - v_capacity: returns the capacity of the vector, which is the maximum number of elements 
 *   the vector can hold without needing to reallocate memory.
 * - v_size: returns the size of the vector, which is the number of elements currently held by
 *   the vector.
 * - v_push_back: adds an element to the back of the vector. If the vector is not allocated, 
 *   memory will be allocated. If the vector does not have enough space, it will reallocate 
 *   the vector, doubling its capacity.
 * - v_insert: inserts an element at a specified index.
 * - v_remove: removes an element from a specified index.
 * - v_pop_front: removes the first element of the vector.
 * - v_pop_back: removes the last element of the vector.
 * - v_shrink_to_fit: shrinks the vector's capacity to fit its current size.
 * 
 * Private Macros (should not be used directly by the user, unless they really want to):
 * - v__alloc: performs the initial allocation.
 * - v__double_capacity: reallocates memory, doubling the vector's capacity.
 * - v__get_metadata: returns a pointer to the vector's metadata.
 */

#ifndef VECTORS_H
#define VECTORS_H

#include <stdlib.h>

#define V_START_CAPACITY 8

#ifdef __cplusplus
    #define v__cast(vec, p) reinterpret_cast<decltype(vec)>(p)
#else 
    #define v__cast(vec, p) (void *)(p)
#endif

/* This struct holds vector metadata:
 * capacity: maximum number of elements the vector can hold without needing to reallocate memory
 * size: number of elements currently held by the vector
*/
typedef struct v__metadata_t {
    size_t capacity;
    size_t size;
} v__metadata_t;

/* Returns a pointer to the vector's metadata. The pointer is of type (v_info *).
 * (Should not be used directly by the user)
*/
#define v__get_metadata(vec) ((v__metadata_t *) (vec) - 1)

/* Performs the initial allocation, allocating enough space for the vector's metadata (v_info)
 * and for V_START_CAPACITY (8) elements of the desired type. This function does not infer the
 * data type; it simply uses the size of the type to allocate the correct amount of memory.
 * In case of malloc failure, the vector is left unchanged
 * (Should not be used directly by the user)
*/
#define v__alloc(vec) do {                                                                                          \
    if ((vec) == NULL) {                                                                                            \
      v__metadata_t *metadata = (v__metadata_t *) malloc(sizeof(v__metadata_t) + sizeof(*(vec)) * V_START_CAPACITY);\
      if (metadata != NULL) {                                                                                       \
        metadata->capacity = V_START_CAPACITY;                                                                      \
        metadata->size = 0;                                                                                         \
        (vec) = v__cast(vec, (metadata + 1));                                                                       \
      }                                                                                                             \
    }                                                                                                               \
  } while (0)                                                                                                       \

/* Frees the allocated memory and sets the vector pointer to NULL
*/
#define v_free(vec) free(v__get_metadata(vec))

/* Returns the vector's capacity as a size_t
*/
#define v_capacity(vec) (((vec) == NULL) ? 0 : v__get_metadata(vec)->capacity)

/* Returns the vector's size as a size_t
*/
#define v_size(vec) (((vec) == NULL) ? 0 : v__get_metadata(vec)->size)

/* Reallocates memory, doubling the vector's capacity. If the reallocation fails,
 * nothing happens.
 * In case of realloc failure, the vector is left unchanged
 * (Should not be used directly by the user)
*/
#define v__double_capacity(vec) do {                                                                    \
    size_t new_capacity = sizeof(v__metadata_t) + sizeof(*(vec)) * v_capacity(vec) * 2;                 \
    v__metadata_t *metadata = (v__metadata_t *) realloc( (void *) v__get_metadata(vec), new_capacity);  \
    if (metadata != NULL) {                                                                             \
      metadata->capacity *= 2;                                                                          \
      (vec) = v__cast(vec, (metadata + 1));                                                             \
    }                                                                                                   \
  } while (0)                                                                                           \

/* Adds an element to the back of the vector. If the vector is not allocated (vec == NULL), 
 * memory will be allocated. If the vector does not have enough space, it will reallocate 
 * the vector, doubling its capacity. If allocation or reallocation fail, the element will
 * not be added.
*/
#define v_push_back(vec, val) do {                \
  v__alloc(vec);                                  \
  if (v_capacity(vec) - v_size(vec) == 0) {       \
    v__double_capacity(vec);                      \
  }                                               \
  (vec)[v__get_metadata(vec)->size++] = (val);	  \
  } while (0)                                     \

/* Removes the last element from the vector.
 * Does not check whether vec is NULL
*/
#define v_pop_back(vec)                    \
    if (v_size(vec) > 0) {                 \
        v__get_metadata(vec)->size -= 1;   \
    }                                      \

/* Shrinks the vector's capacity to fit it's current size.
 * In case of realloc failure, the vector is left unchanged.
 * Does not check whether vec is NULL.
*/
#define v_shrink_to_fit(vec) do {                                                                               \
    size_t shrink_capacity = sizeof(v__metadata_t) + v_size(vec) * sizeof(*(vec));                              \
    v__metadata_t *metadata = (v__metadata_t *) realloc((void *) v__get_metadata(vec), shrink_capacity);        \
    if (metadata != NULL) {                                                                                     \
        metadata->capacity = metadata->size;                                                                    \
        (vec) = v__cast(vec, (metadata + 1));                                                                   \
    }                                                                                                           \
  } while (0)                                                                                                   \

/* Inserts an element at a specified index.
 * 'i' should be non-negative and less than the vector size.
 * 'value' should be of the correct type, as this macro will not perform a cast, so the user is responsible
 * for ensuring the value is of the correct type.
 * Does not check whether vec is NULL or whether the index is in range.
*/
#define v_insert(vec, i, val) do {                             \
    v__alloc(vec);                                             \
    if (v_capacity(vec) - v_size(vec) == 0) {                  \
      v__double_capacity(vec);                                 \
    }                                                          \
    for (size_t j = v_size(vec); j > i; j--) {                 \
      (vec)[j] = (vec)[j - 1];                                 \
    }                                                          \
    (vec)[i] = (val);                                          \
    v__get_metadata(vec)->size++;                              \
  } while (0)                                                  \

/* Removes an element from a specified index.
 * Does not check whether vec is NULL or whether the index is in range.
*/
#define v_remove(vec, i) do {                              \
    for(size_t j = i + 1; j < v_size(vec); j++) {          \
      (vec)[j - 1] = (vec)[j];                             \
    }                                                      \
    v__get_metadata(vec)->size--;                          \
  } while (0)                                              \

/* Removes the first element of the vector.
*/
#define v_pop_front(vec) v_remove(vec, 0)

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

