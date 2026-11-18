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

#include "bfu_array.h"
#include <stdlib.h>
#include <string.h>

bfu_result bfu_array_init(void **arr, u64 item_size, u64 capacity) {
    if (arr == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    bfu_array_header *header = malloc(sizeof(bfu_array_header) + item_size * capacity);
    if (header == NULL) {
        *arr = NULL;
        return BFU_ERROR_OOM;
    }
    header->size = 0;
    header->capacity = capacity;
    header->item_size = item_size;
    *arr = header + 1;
    return BFU_SUCCESS;
}

void bfu_array_delete_ptr(void **array) {
    if (array != NULL && *array != NULL) {
        bfu_array_header *header = ((bfu_array_header *) *array) - 1;
        free(header);
        *array = NULL;
    }
}

bfu_result bfu_array_resize_if_required(void **arr, bfu_array_header *header){
    if (arr == NULL || header == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    
    if (header->size >= header->capacity) {
        header->capacity *= 2;
        header = realloc(header, sizeof(bfu_array_header) + (header->item_size * header->capacity));
        if (header == NULL) {
            *arr = NULL;
            return BFU_ERROR_OOM;
        }
    }
    *arr = header + 1;
    return BFU_SUCCESS;
}

u64 bfu_array_size(void *arr) {
    if (arr == NULL) return 0;
    bfu_array_header *header = ((bfu_array_header*) arr) - 1;
    return header->size;
}

u64 bfu_array_capacity(void *arr) {
    if (arr == NULL) return 0;
    bfu_array_header *header = ((bfu_array_header*) arr) - 1;
    return header->capacity;
}

bfu_array_header *bfu_array_get_header(void *arr) {
    if (arr == NULL) return NULL;
    bfu_array_header *header = ((bfu_array_header*) arr) - 1;
    return header;
}

bfu_result bfu_array_dup(void **dst, void *arr) {
    if (dst == NULL || arr == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    bfu_array_header *header = bfu_array_get_header(arr);
    void * copy;
    bfu_result err = bfu_array_init(&copy, header->item_size, header->capacity);
    if (err != BFU_SUCCESS) return err;
    bfu_array_header *copy_header = bfu_array_get_header(copy);

    copy_header->size = header->size;
    memcpy(copy, arr, header->item_size * header->size);
    *dst = copy;
    return BFU_SUCCESS;
}
