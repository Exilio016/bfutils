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

#include "bfu_string.h"
#include "bfu_common.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bfu_string_delete(bfu_string *string) {
    if (string == NULL) return;

    if (string->data != NULL)
        free(string->data);
    string->data = NULL;
    string->size = 0;
    string->capacity = 0;
}

bfu_result bfu_string_push_cstr_sized(bfu_string *string, const char *content, u64 size) {
    if (string == NULL || content == NULL) 
        return BFU_ERROR_NULLPTR;

    for (u64 i = 0; i < size; i++) {
        bfu_da_append_safe(string, content[i]);
    }
    bfu_da_append(string, '\0');
    string->size--;
    return BFU_SUCCESS;
}

bfu_result bfu_string_push_cstr(bfu_string *string, const char *content) {
    if (string == NULL || content == NULL) 
        return BFU_ERROR_NULLPTR;
    return bfu_string_push_cstr_sized(string, content, strlen(content));
}

bfu_result bfu_string_push_string(bfu_string *string, bfu_string *content) {
    if (string == NULL || content == NULL) 
        return BFU_ERROR_NULLPTR;
    return bfu_string_push_cstr_sized(string, content->data, content->size);
}

void bfu_string_println(bfu_string *string) {
    printf("%.*s\n", (i32) string->size, string->data);
}

bfu_result bfu_string_dup_string(bfu_string *dst, bfu_string *src) {
    if (dst == NULL || src == NULL) {
        return BFU_ERROR_NULLPTR;
    }

    dst->size = 0;
    return bfu_string_push_string(dst, src);
}

bfu_result bfu_string_dup_cstr(bfu_string *dst, const char *src) {
    if (dst == NULL || src == NULL) {
        return BFU_ERROR_NULLPTR;
    }

    dst->size = 0;
    return bfu_string_push_cstr(dst, src);
}

bfu_result bfu_string_dup_cstr_sized(bfu_string *dst, const char *src, u64 size) {
    if (dst == NULL || src == NULL) {
        return BFU_ERROR_NULLPTR;
    }

    dst->size = 0;
    return bfu_string_push_cstr_sized(dst, src, size);
}

bool bfu_string_eq(bfu_string *left, bfu_string *right) {
    if (left == NULL || right == NULL) {
        return false;
    }

    if (left->size != right->size) return false;

    return strncmp(left->data, right->data, left->size) == 0;
}

i32 bfu_string_cmp(bfu_string *left, bfu_string *right) {
    if (left == NULL || right == NULL) {
        return BFU_ERROR_NULLPTR;
    }

    i32 res = strncmp(left->data, right->data, BFU_MIN(left->size, right->size));
    if (left->size != right->size && res == 0) {
        res = left->size - right->size;
    }
    return res;
}

bfu_result bfu_string_format(bfu_string *string, const char *format, ...) {
    if (string == NULL || format == NULL) return BFU_ERROR_NULLPTR;
    string->size = 0;

    va_list ap;
    va_start(ap, format);
    i32 n = vsnprintf(string->data, string->capacity, format, ap);
    va_end(ap);

    if (n < 0) {
        return BFU_ERROR_OVERFLOW;
    }

    if ((u64) n >= string->capacity) {
        string->capacity = n + 1;
        string->data = realloc(string->data, sizeof(char) * string->capacity);
        va_start(ap, format);
        n = vsnprintf(string->data, string->capacity, format, ap);
        va_end(ap);

        if (n < 0) {
            return BFU_ERROR_OVERFLOW;
        }
    }

    string->size = n;
    return BFU_SUCCESS;
}

bfu_result bfu_string_push_format(bfu_string *string, const char *format, ...) {
    if (string == NULL || format == NULL) return BFU_ERROR_NULLPTR;

    va_list ap;
    va_start(ap, format);
    i32 n = vsnprintf(string->data + string->size, string->capacity - string->size, format, ap);
    va_end(ap);

    if (n < 0) {
        return BFU_ERROR_OVERFLOW;
    }

    if ((u64) n >= (string->capacity - string->size)) {
        string->capacity = string->size + n + 1;
        string->data = realloc(string->data, sizeof(char) * string->capacity);
        va_start(ap, format);
        n = vsnprintf(string->data + string->size, string->capacity - string->size, format, ap);
        va_end(ap);

        if (n < 0) {
            return BFU_ERROR_OVERFLOW;
        }
    }

    string->size += n;
    return BFU_SUCCESS;
}

bfu_array(bfu_string) bfu_string_split(bfu_string *string, char delim) {
    bfu_array(bfu_string) list = NULL;

    bfu_string buf = {0};
    for (u64 i = 0; i < string->size; i++) {
        if (string->data[i] == delim) {
            bfu_da_append(&buf, '\0');
            buf.size--;
            bfu_array_push(list, buf);
            buf = bfu_string_new();
        }
        else {
            bfu_da_append(&buf, string->data[i]);
        }
    }
    bfu_da_append(&buf, '\0');
    buf.size--;
    bfu_array_push(list, buf);

    return list;
}
