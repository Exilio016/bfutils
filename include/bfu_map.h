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

#pragma once

#include "bfu_types.h"
#include <string.h>
#include <stddef.h>

typedef struct {
    u64 key_size;
    u64 key_offset;
    u64 item_size;
    u64 item_offset;
    u64 struct_size;
    u64 size;
    u64 capacity;
    bool free_key;
} bfu_map_header;

/* Evaluates to a map type that associates `key_type` keys with `item_type`
 * items.  The result is a pointer to an anonymous struct containing an
 * `in_use` flag, a `key`, and an `item`.
 *
 * Example:
 *     bfu_map(int, double) my_map = NULL;
 *
 * @param key_type  Type of the map keys.
 * @param item_type Type of the map items.
 */
#define bfu_map(key_type, item_type) struct { \
    bool in_use; \
    key_type key; \
    item_type item; \
} *

/* Evaluates to a map type with `char *` (string) keys and `type` items.
 *
 * Example:
 *     bfu_map_str(int) my_map = NULL;
 *
 * @param item_type Type of the map items.
 */
#define bfu_map_str(type) bfu_map(char *, type)

bfu_result bfu_map_init_(void **map, u64 key_size, u64 key_offset, u64 item_size, u64 item_offset, u64 struct_size, bool free_key);

/* Initializes `map` with default settings, automatically deducing key and item
 * sizes from the map's type.  String-keyed maps (`bfu_map_str`) have their
 * keys freed automatically on deletion.
 *
 * The expression evaluates to a `bfu_result` (BFU_SUCCESS on success, or a
 * negative error code on failure).
 *
 * @param map  Map variable to initialize.
 * @return     BFU_SUCCESS on success, or a negative error code on failure.
 */
#define bfu_map_init(map) bfu_map_init_((void**) &(map), \
    sizeof((map)->key), (void*) &((map)->key) - (void*) (map), \
    sizeof((map)->item), (void*) &((map)->item) - (void*)(map), sizeof(*(map)), \
    _Generic((map)->key, char*: true, default: false))

bfu_result bfu_map_grow(void **map);
bfu_result bfu_map_grow_str(void **map);

bool bfu_map_has_item_(void *map, void *key);
bool bfu_map_str_has_item(void *map, const char *key);

/* Returns true if `key` is present in `map`.
 *
 * Dispatches to the string or generic implementation based on the key type.
 *
 * @param map  Map to query. Must not be NULL.
 * @param key  Key to look up.
 * @return     true if the key exists, false otherwise.
 */
#define bfu_map_has_item(map, key) _Generic((key), \
    char *: bfu_map_str_has_item, \
    default: bfu_map_has_item_ \
)((map), _Generic((key), char *: (key), default: &(key)))

i64 bfu_map_item_position_(void *map, void *key);
i64 bfu_map_str_item_position(void *map, const char *key);

/* Returns the index of `key` in `map`, or a negative error code if not found.
 *
 * Dispatches to the string or generic implementation based on the key type.
 *
 * @param map  Map to query. Must not be NULL.
 * @param key  Key to look up.
 * @return     Index of the key, or a negative error code if not found.
 */
#define bfu_map_item_position(map, key) _Generic((key), \
    char *: bfu_map_str_item_position, \
    default: bfu_map_item_position_\
)((map), _Generic((key), char *: (key), default: &(key)))


i64 bfu_map_insert_item_position_(void *map, void *key);
i64 bfu_map_str_insert_item_position(void *map, char *key);

/* Returns the index where `key` should be inserted in `map`, or the index of
 * the existing key if it is already present.
 *
 * Dispatches to the string or generic implementation based on the key type.
 *
 * @param map  Map to query. Must not be NULL.
 * @param key  Key to find the insert position for.
 * @return     Insert position index, or a negative error code on failure.
 */
#define bfu_map_insert_item_position(map, key) _Generic((key), \
    char *: bfu_map_str_insert_item_position, \
    default: bfu_map_insert_item_position_\
)((map), _Generic((key), char *: (key), default: &(key)))


/* Retrieves the item associated with `key` from `map` and stores it in `ret`.
 *
 * The expression evaluates to a `bfu_result` (BFU_SUCCESS on success, or
 * BFU_ERROR_NOT_FOUND if the key is not present).
 *
 * @param ret  Variable to receive the item.
 * @param map  Map to query. Must not be NULL.
 * @param key  Key to look up.
 * @return     BFU_SUCCESS on success, BFU_ERROR_NOT_FOUND if the key is absent.
 */
