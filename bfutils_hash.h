/* bfutils_hash.h

DESCRIPTION: 

    This is a single-header-file library that provides a hashmap for C.

USAGE:
    
    In one source file put:
        #define BFUTILS_HASH_IMPLEMENTATION
        #include "bfutils_hash.h"
    
    Other source files should contain only the import line.

    To use bellow functions you need to have a type T containing an a TK key and a TV value.
    Then just declare: T *hashmap = NULL

    If the key is a string, use the string_hash functions.

    Functions (macros):

        hash_header:
            BFUtilsHashHeader *hash_header(T*); Return a pointer to the hashmap header.

        hash_push:
            void hash_push(T*, TK, TV); Inserts an element to the hashmap.

        hash_get:
            TV hash_get(T*, TK); Returns an element value from the hashmap.

        hash_remove:
            TV hash_remove(T*, TK); Removes and returns an element from the hashmap.

        hash_contains:
            int hash_contains(T*, TK); Returns a non-zero value if the hashmap contains the TK key.

        string_hash_push:
            void string_hash_push(T*, const char*, TV); Inserts an element to the hashmap.

        string_hash_get: 
            TV string_hash_get(T*, const char *); Returns an element value from the hashmap.

        string_hash_remove: 
            TV string_hash_remove(T*, const char*); Removes and returns an element from the hashmap.

        string_hash_contains:
            int hash_contains(T*, const char*); Returns a non-zero value if the hashmap contains the char* key.

        hash_free:
            void hash_free(T*); Frees the hashmap.

    Compile-time options:
        
        #define BFUTILS_HASH_NO_SHORT_NAME
        
            This flag needs to be set globally.
            By default this file exposes functions without bfutils_ prefix.
            By defining this flag, this library will expose only functions prefixed with bfutils_

        #define BFUTILS_HASH_MALLOC another_malloc
        #define BFUTILS_HASH_CALLOC another_calloc
        #define BFUTILS_HASH_REALLOC another_realloc
        #define BFUTILS_HASH_FREE another_free

            These flags needs to be set only in the file containing #define BFUTILS_HASH_IMPLEMENTATION
            If you don't want to use 'stdlib.h' memory functions you can define these flags with custom functions.

LICENSE:

    MIT License
    
    Copyright (c) 2024 Bruno Flávio Ferreira 
    
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

#ifndef BFUTILS_HASH_H
#define BFUTILS_HASH_H

#ifndef BFUTILS_HASH_NO_SHORT_NAME

#define hash_header bfutils_hash_header
#define hash_push bfutils_hash_push
#define hash_get bfutils_hash_get
#define hash_remove bfutils_hash_remove
#define hash_contains bfutils_hash_contains
#define string_hash_push bfutils_string_hash_push
#define string_hash_get bfutils_string_hash_get
#define string_hash_remove bfutils_string_hash_remove
#define string_hash_contains bfutils_string_hash_contains
#define hash_free bfutils_hash_free

#endif //BFUTILS_HASH_NO_SHORT_NAME

#if ((defined(BFUTILS_HASH_REALLOC) && (!defined(BFUTILS_HASH_MALLOC) || !defined(BFUTILS_HASH_CALLOC) || !defined(BFUTILS_HASH_FREE))) \
    || (defined(BFUTILS_HASH_MALLOC) && (!defined(BFUTILS_HASH_REALLOC) || !defined(BFUTILS_HASH_CALLOC) || !defined(BFUTILS_HASH_FREE))) \
    || (defined(BFUTILS_HASH_CALLOC) && (!defined(BFUTILS_HASH_MALLOC) || !defined(BFUTILS_HASH_REALLOC) || !defined(BFUTILS_HASH_FREE))) \
    || (defined(BFUTILS_HASH_FREE) && (!defined(BFUTILS_HASH_MALLOC) || !defined(BFUTILS_HASH_REALLOC) || !defined(BFUTILS_HASH_CALLOC))))
#error "You must define all BFUTILS_HASH_REALLOC, BFUTILS_HASH_CALLOC, BFUTILS_HASH_MALLOC, BFUTILS_HASH_FREE or neither."
#endif

#ifndef BFUTILS_HASH_REALLOC
#include <stdlib.h>
#define BFUTILS_HASH_REALLOC realloc
#define BFUTILS_HASH_CALLOC calloc
#define BFUTILS_HASH_MALLOC malloc
#define BFUTILS_HASH_FREE free
#endif //BFUTILS_HASH_REALLOC

#include <stddef.h>
#include <string.h>

typedef struct {
    size_t insert_count;
    size_t length;
    unsigned char *slots;
    unsigned char *removed;
} BFUtilsHashHeader;

#define bfutils_hash_header(h) ((h) ? (BFUtilsHashHeader *)(h) - 1 : NULL)
#define bfutils_hash_insert_count(h) ((h) ? bfutils_hash_header((h))->insert_count : 0)
#define bfutils_hash_length(h) ((h) ? bfutils_hash_header((h))->length : 0)
#define bfutils_hash_slots(h) ((h) ? bfutils_hash_header((h))->slots : NULL)
#define bfutils_hash_removed(h) ((h) ? bfutils_hash_header((h))->removed : NULL)
#define bfutils_hash_push(h, k, v) { \
    (h) = bfutils_hash_resize((h), sizeof(*(h)), offsetof(typeof(*(h)), key), sizeof((k)), 0); \
    size_t pos = bfutils_hash_insert_position((h), &(k), sizeof(*(h)), offsetof(__typeof__(*(h)), key), sizeof((k)), 0); \
    (h)[pos].key = (k); \
    (h)[pos].value = (v); \
}
#define bfutils_hash_get(h, k) ((h)[bfutils_hash_get_position((h), &(k), sizeof(*(h)), offsetof(__typeof__(*(h)), key), sizeof((k)), 0)].value)
#define bfutils_hash_contains(h, k) (bfutils_hash_get_position((h), &(k), sizeof(*(h)), offsetof(__typeof__(*(h)), key), sizeof((k)), 0) > 0)
#define bfutils_hash_remove(h, k) ((h)[bfutils_hash_remove_key((h), &(k), sizeof(*(h)), offsetof(__typeof__(*(h)), key), sizeof((k)), 0)].value)
#define bfutils_string_hash_push(h, k, v) { \
    (h) = bfutils_hash_resize((h), sizeof(*(h)), offsetof(typeof(*(h)), key), sizeof((k)), 1); \
    size_t pos = bfutils_hash_insert_position((h), (k), sizeof(*(h)), offsetof(__typeof__(*(h)), key), sizeof((k)), 1); \
    (h)[pos].key = (k); \
    (h)[pos].value = (v); \
}
#define bfutils_string_hash_get(h, k) ((h)[bfutils_hash_get_position((h), (k), sizeof(*(h)), offsetof(__typeof__(*(h)), key), sizeof((k)), 1)].value)
#define bfutils_string_hash_contains(h, k) (bfutils_hash_get_position((h), (k), sizeof(*(h)), offsetof(__typeof__(*(h)), key), sizeof((k)), 1) > 0)
#define bfutils_string_hash_remove(h, k) ((h)[bfutils_hash_remove_key((h), (k), sizeof(*(h)), offsetof(__typeof__(*(h)), key), sizeof((k)), 1)].value)
#define bfutils_hash_free(h) (bfutils_hash_free_f((h)), (h) = NULL)

extern void *bfutils_hash_resize(void *hm, size_t element_size, size_t key_offset, size_t key_size, int is_string);
extern size_t bfutils_hash_insert_position(void *hm, const void *key, size_t element_size, size_t key_offset, size_t key_size, int is_string);
extern size_t bfutils_hash_function(const void* key, size_t key_size);
extern long bfutils_hash_get_position(void *hm, const void *key, size_t element_size, size_t key_offset, size_t key_size, int is_string);
extern long bfutils_hash_remove_key(void *hm, const void *key, size_t element_size, size_t key_offset, size_t key_size, int is_string);
extern void bfutils_hash_free_f(void *hm);

#endif // HASH_H
#ifdef BFUTILS_HASH_IMPLEMENTATION

void *bfutils_hash_resize(void *hm, size_t element_size, size_t key_offset, size_t key_size, int is_string) {
    if (bfutils_hash_length(hm) == 0 || bfutils_hash_insert_count(hm) / (double) bfutils_hash_length(hm) > 0.5) {
        size_t old_length = bfutils_hash_length(hm);
        size_t length = bfutils_hash_length(hm) > 0 ? bfutils_hash_length(hm) * 2 : 128;
        unsigned char *slots = bfutils_hash_slots(hm);
        unsigned char *removed = bfutils_hash_removed(hm);
        unsigned char *old_data = NULL;
        if (old_length > 0) {
            old_data = BFUTILS_HASH_MALLOC(element_size * old_length);
            memcpy(old_data, hm, element_size * old_length);
        }

        BFUtilsHashHeader *header = BFUTILS_HASH_REALLOC(bfutils_hash_header(hm), sizeof(BFUtilsHashHeader) + element_size * length);
        header->slots = BFUTILS_HASH_CALLOC(1, length / 8 + 1);
        header->removed = BFUTILS_HASH_CALLOC(1, length / 8 + 1);
        header->length = length;
        header->insert_count = 0;
        hm = (void*) (header + 1);

        if (old_length > 0) {
            for(int i = 0; i <= old_length / 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int is_slot_occupied = slots[i] & (1 << j);
                    int is_removed = removed[i] & (1 << j);
                    if (is_slot_occupied && !is_removed) {
                        void * key = old_data + ((i*8+j) * element_size) + key_offset;
                        size_t pos = bfutils_hash_insert_position(hm, key, element_size, key_offset, key_size, is_string);
                        void *element = (unsigned char*)hm + (pos * element_size);
                        void *source = old_data + ((i*8+j) * element_size);
                        memcpy(element, source, element_size);
                    }
                }
            }
        }

        if(slots) {
            BFUTILS_HASH_FREE(slots);
        }
        if(old_data) {
            BFUTILS_HASH_FREE(old_data);
        }
        if(removed) {
            BFUTILS_HASH_FREE(removed);
        }
    }
    return hm;
}

int keycmp(const void *keya, const void *keyb, size_t key_size, int is_string) {
    if (is_string) {
        return strcmp(keya, *((char**) keyb));
    }
    return memcmp(keya, keyb, key_size);
}

size_t bfutils_hash_insert_position(void *hm, const void *key, size_t element_size, size_t key_offset, size_t key_size, int is_string) {
    size_t hash = is_string ? bfutils_hash_function(key, strlen(key)) : bfutils_hash_function(key, key_size);
    size_t index = hash % bfutils_hash_length(hm);
    size_t slot_index = index % 8;
    size_t slot_array_index = index / 8;

    int is_slot_occupied = bfutils_hash_slots(hm)[slot_array_index] & (1 << slot_index);
    int found_removed_slot = 0;
    size_t removed_slot_index = 0;
    while (is_slot_occupied) {
        int is_slot_removed = bfutils_hash_removed(hm)[slot_array_index] & (1 << slot_index);
        if (is_slot_removed && !found_removed_slot) {
            found_removed_slot = 1;
            removed_slot_index = slot_array_index * 8 + slot_index;
            continue;
        }
        void *src = (unsigned char*) hm + (index * element_size) + key_offset;
        if (0 == keycmp(key, src, key_size, is_string)) {
            return slot_array_index * 8 + slot_index;
        }

        slot_index++;
        if (slot_index == 8){
            slot_index = 0;
            slot_array_index = (slot_array_index + 1) % bfutils_hash_length(hm);
        }
        is_slot_occupied = bfutils_hash_slots(hm)[slot_array_index] & (1 << slot_index);
    }
    if (found_removed_slot) {
        bfutils_hash_removed(hm)[removed_slot_index / 8] &= ~(1 << removed_slot_index % 8);
        bfutils_hash_slots(hm)[removed_slot_index / 8] |= (1 << (removed_slot_index % 8));
    }
    else {
        bfutils_hash_slots(hm)[slot_array_index] |= (1 << slot_index);
    }
    bfutils_hash_header(hm)->insert_count++;
    return slot_array_index * 8 + slot_index;
}

long bfutils_hash_get_position(void *hm, const void *key, size_t element_size, size_t key_offset, size_t key_size, int is_string) {
    size_t hash = is_string ? bfutils_hash_function(key, strlen(key)) : bfutils_hash_function(key, key_size);
    size_t index = hash % bfutils_hash_length(hm);
    size_t slot_index = index % 8;
    size_t slot_array_index = index / 8;

    int is_slot_occupied = bfutils_hash_slots(hm)[slot_array_index] & (1 << slot_index);
    while (is_slot_occupied) {
        int is_slot_removed = bfutils_hash_removed(hm)[slot_array_index] & (1 << slot_index);
        if (!is_slot_removed) {
            void *src = (unsigned char*) hm + (index * element_size) + key_offset;
            if (0 == keycmp(key, src, key_size, is_string)) {
                return index;
            }
        }

        index++;
        slot_index++;
        if (slot_index == 8){
            slot_index = 0;
            slot_array_index = (slot_array_index + 1) % bfutils_hash_length(hm);
        }
        is_slot_occupied = bfutils_hash_slots(hm)[slot_array_index] & (1 << slot_index);
    }
    return -1;
}

void bfutils_hash_free_f(void *hm) {
    BFUTILS_HASH_FREE(bfutils_hash_header(hm)->slots);
    BFUTILS_HASH_FREE(bfutils_hash_header(hm)->removed);
    BFUTILS_HASH_FREE(bfutils_hash_header(hm));
}

long bfutils_hash_remove_key(void *hm, const void *key, size_t element_size, size_t key_offset, size_t key_size, int is_string) {
    long index = bfutils_hash_get_position(hm, key, element_size, key_offset, key_size, is_string);
    if (index > 0) {
        size_t slot_index = index % 8;
        size_t slot_array_index = index / 8;
        bfutils_hash_removed(hm)[slot_array_index] |= (1 << slot_index);
        bfutils_hash_header(hm)->length--;
    }
    return index;
}

size_t bfutils_hash_function(const void* key, size_t key_size) { //SDBM hash function
    unsigned char *str = (unsigned char *) key;
    size_t hash = 0;

    for (size_t i = 0; i < key_size; ++str, ++i) {
        hash = (*str) + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}
#endif //BFUTILS_HASH_IMPLEMENTATION
