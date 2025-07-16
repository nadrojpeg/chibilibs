/*
 * hash.h - Type-generic hash map implementation in pure C
 *
 * Copyright (c) 2025 Paolo Giordano
 * Licensed under the MIT License. See the end of this file for a copy of the LICENSE.
 *
 *
 * WARNING:
 * This library has been tested and is officially supported only with the MSVC compiler
 * on the Windows platform.
 *
 * It relies on x86 SSE2 SIMD instructions for performance. Support for ARM SIMD
 * (e.g., NEON) is not currently implemented or planned in the short term.
 *
 * Currently, only `uint64_t` keys are supported. Support for other key types,
 * including `char *` strings, is planned for future versions.
 *
 * Cross-platform support is also planned, but at this stage the library is designed
 * and tested specifically for Windows/MSVC environments only.
 *
 *
 * This library would never have existed without the brilliant work of the SwissTable Team.
 * I first heard of their techniques in a blog post about HashBrown, a Rust port of the
 * SwissTable algorithm. After that I discovered that HashBrown was a reimplementation of the
 * Google team's work. I then watched the YouTube video presentation of the algorithm behind
 * SwissTable. Terrific work. I can't think of a better word than 'genius' when I think about
 * the ideas behind Google's algorithm.
 *
 * Sources:
 * - https://faultlore.com/blah/hashbrown-insert/
 * - https://www.youtube.com/watch?v=ncHmEUmJZf4&t=840s
 *
 * This implementation supports two types of keys: uint64_t and const char * (currently only uint64_t)
 * By casting char, uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, and all kinds
 * of pointers to uint64_t this implementation offers a good deal of flexibility with integer keys.
 * String keys are a standard feature of hash maps.
 * I would like to implement a truly <generic key, generic val> hash map, but I don't really know
 * of an elegant or efficient way of doing this in pure C. I hope that my choice is not too restrictive
 * for future users' purposes.
 *
 * The SwissTable algorithm is built on an ingenious memory layout. This memory layout makes it possible to leverage
 * SIMD instructions (SSE2) and cache locality to achieve maximum performance. I'm new to SIMD (I
 * actually learned about SIMD by watching the YouTube video mentioned above), so this implementation may
 * not be perfect and the performance might not match the original SwissTable implementation.
 *
 * MEMORY LAYOUT:
 *
 * - An array of uint8_t metadata, divided into groups of 16 bytes. These metadata store information about each
 *   <key, value> slot: the current state (FULL, FREE, TOMB) and a 7-bit portion of the hash derived from the key.
 *   This memory layout is a core idea behind SwissTable's performance. The array is also aligned to allow efficient
 *   aligned loads into `__m128i` registers, thereby maximizing SIMD and cache performance.
 *   |    uint8_t group[16]    ||    uint8_t group[16]    ||    uint8_t group[16]    ||    uint8_t group[16]    || ...
 *
 * - An array of uint64_t keys. As mentioned before, I don't know of an elegant and efficient way to create a truly
 *   generic <key, value> hash map. To increase flexibility, I chose to use 64-bit keys that can store, via appropriate
 *   casting, any kind of 64-bit value — integers, floats, and pointers.
 *
 * - A struct that stores information about the entire map, such as its size, capacity, and the size in bytes of the value type.
 *
 * - An array that stores the user-provided values.
 *
 * The overall memory layout is:
 * | metadata[] | keys[] | info | user pointer -> values[] |
 *
 * Public macros and functions (to be used by the user):
 *
 * - hash_size: macro that "returns" the number of elements stored in the map.
 * - hash_capacity: macro that "returns" the maximum number of elements that can be stored in the map.
 * - hash_is_full: macro that checks whether a slot in the map contains a valid element (i.e., not deleted or free).
 * - hash_is_free: macro that checks whether a slot in the map is free and ready to be filled.
 * - hash_free: macro that frees the map's resources.
 * - hash_set_hash_seed: function used to set the seed that randomizes the hash function output
 * - hash_reserve: ensures the map has capacity for at least the specified number of elements, resizing the map if necessary to the next power of two.
 * - hash_get: function that returns a pointer to the element associated with a given key. Returns NULL if the element
 *   does not exist.
 * - hash_del: function that deletes the element associated with a given key. If the element does not exist, the
 *   function returns false; otherwise, it returns true.
 * - hash_put: macro that inserts a <key, value> pair into the map.
 *
 * Private macros and functions (should not be used directly by the user, unless they really want to):
 *
 * - hash__aligned_allocation: cross-platform aligned malloc macro.
 * - hash__aligned_free: cross-platform aligned free macro.
 * - hash__cast: macro that casts a pointer. This is required for C++ (in C, casting to void * is sufficient).
 * - hash__get_info: macro that "returns" a pointer to the `hash__info_t` structure.
 * - hash__get_keys: macro that "returns" a pointer to the first element of the keys array.
 * - hash__get_meta: macro that "returns" a pointer to the first element of the metadata array.
 * - hash__get_base: macro equivalent to `hash__get_meta`, used to improve clarity. The name `base` is used when
 *   performing allocation/deallocation, while `meta` is used when accessing metadata.
 * - hash__hash57: macro that extracts the 57 least significant bits from the computed hash value.
 * - hash__hash7: macro that extracts the 7 most significant bits from the computed hash value.
 * - hash__malloc: function that allocates a map with a given capacity.
 * - hash__init: macro that initializes the map to its initial capacity. A macro is used to infer the value type
 *   from the pointer type.
 * - hash__hash: the hash function used by this library.
 * - hash__rehash: function that performs rehashing after reallocating the map.
 * - hash__resize: macro that allocates a new map and rehashes the old one into it.
 * - hash__get_idx: function that searches for the position of the element associated with a given key. If found,
 *   returns true and stores the index in the provided `size_t *`. Otherwise, returns false, and the value pointed to
 *   is undefined.
 * - hash__get_freetombidx: function that returns the index of the first FREE or TOMB slot. The map is resized when its
 *   size exceeds 75% of its capacity, so it is guaranteed that at least one empty slot is available.
 *
 * USAGE:
 * The user must create a pointer to the value type they want to store in the map.
 * For example, to create a <uint64_t, type> map, they should initialize
 * a pointer like `type *tmap = NULL`. Then, this pointer can be passed to the
 * macros and functions to perform various operations.
 *
 * The hash function uses a global seed (`hash__seed`) to randomize its output.
 * This seed is defined as a static variable, so each translation unit (TU) has its own copy.
 * If multiple TUs operate on the same hash map, the user must ensure that the seed is
 * explicitly set to the same value in all of them using `hash_set_hash_seed()`.
 *
 * The library does not provide built-in thread safety. If hash maps are accessed
 * concurrently, it is the user's responsibility to enforce synchronization and ensure
 * consistent seed initialization across threads and compilation units.
*/

