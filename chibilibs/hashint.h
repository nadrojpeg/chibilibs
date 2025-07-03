/* hashint.h - Hash table implementation with integer keys
 * Part of the chibilibs project (https://github.com/nadrojpeg/chibilibs)
 *
 * Copyright (c) 2025 Paolo Giordano
 * Licensed under the MIT License. See the LICENSE at the end of this file for details.
 *
 * A single-header, fast and lightweight hash map library written in C, using only uint64_t integer keys.
 * It employs linear probing for collision resolution and stores a hidden vector of keys and metadata before
 * the values vector. The metadata, containing size and capacity, along with the keys array, are hidden
 * from the user who interacts only with a pointer to the values. Fully compatible with C++, hashint.h
 * requires no additional setup beyond calling its functions, providing a minimal and generic-value hash map solution.
 *
 * The underlying data structure used by hashint.h consists of three contiguous arrays laid out in memory:
 * 1. A hidden vector of keys (uint64_t) used for hashing and collision resolution,
 * 2. A metadata struct containing size and capacity:
 *    struct {
 *        size_t capacity;
 *        size_t size;
 *    }
 * 3. The values vector, which is the pointer exposed to the user.
 *
 * The keys array and metadata are stored just before the values array and are managed internally,
 * keeping the implementation details hidden from the user.
 *
 * Note: The user should not access the values array directly as uninitialized fields may contain garbage data.
 *
 * The approach of storing metadata and keys before the values array in hashint.h
 * offers an internal advantage: it allows the library to conveniently manage the
 * values by having a correctly typed pointer directly to the start of the values array.
 * This layout keeps the internal structure organized and efficient without exposing
 * metadata or keys to the user.
 *
 * Public API of hashint.h (available as functions and macros):
 *
 * - hashi_set_seed: sets the global seed used by the hash function.
 * - hashi_put: inserts a key-value pair into the hash map.
 * - hashi_get: retrieves a pointer to the value associated with a given key.
 * - hashi_del: removes a key-value pair from the hash map.
 * - hashi_size: returns the current number of elements in the hash map.
 * - hashi_capacity: returns the current capacity of the hash map.
 *
 * Private API of hashint.h (should not be used directly by the user, unless they really want to):
 * - hashi__get_metadata: returns pointer to metadata before the values array.
 * - hashi__get_keys: returns pointer to the hidden keys array before metadata.
 * - hashi__cast: casts a pointer to the type of the table.
 * - hashi__alloc: allocates and initializes the hash map with initial capacity.
 * - hashi__rehash: rehashes all keys and values into a new table.
 * - hashi__realloc: reallocates and grows the table when load factor exceeds threshold.
 * - hashi__swap: helper function for hash mixing.
 * - hashi__f: internal hash mixing function.
 * - hashi__hash: final hash function combining mixing steps.
 * - hashi__get_slot: finds the slot index of a given key if present.
 * - hashi__hash_inv: calculates distance for probing.
*/
#ifndef CHIBI_HASHINT_H
#define CHIBI_HASHINT_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define HASHI_START_CAPACITY   8
#define HASHI_EMPTY            UINT64_MAX
#define HASHI_WORD             64
#define HASHI_A                11400714819323198485ULL // golden ratio

static uint64_t hashi_seed = 0x12345678ABCDEF00ULL;

#ifdef __cplusplus
extern "C"
{
#endif

void hashi_set_seed(uint64_t seed);

#ifdef HASHI_IMPLEMENTATIONS
inline void hashi_set_seed(uint64_t seed) {
    hashi_seed = seed;
}
#endif

#ifdef __cplusplus
}
#endif

typedef struct {
    size_t size;
    size_t capacity;
} hashi__metadata_t;

