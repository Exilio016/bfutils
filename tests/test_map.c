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

#include "test_framework.h"

#include <stdlib.h>

/* ------------------------------------------------------------------ */
/* bfu_map_init_                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    bool in_use;
    u64 key;
    u8 item;
} u64_u8_entry;

typedef struct {
    bool in_use;
    char *key;
    u8 item;
} str_u8_entry;

static void case_init_and_header(void) {
    bfu_map(u64, u8) map = NULL;

    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);
    TEST_ASSERT(map != NULL);

    bfu_map_header *header = ((bfu_map_header *) map) - 1;
    TEST_ASSERT(header->size == 0);
    TEST_ASSERT(header->capacity == 128);
    TEST_ASSERT(header->key_size == sizeof(u64));
    TEST_ASSERT(header->key_offset == offsetof(u64_u8_entry, key));
    TEST_ASSERT(header->item_size == sizeof(u8));
    TEST_ASSERT(header->item_offset == offsetof(u64_u8_entry, item));
    TEST_ASSERT(header->struct_size == sizeof(u64_u8_entry));
    TEST_ASSERT(header->free_key == false);

    u64_u8_entry *entries = (u64_u8_entry *) map;
    for (u64 i = 0; i < header->capacity; i++) {
        TEST_ASSERT(entries[i].in_use == false);
    }

    bfu_map_delete(map);
}

static void case_init_str_map(void) {
    bfu_map(char *, u8) map = NULL;

    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    bfu_map_header *header = ((bfu_map_header *) map) - 1;
    TEST_ASSERT(header->key_size == sizeof(char *));
    TEST_ASSERT(header->key_offset == offsetof(str_u8_entry, key));
    TEST_ASSERT(header->item_size == sizeof(u8));
    TEST_ASSERT(header->item_offset == offsetof(str_u8_entry, item));
    TEST_ASSERT(header->struct_size == sizeof(str_u8_entry));
    TEST_ASSERT(header->capacity == 128);
    TEST_ASSERT(header->free_key == true);

    bfu_map_delete(map);
}

static void case_init_null(void) {
    TEST_ASSERT(bfu_map_init_(NULL, sizeof(u64), 8, sizeof(u8), 16, 24, false)
                == BFU_ERROR_NULLPTR);
}

static void case_init_oom(void) {
    void *map = NULL;

    test_fail_next_malloc();
    TEST_ASSERT(bfu_map_init_(&map, sizeof(u64), 8, sizeof(u8), 16, 24, false)
                == BFU_ERROR_OOM);
}

/* ------------------------------------------------------------------ */
/* Lookup                                                             */
/* ------------------------------------------------------------------ */

static void case_has_item_missing(void) {
    bfu_map(char *, u8) map = NULL;
    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    TEST_ASSERT(bfu_map_has_item(map, "test") == false);

    bfu_map_delete(map);
}