#ifndef CHIBI_HASH_H
#define CHIBI_HASH_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <intrin.h>
#include <emmintrin.h>

/*
 * The map capacity should be a power of two
 * in order to leverage the bitwise AND (&) operator,
 * which is faster than the modulo (%) operator.
*/
#define HASH__START_CAPACITY 16

typedef struct hash__info_t{
  size_t size;
  size_t capacity;
  size_t val_size;  // Value size in bytes, inferred from the pointer provided by the user
} hash__info_t;

// Currently only supports Windows (MSVC); cross-platform support will be added in the future.
#ifdef _MSC_VER
#include <malloc.h>
#define hash__aligned_allocation(size, align) _aligned_malloc((size), (align))
#define hash__aligned_free(ptr)               _aligned_free((ptr))
#endif

// C++ requires an explicit cast; use reinterpret_cast to preserve type informationx
#ifdef __cplusplus
#define hash__cast(map, ptr) reinterpret_cast<decltype(map)>(ptr)
#else
#define hash__cast(map, ptr) (void *)(ptr)
#endif

// Special metadata values to mark a slot as FREE or TOMB (deleted)
#define HASH__FREE 0x00
#define HASH__TOMB 0x01

// Core access and utility macros
#define hash__get_info(map) ((hash__info_t *)(map) - 1)
#define hash_size(map) ((map) ? hash__get_info(map)->size : 0)
#define hash_capacity(map) ((map) ? hash__get_info(map)->capacity : 0)
#define hash__get_keys(map) ((uint64_t *)(hash__get_info(map)) - hash_capacity(map))
#define hash__get_meta(map) ((uint8_t *)(hash__get_keys(map)) - hash_capacity(map))
#define hash__get_base(map) (hash__get_meta(map))

#define hash_is_full(b) (((b) & 0x80) == 0x80)
#define hash_is_free(b) ((b) == 0)

#define hash__hash57(h) ((h) & 0x01FFFFFFFFFFFFFF)
#define hash__hash7(h)  (((h) >> 57) & 0x7F)

#define hash_free(map) (hash__aligned_free(hash__get_base(map)))


static inline void *hash__malloc(size_t capacity, size_t val_size) {
  size_t bytes = sizeof(uint8_t) * capacity +
    sizeof(uint64_t) * capacity +
    sizeof(hash__info_t) +
    val_size * capacity;

  // performs an aligned allocation, to facilitate SSE2 load 
  void *base = hash__aligned_allocation(bytes, 16);

  return base;
}

