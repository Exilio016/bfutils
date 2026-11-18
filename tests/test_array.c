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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Exercise the cleanup-based array type as well. */
DEFINE_BFU_ARRAY_DELETE(i32)

/* ------------------------------------------------------------------ */
/* bfu_array_init / bfu_array_delete                                  */
/* ------------------------------------------------------------------ */

static void case_init_and_header(void) {
    i32 *arr = NULL;

    TEST_ASSERT(bfu_array_init((void **) &arr, sizeof(i32), 4) == BFU_SUCCESS);
    TEST_ASSERT(arr != NULL);
    TEST_ASSERT(bfu_array_size(arr) == 0);
    TEST_ASSERT(bfu_array_capacity(arr) == 4);

    bfu_array_header *header = bfu_array_get_header(arr);
    TEST_ASSERT(header != NULL);
    TEST_ASSERT(header->size == 0);
    TEST_ASSERT(header->capacity == 4);
    TEST_ASSERT(header->item_size == sizeof(i32));

    bfu_array_delete(arr);
    TEST_ASSERT(arr == NULL);
}

static void case_init_null(void) {
    TEST_ASSERT(bfu_array_init(NULL, sizeof(i32), 4) == BFU_ERROR_NULLPTR);
}

static void case_init_oom(void) {
    void *arr = (void *) 0x1;

    test_fail_next_malloc();
    TEST_ASSERT(bfu_array_init(&arr, sizeof(i32), 4) == BFU_ERROR_OOM);
    TEST_ASSERT(arr == NULL);
}

static void case_delete_null(void) {
    bfu_array_delete_ptr(NULL);

    void *arr = NULL;
    bfu_array_delete_ptr(&arr);
    TEST_ASSERT(arr == NULL);
}

static void case_queries_null(void) {
    TEST_ASSERT(bfu_array_size(NULL) == 0);
    TEST_ASSERT(bfu_array_capacity(NULL) == 0);
    TEST_ASSERT(bfu_array_get_header(NULL) == NULL);
}

/* ------------------------------------------------------------------ */
/* Push / pop / remove                                                */
/* ------------------------------------------------------------------ */

static void case_push_pop_remove(void) {
    i32 *arr = NULL;
    i32 out = 0;

    TEST_ASSERT(bfu_array_push(arr, 10) == BFU_SUCCESS);
    TEST_ASSERT(arr != NULL);
    TEST_ASSERT(bfu_array_size(arr) == 1);
    TEST_ASSERT(bfu_array_capacity(arr) == 64);
    TEST_ASSERT(arr[0] == 10);

    TEST_ASSERT(bfu_array_push(arr, 20) == BFU_SUCCESS);
    TEST_ASSERT(bfu_array_push(arr, 30) == BFU_SUCCESS);
    TEST_ASSERT(bfu_array_size(arr) == 3);
    TEST_ASSERT(arr[1] == 20);
    TEST_ASSERT(arr[2] == 30);

    TEST_ASSERT(bfu_array_pop(out, arr) == BFU_SUCCESS);
    TEST_ASSERT(out == 30);
    TEST_ASSERT(bfu_array_size(arr) == 2);

    TEST_ASSERT(bfu_array_remove_unordered(out, arr, 0) == BFU_SUCCESS);
    TEST_ASSERT(out == 10);
    TEST_ASSERT(bfu_array_size(arr) == 1);
    TEST_ASSERT(arr[0] == 20);

    TEST_ASSERT(bfu_array_remove_unordered(out, arr, 1) == BFU_ERROR_OUT_OF_BOUND);
    TEST_ASSERT(bfu_array_remove_unordered(out, arr, 100) == BFU_ERROR_OUT_OF_BOUND);

    bfu_array_delete(arr);
    TEST_ASSERT(arr == NULL);

    TEST_ASSERT(bfu_array_pop(out, arr) == BFU_ERROR_OUT_OF_BOUND);
    TEST_ASSERT(bfu_array_remove_unordered(out, arr, 0) == BFU_ERROR_OUT_OF_BOUND);
}

static void case_push_grows(void) {
    i32 *arr = NULL;

    for (i32 i = 0; i < 300; i++) {
        TEST_ASSERT(bfu_array_push(arr, i) == BFU_SUCCESS);
    }

    TEST_ASSERT(bfu_array_size(arr) == 300);
    TEST_ASSERT(bfu_array_capacity(arr) >= 300);

    for (i32 i = 0; i < 300; i++) {
        TEST_ASSERT(arr[i] == i);
    }

    bfu_array_delete(arr);
}

static void case_push_init_oom(void) {
    i32 *arr = NULL;

    test_fail_next_malloc();
    TEST_ASSERT(bfu_array_push(arr, 7) == BFU_ERROR_OOM);
    TEST_ASSERT(arr == NULL);
}