static void case_get_missing(void) {
    bfu_map(char *, u8) map = NULL;
    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    u8 item = 0;
    TEST_ASSERT(bfu_map_get(item, map, "missing") == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(item == 0);

    bfu_map_delete(map);
}

/* ------------------------------------------------------------------ */
/* Insert / get                                                       */
/* ------------------------------------------------------------------ */

static void case_insert_and_get(void) {
    bfu_map(char *, u8) map = NULL;
    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    char *key = "foo";
    TEST_ASSERT(bfu_map_insert(map, key, 1) == BFU_SUCCESS);
    key = "bar";
    TEST_ASSERT(bfu_map_insert(map, key, 2) == BFU_SUCCESS);
    key = "baz";
    TEST_ASSERT(bfu_map_insert(map, key, 4) == BFU_SUCCESS);

    key = "foo";
    TEST_ASSERT(bfu_map_has_item(map, key) == true);
    key = "bar";
    TEST_ASSERT(bfu_map_has_item(map, key) == true);
    key = "baz";
    TEST_ASSERT(bfu_map_has_item(map, key) == true);
    TEST_ASSERT(bfu_map_has_item(map, "qux") == false);

    u8 item = 0;
    key = "foo";
    TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
    TEST_ASSERT(item == 1);
    key = "bar";
    TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
    TEST_ASSERT(item == 2);
    key = "baz";
    TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
    TEST_ASSERT(item == 4);

    bfu_map_delete(map);
}

static void case_insert_overwrites(void) {
    bfu_map(char *, u8) map = NULL;
    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    char *key = "key";
    TEST_ASSERT(bfu_map_insert(map, key, 1) == BFU_SUCCESS);
    TEST_ASSERT(bfu_map_insert(map, key, 9) == BFU_SUCCESS);

    u8 item = 0;
    TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
    TEST_ASSERT(item == 9);

    bfu_map_delete(map);
}

static void case_insert_many(void) {
    bfu_map(char *, u64) map = NULL;
    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    char keys[200][16];

    /* Cross the 0.75 load factor so the map has to grow. */
    for (u64 i = 0; i < 200; i++) {
        snprintf(keys[i], sizeof keys[i], "key%03llu", (unsigned long long) i);
        char *key = keys[i];
        TEST_ASSERT(bfu_map_insert(map, key, i * 2) == BFU_SUCCESS);
    }

    char *first = keys[0];
    char *last = keys[199];
    TEST_ASSERT(bfu_map_has_item(map, first) == true);
    TEST_ASSERT(bfu_map_has_item(map, last) == true);

    u64 item = 0;
    char *middle = keys[100];
    TEST_ASSERT(bfu_map_get(item, map, middle) == BFU_SUCCESS);
    TEST_ASSERT(item == 200);

    bfu_map_delete(map);
}

/* ------------------------------------------------------------------ */
/* Non-string keys                                                    */
/* ------------------------------------------------------------------ */

static void case_uint_insert_get(void) {
    bfu_map(u64, u64) map = NULL;
    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    u64 key = 7;
    u64 value = 70;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);

    key = 8;
    value = 80;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);

    key = 7;
    TEST_ASSERT(bfu_map_has_item(map, key) == true);
    u64 item = 0;
    TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
    TEST_ASSERT(item == 70);

    key = 8;
    TEST_ASSERT(bfu_map_has_item(map, key) == true);
    TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
    TEST_ASSERT(item == 80);

    /* Absent key: the probe walks into an unused slot and reports not found. */
    key = 9;
    TEST_ASSERT(bfu_map_has_item(map, key) == false);
    item = 0;
    TEST_ASSERT(bfu_map_get(item, map, key) == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(item == 0);

    /* Inserting an existing key updates the stored item. */
    key = 7;
    value = 71;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
    TEST_ASSERT(item == 71);

    bfu_map_delete(map);
}

static void case_uint_grow(void) {
    bfu_map(u64, u64) map = NULL;
    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    /* Cross the 0.75 load factor several times. */
    for (u64 i = 1; i <= 300; i++) {
        u64 key = i;
        u64 value = i * 3;
        TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    }

    bfu_map_header *header = ((bfu_map_header *) map) - 1;
    TEST_ASSERT(header->size == 300);
    TEST_ASSERT(header->capacity >= 300);

    for (u64 i = 1; i <= 300; i++) {
        u64 key = i;
        TEST_ASSERT(bfu_map_has_item(map, key) == true);

        u64 item = 0;
        TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
        TEST_ASSERT(item == i * 3);
    }

    u64 missing = 100000;
    TEST_ASSERT(bfu_map_has_item(map, missing) == false);

    bfu_map_delete(map);
}

static void case_position_helpers(void) {
    bfu_map(u64, u8) map = NULL;
    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    u64 key = 42;
    u8 value = 5;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);

    TEST_ASSERT(bfu_map_has_item(map, key) == true);
    TEST_ASSERT(bfu_map_item_position(map, key) >= 0);
    TEST_ASSERT(bfu_map_insert_item_position(map, key)
                == bfu_map_item_position(map, key));

    u64 other = 43;
    TEST_ASSERT(bfu_map_has_item(map, other) == false);
    TEST_ASSERT(bfu_map_item_position(map, other) == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(bfu_map_insert_item_position(map, other) >= 0);

    bfu_map_delete(map);
}

