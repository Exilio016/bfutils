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
#include "bfu_common.h"

typedef struct {
    u64 size;
    u64 capacity;
    u64 item_size;
} bfu_array_header;

#define DEFINE_BFU_ARRAY_DELETE(type) \
    static inline void bfu_array_##type##_delete(type **ptr) { \
        bfu_array_delete_ptr((void **)ptr); \
    }

/* Declares a variable of type `type*` whose destructor automatically frees the
 * array when it goes out of scope.
 *
 * Example:
 *     bfu_array_f(int) arr = NULL;
 *
 * @param type  Element type of the array.
 */
#define bfu_array_f(type) __attribute__((cleanup(bfu_array_##type##_delete))) type*

/* Evaluates to a pointer of type `type*`.  Useful as a type alias when
 * declaring array variables.
 *
 * Example:
 *     bfu_array(int) arr = NULL;
 *
 * @param type  Element type of the array.
 */
#define bfu_array(type) type*

/* Returns the number of items currently stored in `arr`.
 *
 * @param arr  Array to query. May be NULL, in which case 0 is returned.
 * @return     Number of items in the array.
 */
u64 bfu_array_size(void *arr);

/* Returns the allocated capacity of `arr` (number of items it can hold
 * without reallocating).
 *
 * @param arr  Array to query. May be NULL, in which case 0 is returned.
 * @return     Allocated capacity of the array, in items.
 */
u64 bfu_array_capacity(void *arr);

/* Internal helpers used by the array macros. */
bfu_array_header *bfu_array_get_header(void *arr);
bfu_result bfu_array_resize_if_required(void **arr, bfu_array_header *header);

/* Initializes a dynamic array backed by a heap-allocated header.
 *
 * @param arr       Pointer to the array variable to initialize. On failure it
 *                  is set to NULL.
 * @param item_size Size of a single item, in bytes.
 * @param capacity  Initial capacity, in items.
 * @return          BFU_SUCCESS on success, BFU_ERROR_NULLPTR if `arr` is NULL,
 *                  or BFU_ERROR_OOM on allocation failure.
 */
bfu_result bfu_array_init(void **arr, u64 item_size, u64 capacity);

/* Duplicates `arr` into a newly allocated array stored at `*dst`.
 *
 * @param dst  Pointer to the destination array variable.
 * @param arr  Array to duplicate. Must not be NULL.
 * @return     BFU_SUCCESS on success, BFU_ERROR_NULLPTR if either argument is
 *             NULL, or BFU_ERROR_OOM on allocation failure.
 */
bfu_result bfu_array_dup(void **dst, void *arr);

/* Evaluates to a null array pointer, useful as an initializer.
 *
 * Example:
 *     bfu_array(int) arr = bfu_array_new();
 */
#define bfu_array_new() NULL;

/* Frees the memory owned by `array` and sets it to NULL.
 *
 * Typically used through the `bfu_array_delete(arr)` macro.
 *
 * @param array  Pointer to the array variable. May be NULL.
 */
void bfu_array_delete_ptr(void **array);

/* Frees the memory owned by `arr` and sets it to NULL.
 *
 * @param arr  Array variable to free. May be NULL.
 */
#define bfu_array_delete(arr) bfu_array_delete_ptr((void**)&(arr))

/* Appends `item` to the end of `arr`, growing the array as needed.
 *
 * If `arr` is NULL it is initialized automatically with an initial capacity of
 * 64 items.  The expression evaluates to a `bfu_result` (BFU_SUCCESS on
 * success, or a negative error code on failure).  On success `arr` is updated
 * in place to point at the (possibly reallocated) array.
 *
 * @param arr  Array variable to append to. May be NULL.
 * @param item Item to append.
 * @return     BFU_SUCCESS on success, or a negative error code on failure.
 */
#define bfu_array_push(arr, item) ({ \
    bfu_result _res = BFU_SUCCESS; \
    if ((arr) == NULL) { \
        _res = bfu_array_init((void**)&(arr), sizeof(*(arr)), 64); \
    }\
    if (_res == 0) { \
        _res = bfu_array_resize_if_required((void**)&(arr), bfu_array_get_header((arr))); \
        (arr)[bfu_array_get_header((arr))->size++] = (item); \
    } \
    _res; \
})

/* Removes the last item from `arr` and stores it in `ret`.
 *
 * The expression evaluates to a `bfu_result` (BFU_SUCCESS on success, or
 * BFU_ERROR_OUT_OF_BOUND if the array is empty).  On success `arr`'s size is
 * decremented and `ret` receives the popped value.
 *
 * @param ret  Variable to receive the popped item.
 * @param arr  Array to pop from. Must not be NULL and must be non-empty.
 * @return     BFU_SUCCESS on success, BFU_ERROR_OUT_OF_BOUND if empty.
 */
#define bfu_array_pop(ret, arr) ({ \
    bfu_result _ret = BFU_SUCCESS; \
    if (bfu_array_size(arr) == 0) { \
        _ret = BFU_ERROR_OUT_OF_BOUND; \
    } \
    else { \
        (ret) = ((arr)[--bfu_array_get_header((arr))->size]); \
    } \
    _ret; \
})

/* Removes the item at index `idx` from `arr` without preserving order, and
 * stores the removed item in `ret`.
 *
 * The item at `idx` is overwritten with the last item in the array, then the
 * size is decremented.  This is O(1) but does not preserve the order of the
 * remaining elements.
 *
 * The expression evaluates to a `bfu_result` (BFU_SUCCESS on success, or
 * BFU_ERROR_OUT_OF_BOUND if `idx` is out of bounds).
 *
 * @param ret  Variable to receive the removed item.
 * @param arr  Array to remove from. Must not be NULL.
 * @param idx  Index of the item to remove.
 * @return     BFU_SUCCESS on success, BFU_ERROR_OUT_OF_BOUND if `idx` >= size.
 */
#define bfu_array_remove_unordered(ret, arr, idx) ({ \
    bfu_result _aux = BFU_SUCCESS; \
    if (bfu_array_size(arr) <= idx) { \
        _aux = BFU_ERROR_OUT_OF_BOUND; \
    } else { \
        (ret) = (arr)[(idx)]; \
        (arr)[(idx)] = (arr)[--bfu_array_get_header(arr)->size]; \
    } \
    _aux; \
})