#define hashi__get_metadata(table) ((hashi__metadata_t *)(table) - 1)
#define hashi_size(table) ((table) == NULL ? 0 : (hashi__get_metadata(table)->size))
#define hashi_capacity(table) ((table) == NULL ? 0 : (hashi__get_metadata(table)->capacity))
#define hashi__get_keys(table) ((uint64_t *)(hashi__get_metadata(table)) - hashi_capacity(table))
#define hashi_free(table) (free(hashi__get_keys(table)))

#ifdef __cplusplus
    #define hashi__cast(pointer, table) reinterpret_cast<decltype(table)>(pointer)
#else
    #define hashi__cast(pointer, table) (void *)(pointer)
#endif

#define hashi__alloc(table) do {                                                                                            \
    size_t val_size = sizeof(*(table));                                                                                     \
    size_t size = HASHI_START_CAPACITY * sizeof(uint64_t) + sizeof(hashi__metadata_t) + HASHI_START_CAPACITY * val_size;    \
    uint64_t *base = (uint64_t *) malloc(size);                                                                             \
    hashi__metadata_t *metadata = (hashi__metadata_t *)(base + HASHI_START_CAPACITY);                                       \
    if (metadata != NULL) {                                                                                                 \
        memset(base, 0xFF, HASHI_START_CAPACITY * sizeof(uint64_t));                                                        \
        metadata->size = 0;                                                                                                 \
        metadata->capacity = HASHI_START_CAPACITY;                                                                          \
        table = hashi__cast(metadata + 1, table);                                                                           \
    }                                                                                                                       \
} while(0)                                                                                                                  \

#ifdef __cplusplus
extern "C"
{
#endif

uint64_t hashi__swap(uint64_t x);
uint64_t hashi__f(uint64_t k);
uint64_t hashi__hash(uint64_t k);

#ifdef HASHI_IMPLEMENTATIONS
inline uint64_t hashi__swap(uint64_t x) {
    return (x >> (HASHI_WORD / 2)) + (x << (HASHI_WORD / 2));
}

inline uint64_t hashi__f(uint64_t k) {
    return hashi__swap(k * (2 * (k + HASHI_WORD) + HASHI_A));
}

inline uint64_t hashi__hash(uint64_t k) {
    return hashi__f(hashi__f(hashi__f(hashi__f(k + hashi_seed))));
}
#endif

#ifdef __cplusplus
}
#endif

#define hashi__rehash(table, new_table) do {                                                                 \
    size_t val_size = sizeof(*(table));                                                                      \
    uint64_t *t_base = hashi__get_keys(table);                                                               \
    uint64_t *nt_base = hashi__get_keys(new_table);                                                          \
    size_t m = hashi_capacity(new_table);                                                                    \
    for (size_t i = 0; i < hashi_capacity(table); i++) {                                                     \
        uint64_t key = t_base[i];                                                                            \
        if (key != HASHI_EMPTY) {                                                                            \
            size_t new_idx = hashi__hash(key) & (m - 1);                                                     \
            while(nt_base[new_idx] != HASHI_EMPTY) {                                                         \
                new_idx = (new_idx + 1) & (m - 1);                                                           \
            }                                                                                                \
            nt_base[new_idx] = key;                                                                          \
            memcpy((void *)((char *)(new_table) + new_idx * val_size), (void *)(table + i), val_size);       \
        }                                                                                                    \
    }                                                                                                        \
} while(0)                                                                                                   \

#define hashi__realloc(table) do {                                                                           \
    size_t val_size = sizeof(*(table));                                                                      \
    size_t new_capacity = 2 * hashi_capacity(table);                                                         \
    size_t size = new_capacity * sizeof(uint64_t) + sizeof(hashi__metadata_t) + new_capacity * val_size;     \
    uint64_t *new_base = (uint64_t *) malloc(size);                                                          \
    if (new_base != NULL) {                                                                                  \
        memset(new_base, 0xFF, new_capacity * sizeof(uint64_t));                                             \
        hashi__metadata_t *metadata = (hashi__metadata_t *)(new_base + new_capacity);                        \
        if (new_base != NULL) {                                                                              \
            metadata->size = hashi_size(table);                                                              \
            metadata->capacity = new_capacity;                                                               \
            hashi__rehash(table, metadata + 1);                                                              \
            free(hashi__get_keys(table));                                                                    \
            table = hashi__cast(metadata + 1, table);                                                        \
        }                                                                                                    \
    }                                                                                                        \
} while(0)                                                                                                   \