/* ------------------------------------------------------------------ */
/* Remove                                                             */
/* ------------------------------------------------------------------ */

static void case_uint_remove(void) {
    bfu_map(u64, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    for (u64 i = 1; i <= 3; i++) {
        u64 key = i;
        u64 value = i * 10;
        TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    }

    bfu_map_header *header = ((bfu_map_header *) map) - 1;
    TEST_ASSERT(header->size == 3);

    /* Removing a present key returns its item and shrinks the map. */
    u64 key = 2;
    u64 removed = 0;
    TEST_ASSERT(bfu_map_remove(removed, map, key) == BFU_SUCCESS);
    TEST_ASSERT(removed == 20);
    TEST_ASSERT(header->size == 2);
    TEST_ASSERT(bfu_map_has_item(map, key) == false);

    removed = 0;
    TEST_ASSERT(bfu_map_get(removed, map, key) == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(removed == 0);

    /* The other keys are untouched. */
    key = 1;
    TEST_ASSERT(bfu_map_get(removed, map, key) == BFU_SUCCESS);
    TEST_ASSERT(removed == 10);
    key = 3;
    TEST_ASSERT(bfu_map_get(removed, map, key) == BFU_SUCCESS);
    TEST_ASSERT(removed == 30);

    /* Removing an absent key leaves the output untouched. */
    key = 99;
    removed = 123;
    TEST_ASSERT(bfu_map_remove(removed, map, key) == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(removed == 123);

    /* A removed key can be inserted again. */
    key = 2;
    u64 value = 200;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    TEST_ASSERT(header->size == 3);
    TEST_ASSERT(bfu_map_get(removed, map, key) == BFU_SUCCESS);
    TEST_ASSERT(removed == 200);

    bfu_map_delete(map);
}

static void case_uint_remove_rehashes_chain(void) {
    enum { COUNT = 300 };

    bfu_map(u64, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    for (u64 i = 1; i <= COUNT; i++) {
        u64 key = i;
        u64 value = i * 10;
        TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    }

    bfu_map_header *header = ((bfu_map_header *) map) - 1;
    TEST_ASSERT(header->size == COUNT);

    /* Removing entries from probe chains forces the backward-shift path. */
    u64 removed_count = 0;
    for (u64 i = 3; i <= COUNT; i += 3) {
        u64 key = i;
        u64 removed = 0;
        TEST_ASSERT(bfu_map_remove(removed, map, key) == BFU_SUCCESS);
        TEST_ASSERT(removed == i * 10);
        removed_count++;
    }
    TEST_ASSERT(header->size == COUNT - removed_count);

    for (u64 i = 1; i <= COUNT; i++) {
        u64 key = i;
        u64 item = 0;
        if (i % 3 == 0) {
            TEST_ASSERT(bfu_map_has_item(map, key) == false);
            TEST_ASSERT(bfu_map_get(item, map, key) == BFU_ERROR_NOT_FOUND);
            TEST_ASSERT(item == 0);
        } else {
            TEST_ASSERT(bfu_map_has_item(map, key) == true);
            TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
            TEST_ASSERT(item == i * 10);
        }
    }

    /* Re-inserting the removed keys must keep every existing key reachable. */
    for (u64 i = 3; i <= COUNT; i += 3) {
        u64 key = i;
        u64 value = i * 10 + 1;
        TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
        u64 item = 0;
        TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
        TEST_ASSERT(item == value);
    }
    TEST_ASSERT(header->size == COUNT);

    bfu_map_delete(map);
}

static void case_str_remove(void) {
    bfu_map(char *, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    u64 value = 1;
    char *key = "alpha";
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    key = "beta";
    value = 2;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    key = "gamma";
    value = 3;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    key = "delta";
    value = 4;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);

    bfu_map_header *header = ((bfu_map_header *) map) - 1;
    TEST_ASSERT(header->size == 4);

    u64 removed = 0;
    key = "beta";
    test_reset_free_count();
    TEST_ASSERT(bfu_map_remove(removed, map, key) == BFU_SUCCESS);
    TEST_ASSERT(removed == 2);
    TEST_ASSERT(test_free_count() == 1); /* the stored copy of the key */
    TEST_ASSERT(header->size == 3);
    TEST_ASSERT(bfu_map_has_item(map, key) == false);

    removed = 0;
    TEST_ASSERT(bfu_map_get(removed, map, key) == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(removed == 0);

    key = "alpha";
    TEST_ASSERT(bfu_map_get(removed, map, key) == BFU_SUCCESS);
    TEST_ASSERT(removed == 1);
    key = "gamma";
    TEST_ASSERT(bfu_map_get(removed, map, key) == BFU_SUCCESS);
    TEST_ASSERT(removed == 3);
    key = "delta";
    TEST_ASSERT(bfu_map_get(removed, map, key) == BFU_SUCCESS);
    TEST_ASSERT(removed == 4);

    /* Removing an absent key leaves the output untouched. */
    removed = 7;
    key = "missing";
    TEST_ASSERT(bfu_map_remove(removed, map, key) == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(removed == 7);

    /* A removed key can be inserted again. */
    key = "beta";
    value = 22;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    TEST_ASSERT(bfu_map_get(removed, map, key) == BFU_SUCCESS);
    TEST_ASSERT(removed == 22);
    TEST_ASSERT(header->size == 4);

    bfu_map_delete(map);
}

static void case_str_remove_rehashes_chain(void) {
    enum { COUNT = 200 };

    bfu_map(char *, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    char keys[COUNT][16];
    for (u64 i = 0; i < COUNT; i++) {
        snprintf(keys[i], sizeof keys[i], "key%03llu", (unsigned long long) i);
        char *key = keys[i];
        TEST_ASSERT(bfu_map_insert(map, key, i) == BFU_SUCCESS);
    }

    bfu_map_header *header = ((bfu_map_header *) map) - 1;
    TEST_ASSERT(header->size == COUNT);

    u64 removed_count = 0;
    for (u64 i = 0; i < COUNT; i += 2) {
        char *key = keys[i];
        u64 removed = 0;
        TEST_ASSERT(bfu_map_remove(removed, map, key) == BFU_SUCCESS);
        TEST_ASSERT(removed == i);
        removed_count++;
    }
    TEST_ASSERT(header->size == COUNT - removed_count);

    for (u64 i = 0; i < COUNT; i++) {
        char *key = keys[i];
        u64 item = 0;
        if (i % 2 == 0) {
            TEST_ASSERT(bfu_map_has_item(map, key) == false);
            TEST_ASSERT(bfu_map_get(item, map, key) == BFU_ERROR_NOT_FOUND);
            TEST_ASSERT(item == 0);
        } else {
            TEST_ASSERT(bfu_map_has_item(map, key) == true);
            TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
            TEST_ASSERT(item == i);
        }
    }

    for (u64 i = 0; i < COUNT; i += 2) {
        char *key = keys[i];
        u64 value = i + 1000;
        TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
        u64 item = 0;
        TEST_ASSERT(bfu_map_get(item, map, key) == BFU_SUCCESS);
        TEST_ASSERT(item == value);
    }
    TEST_ASSERT(header->size == COUNT);

    bfu_map_delete(map);
}

static void case_remove_missing(void) {
    bfu_map(u64, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    u64 key = 5;
    u64 item = 42;
    TEST_ASSERT(bfu_map_remove(item, map, key) == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(item == 42);

    bfu_map_delete(map);

    bfu_map(char *, u64) str_map = NULL;
    TEST_ASSERT(bfu_map_init(str_map) == BFU_SUCCESS);

    char *str_key = "missing";
    u64 str_item = 7;
    TEST_ASSERT(bfu_map_remove(str_item, str_map, str_key) == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(str_item == 7);

    bfu_map_delete(str_map);
}

static void case_remove_null_arguments(void) {
    u64 key = 1;
    u64 item = 0;
    void *map = NULL;

    TEST_ASSERT(bfu_map_remove_(NULL, NULL, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_remove_(NULL, (void **) &map, &key) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_remove_(&item, NULL, &key) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_remove_(&item, (void **) &map, NULL) == BFU_ERROR_NULLPTR);

    char *str_key = "k";
    TEST_ASSERT(bfu_map_str_remove(NULL, NULL, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_str_remove(NULL, (void **) &map, str_key) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_str_remove(&item, NULL, str_key) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_str_remove(&item, (void **) &map, NULL) == BFU_ERROR_NULLPTR);
}

/* ------------------------------------------------------------------ */
/* Delete                                                             */
/* ------------------------------------------------------------------ */

static void case_delete_non_string_map(void) {
    bfu_map(u64, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    for (u64 i = 0; i < 10; i++) {
        u64 key = i;
        u64 value = i * 2;
        TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    }

    test_reset_free_count();
    bfu_map_delete(map);
    TEST_ASSERT(map == NULL);
    TEST_ASSERT(test_free_count() == 1); /* inline keys are not owned */
}

static void case_delete_frees_string_keys(void) {
    bfu_map(char *, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    u64 value = 1;
    char *key = "one";
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    key = "two";
    value = 2;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    key = "three";
    value = 3;
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);

    test_reset_free_count();
    bfu_map_delete(map);
    TEST_ASSERT(map == NULL);
    TEST_ASSERT(test_free_count() == 4); /* 3 duplicated keys + the map */
}

/* ------------------------------------------------------------------ */
/* Error paths                                                        */
/* ------------------------------------------------------------------ */

static void case_null_arguments(void) {
    bfu_map(u64, u8) map = NULL;
    bfu_result result = bfu_map_init(map);
    TEST_ASSERT(result == BFU_SUCCESS);

    u64 key = 1;
    u8 item = 0;

    /* Empty map: both entry points report "not found" without hashing. */
    TEST_ASSERT(bfu_map_has_item(map, key) == false);
    TEST_ASSERT(bfu_map_item_position(map, key) == BFU_ERROR_NOT_FOUND);

    TEST_ASSERT(bfu_map_has_item(NULL, key) == false);
    TEST_ASSERT(bfu_map_has_item_(map, NULL) == false);
    TEST_ASSERT(bfu_map_str_has_item(NULL, "k") == false);

    TEST_ASSERT(bfu_map_item_position(NULL, key) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_item_position_(map, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_str_item_position(NULL, "k") == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_str_item_position(map, NULL) == BFU_ERROR_NULLPTR);

    TEST_ASSERT(bfu_map_insert_item_position(NULL, key) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_insert_item_position_(map, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_str_insert_item_position(NULL, "k") == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_str_insert_item_position(map, NULL) == BFU_ERROR_NULLPTR);

    TEST_ASSERT(bfu_map_grow(NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_grow_str(NULL) == BFU_ERROR_NULLPTR);

    TEST_ASSERT(bfu_map_insert_(NULL, &key, &item) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_insert_((void **) &map, NULL, &item) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_insert_((void **) &map, &key, NULL) == BFU_ERROR_NULLPTR);

    TEST_ASSERT(bfu_map_str_insert(NULL, "k", &item) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_str_insert((void **) &map, NULL, &item) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_str_insert((void **) &map, "k", NULL) == BFU_ERROR_NULLPTR);

    bfu_map_delete(map);
    bfu_map_delete(map); // TEST with NULL value
    bfu_map_delete_ptr(NULL); // TEST with NULL value
}

static void case_get_from_null_map(void) {
    bfu_map(char *, u8) map = NULL;

    u8 item = 0;
    TEST_ASSERT(bfu_map_get(item, map, "k") == BFU_ERROR_NOT_FOUND);
    TEST_ASSERT(item == 0);
    TEST_ASSERT(bfu_map_has_item(map, "k") == false);
}

static void case_grow_oom(void) {
    /* The map only allocates while growing (load factor > 0.75).  Fill a map
       past that threshold so the next insert triggers a grow, then fail the
       grow's malloc and verify the insert reports OOM without inserting. */
    bfu_map(u64, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    enum { FILL = 97 }; /* 97 / 128 > 0.75 */
    for (u64 i = 0; i < FILL; i++) {
        TEST_ASSERT(bfu_map_insert(map, i, i) == BFU_SUCCESS);
    }

    u64 key = FILL;
    u64 value = FILL;
    test_fail_next_malloc();
    TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_ERROR_OOM);
    TEST_ASSERT(bfu_map_has_item(map, key) == false);

    bfu_map_delete(map);

    bfu_map(char *, u64) str_map = NULL;
    TEST_ASSERT(bfu_map_init(str_map) == BFU_SUCCESS);

    for (u64 i = 0; i < FILL; i++) {
        char key_buf[16];
        snprintf(key_buf, sizeof(key_buf), "%llu", (unsigned long long) i);
        TEST_ASSERT(bfu_map_insert(str_map, (char*) key_buf, i) == BFU_SUCCESS);
    }

    char *str_key = strdup("one");
    u64 str_value = 1;
    test_fail_next_malloc();
    TEST_ASSERT(bfu_map_insert(str_map, str_key, str_value) == BFU_ERROR_OOM);
    TEST_ASSERT(bfu_map_has_item(str_map, str_key) == false);
    /* The insert failed before taking ownership of str_key, so free it here. */
    free(str_key);

    bfu_map_delete(str_map);
}

/* ------------------------------------------------------------------ */
/* Next / prev / foreach                                              */
/* ------------------------------------------------------------------ */

static void case_map_remove_wraparound(void) {
    /* Fill the table densely so a removal near the end makes the backward-shift
       scan wrap past index 0, hitting the cur >= pos path of needs_to_shift().
       Reachability of remaining keys is verified by case_uint_remove_rehashes_chain;
       this case only needs to run the wraparound scan itself. */
    bfu_map(u64, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    enum { COUNT = 768 };
    u64 keys[COUNT];
    for (u64 i = 0; i < COUNT; i++) {
        keys[i] = i;
        TEST_ASSERT(bfu_map_insert(map, keys[i], i * 2) == BFU_SUCCESS);
    }

    bfu_map_header *header = ((bfu_map_header *) map) - 1;
    TEST_ASSERT(header->size == COUNT);
    i64 cap = (i64) header->capacity;

    /* Try to remove a key from the last slot so the scan wraps to 0
       immediately (cur = cap-1, pos = 0). */
    u64 last_key = 0;
    bool found_last = false;
    for (u64 i = 0; i < COUNT; i++) {
        i64 p = bfu_map_item_position(map, keys[i]);
        if (p == cap - 1) {
            last_key = keys[i];
            found_last = true;
            break;
        }
    }

    if (found_last) {
        u64 removed = 0;
        TEST_ASSERT(bfu_map_remove(removed, map, last_key) == BFU_SUCCESS);
        TEST_ASSERT(removed == last_key * 2);
        TEST_ASSERT(header->size == COUNT - 1);
        TEST_ASSERT(bfu_map_has_item(map, last_key) == false);
    }

    /* Remove every remaining key; each removal succeeds and decrements size. */
    u64 remaining = found_last ? COUNT - 1 : COUNT;
    for (u64 i = 0; i < COUNT; i++) {
        if (found_last && keys[i] == last_key) continue;
        u64 removed = 0;
        TEST_ASSERT(bfu_map_remove(removed, map, keys[i]) == BFU_SUCCESS);
        TEST_ASSERT(removed == keys[i] * 2);
        remaining--;
        TEST_ASSERT(header->size == remaining);
    }

    TEST_ASSERT(header->size == 0);
    bfu_map_delete(map);
}

static void case_map_next_prev(void) {
    bfu_map(u64, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    u64 keys[] = {10, 20, 30, 40, 50};
    for (u64 i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        u64 key = keys[i];
        u64 value = key * 2;
        TEST_ASSERT(bfu_map_insert(map, key, value) == BFU_SUCCESS);
    }

    /* next() from the empty state returns the first used slot. */
    i64 idx = bfu_map_next(map, 0);
    TEST_ASSERT(idx >= 0);
    TEST_ASSERT(idx < (i64) ((bfu_map_header *) map - 1)->capacity);

    /* Walk forward through all inserted entries. */
    u64 found = 0;
    idx = bfu_map_next(map, 0);
    u64 guard = 0;
    i64 cap = (i64) ((bfu_map_header *) map - 1)->capacity;
    while (idx >= 0 && idx < cap && guard <= (u64) cap) {
        guard++;
        u64 key = ((bfu_map(u64, u64)) map)[idx].key;
        if (key > 0) found++;
        i64 next_idx = bfu_map_next(map, idx);
        if (next_idx == idx || next_idx < 0 || next_idx >= cap) break;
        idx = next_idx;
    }
    TEST_ASSERT(found == sizeof(keys) / sizeof(keys[0]));

    /* prev() from the last slot returns the previous used slot. */
    i64 prev_idx = bfu_map_prev(map, idx);
    TEST_ASSERT(prev_idx >= 0);

    /* next() maps out-of-range back to the given index. */
    cap = (i64) ((bfu_map_header *) map - 1)->capacity;
    TEST_ASSERT(bfu_map_next(map, cap) == cap);

    /* prev() maps before the first slot back to 0. */
    TEST_ASSERT(bfu_map_prev(map, 0) == 0);

    /* next() and prev() on NULL map report NULLPTR. */
    TEST_ASSERT(bfu_map_next(NULL, 0) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_map_prev(NULL, 0) == BFU_ERROR_NULLPTR);

    /* Next past the end and prev before zero report out of bounds. */
    TEST_ASSERT(bfu_map_next(map, cap + 1) == BFU_ERROR_OUT_OF_BOUND);
    TEST_ASSERT(bfu_map_prev(map, -1) == BFU_ERROR_OUT_OF_BOUND);

    bfu_map_delete(map);
}

static void case_map_foreach(void) {
    bfu_map(char *, u64) map = NULL;
    TEST_ASSERT(bfu_map_init(map) == BFU_SUCCESS);

    u64 values[] = {5, 10, 15, 20};
    const char *names[] = {"one", "two", "three", "four"};
    for (u64 i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        char *key = (char *) names[i];
        TEST_ASSERT(bfu_map_insert(map, key, values[i]) == BFU_SUCCESS);
    }

    u64 sum = 0;
    u64 count = 0;
    i64 idx = 0;
    i64 cap = (i64) ((bfu_map_header *) map - 1)->capacity;
    u64 guard = 0;
    while (idx < cap && guard <= (u64) cap) {
        guard++;
        u64 item = map[idx].item;
        if (item > 0) {
            sum += item;
            count++;
        }
        i64 next_idx = bfu_map_next(map, idx);
        if (next_idx == idx || next_idx < 0 || next_idx >= cap) break;
        idx = next_idx;
    }
    TEST_ASSERT(count == sizeof(values) / sizeof(values[0]));
    TEST_ASSERT(sum == 50);

    bfu_map_delete(map);
}

/* ------------------------------------------------------------------ */

void test_map_suite(void) {
    printf("\n[bfu_map]\n");

    TEST_RUN(case_init_and_header);
    TEST_RUN(case_init_str_map);
    TEST_RUN(case_init_null);
    TEST_RUN(case_init_oom);
    TEST_RUN(case_has_item_missing);
    TEST_RUN(case_get_missing);
    TEST_RUN(case_insert_and_get);
    TEST_RUN(case_insert_overwrites);
    TEST_RUN(case_insert_many);
    TEST_RUN(case_uint_insert_get);
    TEST_RUN(case_uint_grow);
    TEST_RUN(case_position_helpers);
    TEST_RUN(case_remove_missing);
    TEST_RUN(case_remove_null_arguments);
    TEST_RUN(case_uint_remove);
    TEST_RUN(case_uint_remove_rehashes_chain);
    TEST_RUN(case_delete_non_string_map);
    TEST_RUN(case_delete_frees_string_keys);
    TEST_RUN(case_null_arguments);
    TEST_RUN(case_get_from_null_map);
    TEST_RUN(case_grow_oom);
    TEST_RUN(case_str_remove);
    TEST_RUN(case_str_remove_rehashes_chain);
    TEST_RUN(case_map_remove_wraparound);
    TEST_RUN(case_map_next_prev);
    TEST_RUN(case_map_foreach);
}
