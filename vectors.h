/* vectors.h
 *
 * Copyright (c) 2025 Paolo Giordano
 * Licensed under the MIT License. See the LICENSE at the end of this file for details.
 * 
 * A single-header, fast and lightweight macro-based implementation of dynamic arrays in C. 
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
 *     type *data;
 * }
 * 
 * The approach of storing metadata before the data array has two key advantages:
 *
 * 1. It encapsulates certain data, preventing the user from directly accessing
 *    or modifying the metadata, ensuring better control over the internal workings
 *    of the vector.
 *
 * 2. It allows the user to access and modify the vector elements using the same syntax as
 *    regular arrays, e.g. "vec[i]";
 * 
 * Public Macros (to be used by the user):
 * - Vector(type): creates a pointer to type (for code readability)
 * - Newvector(): initialize the pointer to NULL (for code readability)
 * - v_free: frees the vector
 * - v_capacity: returns the capacity of the vector, which is the maximum number of elements 
 *   the vector can hold without needing to reallocate memory.
 * - v_size: returns the size of the vector, which is the number of elements currently held by
 *   the vector
 * - v_push_back: adds an element to the back of the vector. If the vector is not allocated, 
 *   memory will be allocated. If the vector does not have enough space, it will reallocate 
 *   the vector, doubling its capacity.
 * - v_insert: insert an element at a specified index. It checks wheater the index is valid (i.e., whitin range)
 * - v_remove: removes an element from a specified index. It checks wheater the index is valid (i.e., whitin range)
 * - v_pop_front: removes the first element of the vector
 * - v_pop_back: removes the last element of the vector
 * - v_shrink_to_fit: shrinks the vector's capacity to fit it's current size
 * 
 * Private Macros (should not be used directly by the user, unless they really want to):
 * - _v_alloc_: performs the initial allocation
 * - _v_double_capacity_: reallocates memory, doubling the vector's capacity
 * - _v_raw_info_: returns a pointer to the vector's metadata
 */

#ifndef VECTORS_H
#define VECTORS_H

#include <stdlib.h>

// Disable C++ name mangling
#ifdef __cplusplus
extern "C"
{
#endif

/* Cast from one pointer type to another, necessary for C++ compatibility */
#ifdef __cplusplus
    #define cast(vec, var) (reinterpret_cast<decltype(vec)>(var))
#else 
    #define cast(vec, var) ((void *)(var))
#endif

/* These two macros were created solely to improve code readability.
 * Example:
 * Using 
 * "Vector(int) iVec = NewVector();" 
 * is equivalent to:
 * "int *iVec = NULL;"
 */
#define Vector(type) type *
#define NewVector() NULL

#define V_START_CAPACITY 8

/* This struct holds vector metadata:
 * capacity: maximum number of elements the vector can hold without needing to reallocate memory
 * size: number of elements currently held by the vector
 */
typedef struct v_info {
    size_t capacity;
    size_t size;
} v_info;

/* Performs the initial allocation, allocating enough space for the vector's metadata (v_info)
 * and for V_START_CAPACITY (8) elements of the desired type. This function does not infer the
 * data type; it simply uses the size of the type to allocate the correct amount of memory.
 * In case of malloc failure, the vector is left unchanged
 * (Should not be used directly by the user)
 */
#define _v_alloc_(vec) if(vec == NULL) { \
    v_info *_raw = (v_info *) malloc(sizeof(v_info) + sizeof(*vec) * V_START_CAPACITY); \
    if (_raw != NULL) { \
        _raw->capacity = V_START_CAPACITY; \
        _raw->size = 0; \
        vec = cast(vec, _raw + 1); \
    } \
} \

/* Frees the allocated memory and sets the vector pointer to NULL
 */
#define v_free(vec) (free(_v_raw_info_(vec)), vec=NULL)

/* Returns a pointer to the vector's metadata. The pointer is of type (v_info *).
 * (Should not be used directly by the user)
 */
