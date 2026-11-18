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
#include "bfu_array.h"

typedef struct {
    u64 size;
    u64 capacity;
    char *data;
} bfu_string;

/* Declares a `bfu_string` variable whose buffer is automatically freed when it
 * goes out of scope.
 *
 * Example:
 *     bfu_string_f s = bfu_string_new();
 */
#define bfu_string_f __attribute__((cleanup(bfu_string_delete))) bfu_string

/* Evaluates to a new, empty bfu_string.
 *
 * Example:
 *     bfu_string s = bfu_string_new();
 */
#define bfu_string_new() ((bfu_string){0})

/* Frees the memory owned by `string` and resets it to an empty state.
 *
 * Typically used through the `bfu_string_f` cleanup variable attribute.
 *
 * @param string  String to free. May be NULL.
 */
void bfu_string_delete(bfu_string *string);

/* Appends a null-terminated C string to `string`.
 *
 * @param string   String to append to. Must not be NULL.
 * @param content  Null-terminated C string to append. Must not be NULL.
 * @return         BFU_SUCCESS on success, BFU_ERROR_NULLPTR if either argument
 *                 is NULL, or BFU_ERROR_OOM on allocation failure.
 */
bfu_result bfu_string_push_cstr(bfu_string *string, const char *content);

/* Appends at most `size` bytes of `content` to `string`.
 *
 * @param string   String to append to. Must not be NULL.
 * @param content  Content to append. Must not be NULL.
 * @param size     Number of bytes to append from `content`.
 * @return         BFU_SUCCESS on success, BFU_ERROR_NULLPTR if either argument
 *                 is NULL, or BFU_ERROR_OOM on allocation failure.
 */
bfu_result bfu_string_push_cstr_sized(bfu_string *string, const char *content, u64 size);

/* Appends the contents of another string to `string`.
 *
 * @param string   String to append to. Must not be NULL.
 * @param content  String whose contents are appended. Must not be NULL.
 * @return         BFU_SUCCESS on success, BFU_ERROR_NULLPTR if either argument
 *                 is NULL, or BFU_ERROR_OOM on allocation failure.
 */
bfu_result bfu_string_push_string(bfu_string *string, bfu_string *content);

/* Appends `content` to `string`.
 *
 * Dispatches based on the type and number of arguments:
 *   - `bfu_string_push(string, cstr)` appends a null-terminated C string.
 *   - `bfu_string_push(string, cstr, size)` appends at most `size` bytes.
 *   - `bfu_string_push(string, bfu_string)` appends another string's contents.
 *
 * The expression evaluates to a `bfu_result` (BFU_SUCCESS on success, or a
 * negative error code on failure).
 *
 * @param string  String to append to. Must not be NULL.
 * @param ...     Content to append (see above for the accepted forms).
 * @return        BFU_SUCCESS on success, or a negative error code on failure.
 */
#define bfu_string_push_(string, content) _Generic((content), \
    char*: bfu_string_push_cstr, \
    bfu_string*: bfu_string_push_string \
)(string, content)
#define bfu_string_push(...) BFU_GET_4(__VA_ARGS__, bfu_string_push_cstr_sized, bfu_string_push_)(__VA_ARGS__)

/* Replaces the contents of `dst` with a copy of `src`.
 *
 * @param dst  Destination string. Must not be NULL.
 * @param src  Source string to copy. Must not be NULL.
 * @return     BFU_SUCCESS on success, BFU_ERROR_NULLPTR if either argument is
 *             NULL, or BFU_ERROR_OOM on allocation failure.
 */
bfu_result bfu_string_dup_string(bfu_string *dst, bfu_string *src);

/* Replaces the contents of `dst` with a copy of the null-terminated C string
 * `src`.
 *
 * @param dst  Destination string. Must not be NULL.
 * @param src  Null-terminated C string to copy. Must not be NULL.
 * @return     BFU_SUCCESS on success, BFU_ERROR_NULLPTR if either argument is
 *             NULL, or BFU_ERROR_OOM on allocation failure.
 */