#define hashi_put(table, key, val) do {                                      \
    if ((table) == NULL) {                                                   \
        hashi__alloc(table);                                                 \
    }                                                                        \
    if (hashi_size(table) >= (hashi_capacity(table) / 4) * 3) {              \
        hashi__realloc(table);                                               \
    }                                                                        \
    uint64_t k = (uint64_t)(key);                                            \
    if (k == HASHI_EMPTY) {                                                  \
        k = 0;                                                               \
    }                                                                        \
    uint64_t *base = hashi__get_keys(table);                                 \
    size_t m = hashi_capacity(table);                                        \
    size_t idx = hashi__hash(k) & (m - 1);                                   \
    while (base[idx] != HASHI_EMPTY && base[idx] != k) {                     \
        idx = (idx + 1) & (m - 1);                                           \
    }                                                                        \
    if (base[idx] == HASHI_EMPTY) {                                          \
        hashi__get_metadata(table)->size++;                                  \
    }                                                                        \
    base[idx] = (k);                                                         \
    table[idx] = (val);                                                      \
} while(0)                                                                   \


#ifdef __cplusplus
extern "C"
{
#endif

bool hashi__get_slot(void *table, size_t val_size, uint64_t key, uint64_t *slot);
void *hashi_get(void *table, size_t val_size, uint64_t key);
uint64_t hashi__hash_inv(void *table, uint64_t key, uint64_t idx);
bool hashi_del(void *table, size_t val_size, uint64_t key);

#ifdef HASHI_IMPLEMENTATIONS
inline bool hashi__get_slot(void *table, size_t val_size, uint64_t key, uint64_t *slot) {
    if (key == HASHI_EMPTY) {key = 0;}
    size_t idx = hashi__hash(key) % hashi_capacity(table);
    uint64_t *base = hashi__get_keys(table);
    while (base[idx] != HASHI_EMPTY && base[idx] != key) {
        idx = (idx + 1) & (hashi_capacity(table) - 1);
    }
    *slot = idx;

    return base[idx] == key;
}

inline void *hashi_get(void *table, size_t val_size, uint64_t key) {
    if (key == HASHI_EMPTY) {key = 0;}
    uint64_t slot;
    if (hashi__get_slot(table, val_size, key, &slot)) {
        return (void *)((char *)(table) + slot * val_size);
    } else {
        return NULL;
    }
}

inline uint64_t hashi__hash_inv(void *table, uint64_t key, uint64_t idx) {
    return (idx - hashi__hash(key)) & (hashi_capacity(table) - 1);
}

inline bool hashi_del(void *table, size_t val_size, uint64_t key) {
    uint64_t idx;
    if (hashi__get_slot(table, val_size, key, &idx)) {
        uint64_t *base = hashi__get_keys(table);
        uint64_t new_idx;
        uint64_t new_key;
        for(;;) {
            base[idx] = HASHI_EMPTY;
            new_idx = idx;
            do {
                new_idx = (new_idx + 1) & (hashi_capacity(table) - 1);
                new_key = base[new_idx];
                if (new_key == HASHI_EMPTY) {
                    return true;
                }
            } while (hashi__hash_inv(table, new_key, new_idx) < hashi__hash_inv(table, new_key, idx));
            base[idx] = new_key;
            memcpy((void *)((char*)(table) + idx * val_size), (void *)((char*)(table) + new_idx * val_size), val_size);
            idx = new_idx;
        }
        return true;
    } else {
        return false;
    }
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