/* ------------------------------------------------------------------ */
/* bfu_array_resize_if_required                                       */
/* ------------------------------------------------------------------ */

static void case_resize_paths(void) {
    void *arr = NULL;

    TEST_ASSERT(bfu_array_resize_if_required(NULL, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_array_resize_if_required(&arr, NULL) == BFU_ERROR_NULLPTR);

    TEST_ASSERT(bfu_array_init(&arr, sizeof(i32), 2) == BFU_SUCCESS);

    /* Below capacity: no reallocation. */
    bfu_array_header *header = bfu_array_get_header(arr);
    header->size = 1;
    TEST_ASSERT(bfu_array_resize_if_required(&arr, header) == BFU_SUCCESS);
    TEST_ASSERT(bfu_array_capacity(arr) == 2);

    /* At capacity: capacity doubles. */
    header = bfu_array_get_header(arr);
    header->size = header->capacity;
    TEST_ASSERT(bfu_array_resize_if_required(&arr, header) == BFU_SUCCESS);
    TEST_ASSERT(bfu_array_capacity(arr) == 4);

    /* Forced realloc failure reports OOM. */
    header = bfu_array_get_header(arr);
    header->size = header->capacity;
    test_fail_next_realloc();
    TEST_ASSERT(bfu_array_resize_if_required(&arr, header) == BFU_ERROR_OOM);
    TEST_ASSERT(arr == NULL);
    free(header);
}

/* ------------------------------------------------------------------ */
/* bfu_array_dup                                                      */
/* ------------------------------------------------------------------ */

static void case_dup_null(void) {
    i32 *arr = NULL;
    i32 *copy = NULL;

    TEST_ASSERT(bfu_array_dup(NULL, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_array_dup((void **) &copy, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_array_dup(NULL, arr) == BFU_ERROR_NULLPTR);
}

static void case_dup(void) {
    i32 *arr = NULL;
    i32 *copy = NULL;

    TEST_ASSERT(bfu_array_push(arr, 1) == BFU_SUCCESS);
    TEST_ASSERT(bfu_array_push(arr, 2) == BFU_SUCCESS);
    TEST_ASSERT(bfu_array_push(arr, 3) == BFU_SUCCESS);

    TEST_ASSERT(bfu_array_dup((void **) &copy, arr) == BFU_SUCCESS);
    TEST_ASSERT(copy != NULL);
    TEST_ASSERT(copy != arr);
    TEST_ASSERT(bfu_array_size(copy) == 3);
    TEST_ASSERT(bfu_array_capacity(copy) == bfu_array_capacity(arr));
    TEST_ASSERT(copy[0] == 1 && copy[1] == 2 && copy[2] == 3);

    /* The copy must not alias the source. */
    copy[0] = 99;
    TEST_ASSERT(arr[0] == 1);

    bfu_array_delete(copy);
    bfu_array_delete(arr);
}

static void case_dup_oom(void) {
    i32 *arr = NULL;
    i32 *copy = NULL;

    TEST_ASSERT(bfu_array_push(arr, 1) == BFU_SUCCESS);

    test_fail_next_malloc();
    TEST_ASSERT(bfu_array_dup((void **) &copy, arr) == BFU_ERROR_OOM);
    TEST_ASSERT(copy == NULL);

    bfu_array_delete(arr);
}

/* ------------------------------------------------------------------ */
/* Cleanup attribute                                                   */
/* ------------------------------------------------------------------ */

static void scoped_array(void) {
    bfu_array_f(i32) scoped = NULL;

    TEST_ASSERT(bfu_array_push(scoped, 5) == BFU_SUCCESS);
    TEST_ASSERT(bfu_array_push(scoped, 6) == BFU_SUCCESS);
    TEST_ASSERT(scoped[0] == 5);
    TEST_ASSERT(scoped[1] == 6);
}

static void case_cleanup_attribute(void) {
    scoped_array();
    TEST_ASSERT(true);
}

/* ------------------------------------------------------------------ */

void test_array_suite(void) {
    printf("\n[bfu_array]\n");

    TEST_RUN(case_init_and_header);
    TEST_RUN(case_init_null);
    TEST_RUN(case_init_oom);
    TEST_RUN(case_delete_null);
    TEST_RUN(case_queries_null);
    TEST_RUN(case_push_pop_remove);
    TEST_RUN(case_push_grows);
    TEST_RUN(case_push_init_oom);
    TEST_RUN(case_resize_paths);
    TEST_RUN(case_dup_null);
    TEST_RUN(case_dup_oom);
    TEST_RUN(case_cleanup_attribute);
    TEST_RUN(case_dup);
}