// We use a macro to infer the value size from the map pointer provided by the user
#define hash__init(map) do {                                                                                             \
  if((map) == NULL) {                                                                                                    \
    uint8_t *base = (uint8_t *) hash__malloc(HASH__START_CAPACITY, sizeof(*(map)));                                      \
    if (base != NULL) {                                                                                                  \
      memset(base, HASH__FREE, HASH__START_CAPACITY);                                                                    \
      hash__info_t *info = (hash__info_t *)(base + HASH__START_CAPACITY + sizeof(uint64_t) * HASH__START_CAPACITY);      \
      info->size = 0;                                                                                                    \
      info->capacity = HASH__START_CAPACITY;                                                                             \
      info->val_size = sizeof(*(map));                                                                                   \
      (map) = hash__cast(map, (info + 1));                                                                               \
    }                                                                                                                    \
  }                                                                                                                      \
} while(0)

/*
 * Note: hash__seed is defined as a static variable, so each translation unit (TU) gets its own copy.
 * If multiple TUs operate on the same hash map, the user must ensure that the seed is set consistently
 * in all of them. In such cases, thread safety is also the user's responsibility.
*/
static uint64_t hash__seed   = 0x12345678ABCDEF00ULL;

static inline void hash_set_hash_seed(uint64_t seed) {
  hash__seed = seed;
}

static inline uint64_t hash__hash(uint64_t val) {
  val ^= hash__seed;
  val ^= val >> 33;
  val *= 0xff51afd7ed558ccdULL;
  val ^= val >> 33;
  val *= 0xc4ceb9fe1a85ec53ULL;
  val ^= val >> 33;
  return val;
}

static inline void hash__rehash(void *map, void *nmap) {
  size_t val_size = hash__get_info(map)->val_size;                                      
  uint8_t *base = hash__get_base(map);                                                     
  uint8_t *nbase = hash__get_base(nmap);                                                   
  uint64_t *keys = hash__get_keys(map);                                                 
  uint64_t *nkeys = hash__get_keys(nmap);                                               
  for (size_t i = 0; i < hash_capacity(map); i++) {                                     
    if(hash_is_full(base[i])) {                                                        
      uint64_t key = keys[i];                                                           
      uint64_t hash = hash__hash(key);                                                  
      size_t idx = hash__hash57(hash) & ((hash_capacity(nmap)/16) - 1);  
      idx *= 16;                                                                        
      while(!hash_is_free(nbase[idx])) {                                               
        idx = (idx + 1) & (hash_capacity(nmap) - 1);                                    
      }                                                                                 
      nbase[idx] = base[i];                                                             
      nkeys[idx] = keys[i];                                                             
      memcpy((uint8_t *)(nmap) + val_size * idx, (uint8_t *)(map) + val_size * i, val_size);  
    }
  }
}

#define hash__resize(map, ncapacity) do {                                                        \
  size_t val_size = hash__get_info(map)->val_size;                                               \
  uint8_t *nbase = (uint8_t *) hash__malloc((ncapacity), val_size);	                         \
  if (nbase != 0) {                                                                              \
    memset(nbase, HASH__FREE, (ncapacity));				                         \
    hash__info_t *info = (hash__info_t *)(nbase + (ncapacity) + sizeof(uint64_t) * (ncapacity)); \
    info->size = hash_size(map);                                                                 \
    info->capacity = (ncapacity);                                                                \
    info->val_size = val_size;                                                                   \
    hash__rehash((void *) map, (void *)(info + 1));                                              \
    hash_free(map);                                                                              \
    (map) = hash__cast(map, (info + 1));                                                         \
  }                                                                                              \
} while(0)

/*
 * The hash_reserve function allocates space for the map to hold at least the requested capacity.
 * Internally, it always rounds up the capacity to the next power of two.
 * 
 * Because of this, to avoid unnecessary memory waste, users should ideally request capacities
 * close to powers of two. For example, requesting 17,000 slots will actually allocate 32,768 slots,
 * nearly doubling the space used and potentially wasting memory.
 *
 * Additionally, the map maintains a load factor of 75%, meaning that rehashing is triggered when
 * the number of stored elements reaches 75% of the capacity. Therefore, even with a capacity of
 * 32,768, only about 24,576 elements can be stored before resizing occurs.
 *
 * Users should keep this behavior in mind when choosing capacity values to balance between
 * memory usage and performance.
*/
// WARNING: This macro has not yet been tested and may contain bugs.
#define hash_reserve(map, capacity) do {               \
    if ((map) == NULL) {                               \
        hash__init(map);                               \
    }                                                  \
    size_t cap = (capacity);                           \
    if (cap > hash_capacity(map)) {                    \
        size_t true_cap = 1ULL;                        \
        while (true_cap < cap) {                       \
            true_cap <<= 1;                            \
        }                                              \
        hash__resize((map), true_cap);                 \
    }                                                  \
} while(0)

