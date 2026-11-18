/*
 * Copyright (c) 2026 Bruno Flávio Ferreira <him@brunoflaviof.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "bfu_map.h"
#include "rapidhash.h"
#include <stdlib.h>

bfu_result bfu_map_init_(void **map, u64 key_size, u64 key_offset,
        u64 item_size, u64 item_offset, u64 struct_size, bool free_key){
    if (map == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    bfu_map_header *header = malloc(sizeof(bfu_map_header) + struct_size * 128);
    if (header == NULL) {
        return BFU_ERROR_OOM;
    }
    header->item_size = item_size;
    header->item_offset = item_offset;
    header->key_size = key_size;
    header->key_offset = key_offset;
    header->struct_size = struct_size;
    header->size = 0;
    header->capacity = 128;
    header->free_key = free_key;

    *map = header + 1;
    memset(*map, 0, header->capacity * header->struct_size);

    return BFU_SUCCESS;
}

bool bfu_map_has_item_(void *map, void *key) {
    i64 pos =  bfu_map_item_position_(map, key);
    return pos >= 0;
}

bool bfu_map_str_has_item(void *map, const char *key) {
    i64 pos = bfu_map_str_item_position(map, key);
    return pos >= 0;
}

static inline bfu_map_header *bfu_map_get_header(void *map) {
    return ((bfu_map_header *)map) - 1;
}

static inline u64 bfu_map_get_byte_pos(bfu_map_header *header, u64 pos) {
    u64 res = (header->struct_size * pos);
    return res;
}

i64 bfu_map_item_position_(void *map, void *key) {
    if (map == NULL || key == NULL) return BFU_ERROR_NULLPTR;
    bfu_map_header *header = bfu_map_get_header(map);
    if (header->size == 0) return BFU_ERROR_NOT_FOUND;
    u64 hash = rapidhash(key, header->key_size);
    u64 pos = hash % header->capacity;
    while(true) {
        bool in_use = *((bool*) (map + bfu_map_get_byte_pos(header, pos)));
        if (!in_use) return BFU_ERROR_NOT_FOUND;
        void *k = (void*) (map + bfu_map_get_byte_pos(header, pos) + header->key_offset);
        if (memcmp(key, k, header->key_size) == 0) return pos;
        pos = (pos + 1) % header->capacity;
    }
}

i64 bfu_map_str_item_position(void *map, const char *key) {
    if (map == NULL || key == NULL) return BFU_ERROR_NULLPTR;
    bfu_map_header *header = bfu_map_get_header(map);
    if (header->size == 0) return BFU_ERROR_NOT_FOUND;
    u64 hash = rapidhash(key, strlen(key));
    u64 pos = hash % header->capacity;
    while(true) {
        bool in_use = *((bool*) (map + bfu_map_get_byte_pos(header, pos)));
        if (!in_use) return BFU_ERROR_NOT_FOUND;
        char **k = (char**) (map + bfu_map_get_byte_pos(header, pos) + header->key_offset);
        if (strcmp(key, *k) == 0) return pos;
        pos = (pos + 1) % header->capacity;
    }
}

bfu_result bfu_map_grow(void **map) {
    if (map == NULL) return BFU_ERROR_NULLPTR;
    bfu_map_header *header = bfu_map_get_header(*map);
    f64 alpha = (double) header->size / (double) header->capacity;
    void *elements = malloc(header->struct_size * header->size);
    if (elements == NULL) {
        return BFU_ERROR_OOM;
    }

    if (alpha > 0.75) {
        int j = 0;
        u8 *bytes = *map;
        for (u64 i = 0; i < header->capacity; i++) {
            bool in_use = (bool) bytes[bfu_map_get_byte_pos(header, i)];
            if (in_use) {
                memcpy(elements + (header->struct_size * j), 
                       bytes + bfu_map_get_byte_pos(header, i), header->struct_size);
                j++;
            }
        }
        header->capacity *= 2;
        header = realloc(header, sizeof(bfu_map_header) + (header->struct_size * header->capacity));
        *map = header + 1;
        memset(*map, 0, header->capacity * header->struct_size);
        for (u64 i = 0; i < header->size; i++) {
            void *key = elements + bfu_map_get_byte_pos(header, i) + header->key_offset;
            i64 pos = bfu_map_insert_item_position_(*map, key);
            memcpy((*map) + bfu_map_get_byte_pos(header, pos), elements + bfu_map_get_byte_pos(header, i), header->struct_size);
        }
        free(elements);
    }
    return BFU_SUCCESS;
}

bfu_result bfu_map_grow_str(void **map) {
    if (map == NULL) return BFU_ERROR_NULLPTR;
    bfu_map_header *header = bfu_map_get_header(*map);
    f64 alpha = (double) header->size / (double) header->capacity;
    void *elements = malloc(header->struct_size * header->size);
    if (elements == NULL) {
        return BFU_ERROR_OOM;
    }

    if (alpha > 0.75) {
        int j = 0;
        u8 *bytes = *map;
        for (u64 i = 0; i < header->capacity; i++) {
            bool in_use = (bool) bytes[bfu_map_get_byte_pos(header, i)];
            if (in_use) {
                memcpy(elements + (header->struct_size * j), 
                       bytes + bfu_map_get_byte_pos(header, i), header->struct_size);
                j++;
            }
        }
        header->capacity *= 2;
        header = realloc(header, sizeof(bfu_map_header) + (header->struct_size * header->capacity));
        *map = header + 1;
        memset(*map, 0, header->capacity * header->struct_size);
        for (u64 i = 0; i < header->size; i++) {
            char **key = elements + bfu_map_get_byte_pos(header, i) + header->key_offset;
            i64 pos = bfu_map_insert_item_position(*map, *key);
            memcpy((*map) + bfu_map_get_byte_pos(header, pos), elements + bfu_map_get_byte_pos(header, i), header->struct_size);
        }
        free(elements);
    }
    return BFU_SUCCESS;
}


i64 bfu_map_insert_item_position_(void *map, void *key) {
    if (map == NULL || key == NULL) return BFU_ERROR_NULLPTR;
    bfu_map_header *header = bfu_map_get_header(map);
    u64 hash = rapidhash(key, header->key_size);
    u64 pos = hash % header->capacity;
    u8 *bytes = (u8*) map;
    while(true) {
        bool in_use = (bool) bytes[bfu_map_get_byte_pos(header, pos)];
        if (!in_use) return pos;
        void *k = (void*) (bytes + bfu_map_get_byte_pos(header, pos) + header->key_offset);
        if (memcmp(key, k, header->key_size) == 0) return pos;
        pos = (pos + 1) % header->capacity;
    }
}

i64 bfu_map_str_insert_item_position(void *map, char *key) {
    if (map == NULL || key == NULL) return BFU_ERROR_NULLPTR;
    bfu_map_header *header = bfu_map_get_header(map);
    u64 hash = rapidhash(key, strlen(key));
    u64 pos = hash % header->capacity;
    u8 *bytes = (u8*) map;
    while(true) {
        bool in_use = (bool) bytes[bfu_map_get_byte_pos(header, pos)];
        if (!in_use) return pos;
        char **k = (char**) (bytes + bfu_map_get_byte_pos(header, pos) + header->key_offset);
        if (strcmp(key, *k) == 0) return pos;
        pos = (pos + 1) % header->capacity;
    }
}


bfu_result bfu_map_str_insert(void **map, char *key, void *item) {
    if (map == NULL || key == NULL || item == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    bfu_result err = bfu_map_grow_str(map);
    if (err) return err;

    i64 pos = bfu_map_insert_item_position(*map, key);
    if (pos < 0) {
        return pos;
    }

    bfu_map_header *header = bfu_map_get_header(*map);
    header->size++;
    bool *in_use = (*map) + bfu_map_get_byte_pos(header, pos);
    *in_use = true;
    char **key_addr = (*map) + bfu_map_get_byte_pos(header, pos) + header->key_offset;
    *key_addr = strdup(key);

    memcpy((*map) + bfu_map_get_byte_pos(header, pos) + header->item_offset,
           item, header->item_size);
    return BFU_SUCCESS;
}

bfu_result bfu_map_insert_(void **map, void *key, void *item) {
    if (map == NULL || key == NULL || item == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    bfu_result err = bfu_map_grow(map);
    if (err) return err;

    i64 pos = bfu_map_insert_item_position_(*map, key);
    if (pos < 0) {
        return pos;
    }

    bfu_map_header *header = bfu_map_get_header(*map);
    header->size++;
    bool *in_use = (*map) + bfu_map_get_byte_pos(header, pos);
    *in_use = true;

    memcpy((*map) + bfu_map_get_byte_pos(header, pos) + header->key_offset,
           key, header->key_size);
    memcpy((*map) + bfu_map_get_byte_pos(header, pos) + header->item_offset,
           item, header->item_size);
    return BFU_SUCCESS;
}

void bfu_map_delete_ptr(void **map) {
    if (map == NULL || *map == NULL) {
        return;
    }
    bfu_map_header *header = bfu_map_get_header(*map);
    u8 *bytes = (u8*) *map;
    if (header->free_key) {
        for (u64 i = 0; i < header->capacity; i++) {
            bool in_use = (bool) bytes[bfu_map_get_byte_pos(header, i)];
            if (in_use) {
                char **key_addr = (*map) + bfu_map_get_byte_pos(header, i) + header->key_offset;
                free(*key_addr);
            }
        }
    }
    free(header);
    *map = NULL;
}

static inline bool needs_to_shift(u64 home, u64 cur, u64 pos) {
    if (cur < pos) {
        return home <= cur || home > pos;
    }
    return home <= cur && home > pos;
}

bfu_result bfu_map_remove_(void *item, void **map, void *key) {
    if (map == NULL || item == NULL || key == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    u8 *bytes = (u8*) *map;
    bfu_map_header *header = bfu_map_get_header(*map);
    i64 pos = bfu_map_item_position_(*map, key);
    if (pos < 0) {
        return pos;
    }
    bytes[bfu_map_get_byte_pos(header, pos)] = false;
    memcpy(item, (*map) + bfu_map_get_byte_pos(header, pos) + header->item_offset, header->item_size);
    header->size--;
    
    u64 cur = pos;
    pos = (pos + 1) % header->capacity;
    while (bytes[bfu_map_get_byte_pos(header, pos)]) {
        void *item_key = (*map) + bfu_map_get_byte_pos(header, pos) + header->key_offset;
        u64 home = (rapidhash(item_key, header->key_size) % header->capacity);
        if (needs_to_shift(home, cur, pos)) {
            memcpy((*map) + bfu_map_get_byte_pos(header, cur), (*map) + bfu_map_get_byte_pos(header, pos), header->struct_size);
            bytes[bfu_map_get_byte_pos(header, pos)] = false;
        
            cur = pos;
        }
        pos = (pos + 1) % header->capacity;
    }
    return BFU_SUCCESS;
}

bfu_result bfu_map_str_remove(void *item, void **map, char *key) {
    if (map == NULL || item == NULL || key == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    u8 *bytes = (u8*) *map;
    bfu_map_header *header = bfu_map_get_header(*map);
    i64 pos = bfu_map_str_item_position(*map, key);
    if (pos < 0) {
        return pos;
    }
    bytes[bfu_map_get_byte_pos(header, pos)] = false;
    memcpy(item, (*map) + bfu_map_get_byte_pos(header, pos) + header->item_offset, header->item_size);
    char **key_addr = (*map) + bfu_map_get_byte_pos(header, pos) + header->key_offset;
    free(*key_addr);
    header->size--;

    u64 cur = pos;
    pos = (pos + 1) % header->capacity;
    while (bytes[bfu_map_get_byte_pos(header, pos)]) {
        char **item_key = (*map) + bfu_map_get_byte_pos(header, pos) + header->key_offset;
        u64 home = (rapidhash(*item_key, strlen(*item_key)) % header->capacity);
        if (needs_to_shift(home, cur, pos)) {
            memcpy((*map) + bfu_map_get_byte_pos(header, cur), (*map) + bfu_map_get_byte_pos(header, pos), header->struct_size);
            bytes[bfu_map_get_byte_pos(header, pos)] = false;
        
            cur = pos;
        }
        pos = (pos + 1) % header->capacity;
    }
    return BFU_SUCCESS;
}

i64 bfu_map_next(void *map, i64 current) {
    if (map == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    
    u8 *bytes = (u8*) map;
    bfu_map_header *header = bfu_map_get_header(map);
    if (header->capacity < (u64) current) {
        return BFU_ERROR_OUT_OF_BOUND;
    }
    for (u64 i = (u64) current + 1; i < header->capacity; i++) {
        if (bytes[bfu_map_get_byte_pos(header, i)]) {
            return i;
        }
    }
    return current;
}

i64 bfu_map_prev(void *map, i64 current) {
    if (map == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    
    u8 *bytes = (u8*) map;
    bfu_map_header *header = bfu_map_get_header(map);
    if (current < 0) {
        return BFU_ERROR_OUT_OF_BOUND;
    }
    for (i64 i = current - 1; i >= 0; i--) {
        if (bytes[bfu_map_get_byte_pos(header, i)]) {
            return i;
        }
    }
    return current;
}