bfu_result bfu_string_dup_cstr(bfu_string *dst, const char *src);

/* Replaces the contents of `dst` with a copy of the first `size` bytes of
 * `src`.
 *
 * @param dst   Destination string. Must not be NULL.
 * @param src   Content to copy. Must not be NULL.
 * @param size  Number of bytes to copy from `src`.
 * @return      BFU_SUCCESS on success, BFU_ERROR_NULLPTR if either argument is
 *              NULL, or BFU_ERROR_OOM on allocation failure.
 */
bfu_result bfu_string_dup_cstr_sized(bfu_string *dst, const char *src, u64 size);

#define bfu_string_dup_(dst, src) _Generic((src), \
    char*: bfu_string_dup_cstr, \
    bfu_string*: bfu_string_dup_string \
)(dst, src)

/* Replaces the contents of `dst` with a copy of `src`.
 *
 * Dispatches based on the type and number of arguments:
 *   - `bfu_string_dup(dst, cstr)` copies a null-terminated C string.
 *   - `bfu_string_dup(dst, cstr, size)` copies at most `size` bytes.
 *   - `bfu_string_dup(dst, bfu_string)` copies another string's contents.
 *
 * The expression evaluates to a `bfu_result` (BFU_SUCCESS on success, or a
 * negative error code on failure).
 *
 * @param dst  Destination string. Must not be NULL.
 * @param ...  Source to copy (see above for the accepted forms).
 * @return     BFU_SUCCESS on success, or a negative error code on failure.
 */
#define bfu_string_dup(...) BFU_GET_4(__VA_ARGS__, bfu_string_dup_cstr_sized, bfu_string_dup_)(__VA_ARGS__)

/* Compares two strings for equality.
 *
 * @param left  First string. Must not be NULL.
 * @param right Second string. Must not be NULL.
 * @return      true if both strings have equal contents, false otherwise.
 */
bool bfu_string_eq(bfu_string *left, bfu_string *right);

/* Compares two strings lexicographically.
 *
 * @param left  First string. Must not be NULL.
 * @param right Second string. Must not be NULL.
 * @return      Negative if left < right, zero if equal, positive if left >
 *              right. BFU_ERROR_NULLPTR if either argument is NULL.
 */
i32 bfu_string_cmp(bfu_string *left, bfu_string *right);

/* Prints the contents of `string` followed by a newline to stdout.
 *
 * @param string  String to print. May be NULL.
 */
void bfu_string_println(bfu_string *string);

/* Appends a formatted string to the end of `string`.
 *
 * The formatted output is written after the existing contents and `string->size`
 * is increased accordingly.
 *
 * @param string  String to append to. Must not be NULL.
 * @param format  printf-style format string. Must not be NULL.
 * @param ...     Arguments for the format string.
 * @return        BFU_SUCCESS on success, BFU_ERROR_NULLPTR if either argument
 *                is NULL, or BFU_ERROR_OVERFLOW on formatting failure.
 */
bfu_result bfu_string_push_format(bfu_string *string, const char *format, ...) __attribute__((format(printf, 2, 3)));

/* Formats `format` into `string`, replacing its current contents.
 *
 * @param string  String to write into. Must not be NULL.
 * @param format  printf-style format string. Must not be NULL.
 * @param ...     Arguments for the format string.
 * @return        BFU_SUCCESS on success, BFU_ERROR_NULLPTR if either argument
 *                is NULL, or BFU_ERROR_OVERFLOW on formatting failure.
 */
bfu_result bfu_string_format(bfu_string *string, const char *format, ...) __attribute__((format(printf, 2, 3)));

/* Splits `string` into an array of bfu_string at every occurrence of `delim`.
 *
 * @param string  String to split. Must not be NULL.
 * @param delim   Delimiter character.
 * @return        Newly allocated array of bfu_string (NULL on failure).
 */
bfu_array(bfu_string) bfu_string_split(bfu_string *string, char delim);