#define _v_raw_info_(vec) ((v_info *) ((char *) (vec) - sizeof(v_info)))

/* Returns the vector's capacity as a size_t
 */
#define v_capacity(vec) ( (vec == NULL) ? 0 : _v_raw_info_(vec)->capacity )

/* Returns the vector's size as a size_t
 */
#define v_size(vec) ( (vec == NULL) ? 0 : _v_raw_info_(vec)->size )

/* Reallocates memory, doubling the vector's capacity. If the reallocation fails,
 * nothing happens.
 * In case of realloc failure, the vector is left unchanged
 * (Should not be used directly by the user)
 */
#define _v_double_capacity_(vec) if(vec != NULL) { \
    v_info *_raw = (v_info*) realloc((void *) _v_raw_info_(vec), sizeof(v_info) + sizeof(*vec) * v_capacity(vec) * 2); \
    if (_raw != NULL) { \
        _raw->capacity *= 2; \
        vec = cast(vec, _raw + 1); \
    } \
} \

/* Adds an element to the back of the vector. If the vector is not allocated, 
 * memory will be allocated. If the vector does not have enough space, it will reallocate 
 * the vector, doubling its capacity. If allocation or reallocation fail, the element will
 * not be added.
 */
#define v_push_back(vec, value) \
    if (vec == NULL) {_v_alloc_(vec);} \
    if (v_capacity(vec) - v_size(vec) == 0) { \
        _v_double_capacity_(vec); \
    } \
    if (v_capacity(vec) - v_size(vec) != 0) { \
        vec[_v_raw_info_(vec)->size++] = value; \
    } \

/* Removes the last element from the vector 
 */
#define v_pop_back(vec) \
    if ((vec != NULL) && v_size(vec)) { \
        _v_raw_info_(vec)->size -= 1; \
    } \

/* Shrinks the vector's capacity to fit it's current size
 * In case of realloc failure, the vector is left unchanged
 */
#define v_shrink_to_fit(vec) \
    if (vec != NULL) { \
        v_info *_raw = (v_info*) realloc((void *)_v_raw_info_(vec), v_size(vec) * sizeof(*vec) + sizeof(v_info)); \
        if (_raw != NULL) { \
            /* capacity = size */ \
            _v_raw_info_(vec)->capacity = _v_raw_info_(vec)->size; \
            vec = cast(vec, _raw + 1); \
        } \
    } \

/* Inserts an element at a specified index. It checks whether the index is valid (i.e., within range).
 * 'i' should be a size_t variable.
 * 'value' should be of the correct type, as this macro will not perform a cast, so the user is responsible
 * for ensuring the value is of the correct type.
 */
#define v_insert(vec, i, value) \
    if (vec != NULL) { \
        if (((i) >= 0) && ((i) < v_size(vec))) { \
            if (v_capacity(vec) - v_size(vec) == 0) { \
                _v_double_capacity_(vec); \
            } \
            if (v_capacity(vec) - v_size(vec) != 0) { \
                for (size_t j = v_size(vec) - 1; j >= i; j--) { \
                    vec[j+1] = vec[j]; \
                } \
                vec[i] = value; \
                _v_raw_info_(vec)->size++; \
            } \
        } \
    } \

/* removes an element from a specified index. It checks wheater the index is valid (i.e., whitin range)
 */
#define v_remove(vec, i) \
    if((vec != NULL) && v_size(vec)) { \
        if (((i) >= 0) && ((i) < v_size(vec))) { \
            for(size_t j = i + 1; j <= v_size(vec); j++) { \
                vec[j - 1] = vec[j]; \
            } \
            _v_raw_info_(vec)->size--; \
        } \
    } \

/* removes the first element of the vector.
 */
#define v_pop_front(vec) v_remove(vec, 0)

// Disable C++ name mangling
#ifdef __cplusplus
};
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