#define bfu_map_get(ret, map, key) ({ \
    bfu_result _aux = BFU_SUCCESS; \
    typeof(key) _aux_key = (key); \
    i64 _pos = bfu_map_item_position(map, _aux_key); \
    if (_pos < 0) { \
        _aux = BFU_ERROR_NOT_FOUND; \
    } else { \
        (ret) = (map)[_pos].item; \
    } \
    _aux; \
})

bfu_result bfu_map_insert_(void **map, void *key, void *item);
bfu_result bfu_map_str_insert(void **map, char *key, void *item);

/* Inserts or updates `item` at `key` in `map`, growing the map as needed.
 *
 * Dispatches to the string or generic implementation based on the key type.
 * The expression evaluates to a `bfu_result` (BFU_SUCCESS on success, or a
 * negative error code on failure).  On success `map` is updated in place.
 *
 * @param map  Map variable to insert into. Must not be NULL.
 * @param key  Key to insert or update.
 * @param item  Item to insert.
 * @return     BFU_SUCCESS on success, or a negative error code on failure.
 */
#define bfu_map_insert(map, key, item) ({ \
    typeof(item) _aux = (item); \
    typeof(key) _aux_key = (key); \
    _Generic((key), \
        char *: bfu_map_str_insert, \
        default: bfu_map_insert_\
    )((void**)&(map), _Generic(_aux_key, char *: _aux_key, default: &_aux_key), &_aux); \
})

/* Frees the memory owned by `map` and sets it to NULL.
 *
 * If the map was created with string keys, the keys are freed as well.
 * Typically used through the `bfu_map_delete(map)` macro.
 *
 * @param map  Pointer to the map variable. May be NULL.
 */
void bfu_map_delete_ptr(void **map);

/* Frees the memory owned by `map` and sets it to NULL.
 *
 * If the map was created with string keys, the keys are freed as well.
 *
 * @param map  Map variable to free. May be NULL.
 */
#define bfu_map_delete(map) bfu_map_delete_ptr((void**)&(map));

/* Internal helpers used by the `bfu_map_remove` macro. */
bfu_result bfu_map_remove_(void *item, void **map, void *key);
bfu_result bfu_map_str_remove(void *item, void **map, char *key);

/* Removes `key` from `map` and stores the removed item in `ret`.
 *
 * Dispatches to the string or generic implementation based on the key type.
 * The expression evaluates to a `bfu_result` (BFU_SUCCESS on success, or
 * BFU_ERROR_NOT_FOUND if the key is absent).
 *
 * @param ret  Variable to receive the removed item.
 * @param map  Map variable to remove from. Must not be NULL.
 * @param key  Key to remove.
 * @return     BFU_SUCCESS on success, BFU_ERROR_NOT_FOUND if the key is absent.
 */
#define bfu_map_remove(ret, map, key) ({ \
    typeof(key) _aux_key = (key); \
    _Generic((key), \
        char *: bfu_map_str_remove, \
        default: bfu_map_remove_\
    )(&(ret), (void**)&(map), _Generic(_aux_key, char *: _aux_key, default: &_aux_key)); \
})

/* Advances `current` to the index of the next in-use slot in `map`.
 *
 * Used to iterate over the entries of a map (see the `bfu_map_foreach` macro).
 *
 * @param map     Map to iterate. Must not be NULL.
 * @param current Index of the current slot, or -1 to start from the beginning.
 * @return        Index of the next in-use slot, or `current` if there are no
 *                more entries. BFU_ERROR_NULLPTR if `map` is NULL.
 */
i64 bfu_map_next(void *map, i64 current);

/* Moves `current` to the index of the previous in-use slot in `map`.
 *
 * @param map     Map to iterate. Must not be NULL.
 * @param current Index of the current slot.
 * @return        Index of the previous in-use slot, or `current` if there are
 *                no earlier entries. BFU_ERROR_OUT_OF_BOUND if `current` < 0,
 *                BFU_ERROR_NULLPTR if `map` is NULL.
 */
i64 bfu_map_prev(void *map, i64 current);

/* Iterates over all in-use entries of `map`, executing the loop body once per
 * entry with `idx` set to the entry's index.
 *
 * Example:
 *     bfu_map_foreach(my_map, i) {
 *         // access my_map[i].key and my_map[i].item
 *     }
 *
 * @param map  Map to iterate. Must not be NULL.
 * @param idx  Loop variable receiving each in-use index.
 */
#define bfu_map_foreach(map, idx) for (i64 idx = bfu_map_next((map), -1); idx < (i64) ((bfu_map_header*)(map) - 1)->capacity; idx = bfu_map_next((map), idx)) 