static inline int hash__get_idx(void *map, uint64_t key, size_t *idx) {
  uint8_t *meta  = hash__get_meta(map);
  uint64_t *keys = hash__get_keys(map);
  uint64_t hash  = hash__hash(key);
  size_t m       = hash_capacity(map);
  size_t grpidx  = hash__hash57(hash) & ((m/16) - 1);
  size_t i = grpidx * 16;
  uint8_t mask   = hash__hash7(hash) | 0x80;
  __m128i vmeta;
  int match;
  int free;
  for(;;) {
    vmeta = _mm_load_si128((__m128i *)(meta + i));
    match = _mm_movemask_epi8(_mm_cmpeq_epi8(vmeta, _mm_set1_epi8(mask)));
    
    if (match != 0) {
      unsigned long off;
      while(_BitScanForward(&off, match)) {
	if (keys[i + off] == key) {
	  *idx = i + off;
	  return 1;
	}
	match &= (match - 1);
      }
    }

    free = _mm_movemask_epi8(_mm_cmpeq_epi8(vmeta, _mm_setzero_si128()));

    if (free != 0) {
      return -1;
    }

    i = (i + 16) & (m - 1);
  }
}

static inline void *hash_get(void *map, uint64_t key) {
  size_t val_size = hash__get_info(map)->val_size;
  size_t idx;
  if(hash__get_idx(map, key, &idx) == 1) {
    return (void *)((char *)(map) + val_size * idx);
  } else {
    return NULL;
  }
}

static inline bool hash_del(void *map, uint64_t key, int free_val) {
  size_t val_size = hash__get_info(map)->val_size;
  uint8_t *meta   = hash__get_meta(map);
  size_t idx;
  if(hash__get_idx(map, key, &idx) == 1) {
    meta[idx] = HASH__TOMB;
    // If the map stores dynamically allocated values,
    // this function can automatically free them.
    if (free_val) {
      void *val_ptr = *((void **)((char *)(map) + val_size * idx));
      free(val_ptr);
    }
    hash__get_info(map)->size--;
    return true;
  } else {
    return false;
  }
}

static inline size_t hash__get_freetombidx(void *map, uint64_t key) {
  uint8_t *meta  = hash__get_meta(map);
  uint64_t hash  = hash__hash(key);
  size_t m       = hash_capacity(map);
  size_t grpidx  = hash__hash57(hash) & ((m/16) -1);
  size_t i = grpidx * 16;
  __m128i vmeta;
  for (;;) {
    vmeta = _mm_load_si128((__m128i *)(meta + i));
    __m128i vmask = _mm_and_si128(vmeta, _mm_set1_epi8(0x80));
    int freetomb = _mm_movemask_epi8(_mm_cmpeq_epi8(vmask, _mm_setzero_si128()));
    if (freetomb != 0) {
      unsigned long off;
      _BitScanForward(&off, freetomb);
      return i + off;
    }

    i = (i + 16) & (m -1);
  }
}


/*
 * Inserts or updates a <key, value> pair in the map.
 * If the map is NULL, initializes it first.
 * Uses a macro to support generic value types with minimal overhead.
 * Computes the hash and probes for an existing key or a free slot.
 * Inserts the new pair or updates the existing value.
 * Increments the size accordingly.
 * Automatically resizes the map when the load factor exceeds 75%.
 * Measures performance ticks for profiling.
*/
#define hash_put(map, key, val) do{                           \
  ticks_t start = prof_get_ticks();                           \
  if ((map) == NULL) {					      \
    hash__init(map);                                          \
  }                                                           \
  uint64_t k = (key);                                         \
  uint64_t hash = hash__hash(k);                              \
  uint8_t *meta = hash__get_meta(map);                        \
  uint64_t *keys = hash__get_keys(map);                       \
  uint8_t mask = hash__hash7(hash) | 0x80;                    \
  size_t idx;                                                 \
  if(hash__get_idx(map, k, &idx) == 1) {                      \
    (map)[idx] = (val);					      \
  } else {                                                    \
    idx = hash__get_freetombidx(map, k);                      \
    meta[idx] = mask;                                         \
    keys[idx] = k;                                            \
    (map)[idx] = (val);					      \
    hash__get_info(map)->size++;                              \
  }                                                           \
  if(hash_size(map) >= (hash_capacity(map) / 4) * 3) {        \
    hash__resize(map, hash_capacity(map) * 2);                \
  }                                                           \
  perf_ticks += (prof_get_ticks() - start);                   \
  perf_count++;                                               \
} while(0)

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
