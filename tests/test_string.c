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
#include <wchar.h>

DEFINE_BFU_ARRAY_DELETE(bfu_string)

/* ------------------------------------------------------------------ */
/* bfu_string_delete                                                  */
/* ------------------------------------------------------------------ */

static void case_delete(void) {
    bfu_string_delete(NULL);

    bfu_string string = bfu_string_new();
    TEST_ASSERT(bfu_string_push_cstr(&string, "abc") == BFU_SUCCESS);
    bfu_string_delete(&string);

    TEST_ASSERT(string.data == NULL);
    TEST_ASSERT(string.size == 0);
    TEST_ASSERT(string.capacity == 0);

    /* Deleting an already deleted string is a no-op. */
    bfu_string_delete(&string);
    TEST_ASSERT(string.data == NULL);

    bfu_string empty = bfu_string_new();
    bfu_string_delete(&empty);
}

/* ------------------------------------------------------------------ */
/* Push                                                               */
/* ------------------------------------------------------------------ */

static void case_push_null(void) {
    bfu_string string = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr(NULL, "x") == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_push_cstr(&string, NULL) == BFU_ERROR_NULLPTR);

    TEST_ASSERT(bfu_string_push_cstr_sized(NULL, "x", 1) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_push_cstr_sized(&string, NULL, 1) == BFU_ERROR_NULLPTR);

    TEST_ASSERT(bfu_string_push_string(NULL, &string) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_push_string(&string, NULL) == BFU_ERROR_NULLPTR);

    bfu_string_delete(&string);
}

static void case_push_cstr(void) {
    bfu_string string = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr(&string, "Hello") == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "Hello");
    TEST_ASSERT(string.capacity >= string.size + 1);

    TEST_ASSERT(bfu_string_push_cstr(&string, "") == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "Hello");

    TEST_ASSERT(bfu_string_push_cstr(&string, ", World!") == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "Hello, World!");
    TEST_ASSERT(string.data[string.size] == '\0');

    bfu_string_delete(&string);
}

static void case_push_cstr_sized(void) {
    bfu_string string = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr_sized(&string, "abcdef", 3) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "abc");

    /* Embedded NUL bytes are preserved and counted in size. */
    u64 base = string.size;
    TEST_ASSERT(bfu_string_push_cstr_sized(&string, "a\0b", 3) == BFU_SUCCESS);
    TEST_ASSERT(string.size == base + 3);
    TEST_ASSERT(memcmp(string.data + base, "a\0b", 3) == 0);
    TEST_ASSERT(string.data[string.size] == '\0');

    /* Zero sized append is allowed. */
    base = string.size;
    TEST_ASSERT(bfu_string_push_cstr_sized(&string, "ignored", 0) == BFU_SUCCESS);
    TEST_ASSERT(string.size == base);

    bfu_string_delete(&string);
}

static void case_push_string(void) {
    bfu_string string = bfu_string_new();
    bfu_string other = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr(&other, "world") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push_cstr(&string, "hello ") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push_string(&string, &other) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "hello world");

    bfu_string_delete(&other);
    bfu_string_delete(&string);
}

static void case_push_oom(void) {
    bfu_string string = bfu_string_new();

    test_fail_next_realloc();
    TEST_ASSERT(bfu_string_push_cstr(&string, "x") == BFU_ERROR_OOM);
    TEST_ASSERT(string.data == NULL);
    TEST_ASSERT(string.size == 0);

    bfu_string_delete(&string);
}

static void case_push_macros(void) {
    bfu_string string = bfu_string_new();
    bfu_string other = bfu_string_new();

    TEST_ASSERT(bfu_string_push(&string, "abc") == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "abc");

    TEST_ASSERT(bfu_string_push(&string, "def", 2) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "abcde");

    TEST_ASSERT(bfu_string_push(&other, "XY") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push(&string, &other) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "abcdeXY");

    bfu_string_delete(&other);
    bfu_string_delete(&string);
}

static void case_push_growth(void) {
    bfu_string exact = bfu_string_new();
    bfu_string long_value = bfu_string_new();
    char *buffer = malloc(BUFSIZ + 1000);
    TEST_ASSERT(buffer != NULL);

    memset(buffer, 'a', BUFSIZ + 1000);

    /* Exactly BUFSIZ characters: the terminating NUL needs one more byte. */
    TEST_ASSERT(bfu_string_push_cstr_sized(&exact, buffer, BUFSIZ) == BFU_SUCCESS);
    TEST_ASSERT(exact.size == BUFSIZ);
    TEST_ASSERT(exact.capacity > BUFSIZ);
    TEST_ASSERT(exact.data[BUFSIZ] == '\0');

    /* More than BUFSIZ characters: the character loop must grow the buffer. */
    TEST_ASSERT(bfu_string_push_cstr_sized(&long_value, buffer, BUFSIZ + 1000)
                == BFU_SUCCESS);
    TEST_ASSERT(long_value.size == BUFSIZ + 1000);
    TEST_ASSERT(long_value.capacity > BUFSIZ + 1000);
    TEST_ASSERT(long_value.data[BUFSIZ + 1000] == '\0');

    free(buffer);
    bfu_string_delete(&long_value);
    bfu_string_delete(&exact);
}

/* ------------------------------------------------------------------ */
/* println                                                            */
/* ------------------------------------------------------------------ */

static void println_cb(void *arg) {
    bfu_string_println((bfu_string *) arg);
}

static void case_println(void) {
    bfu_string string = bfu_string_new();
    bfu_string empty = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr(&string, "captured line") == BFU_SUCCESS);

    char *out = test_capture_stdout(println_cb, &string);
    TEST_ASSERT(out != NULL);
    TEST_ASSERT_STR(out, "captured line\n");
    free(out);

    /* Empty string: only the newline is printed (data may be NULL). */
    out = test_capture_stdout(println_cb, &empty);
    TEST_ASSERT(out != NULL);
    TEST_ASSERT_STR(out, "\n");
    free(out);

    bfu_string_delete(&string);
    bfu_string_delete(&empty);
}

/* ------------------------------------------------------------------ */
/* Duplication                                                        */
/* ------------------------------------------------------------------ */

static void case_dup_null(void) {
    bfu_string string = bfu_string_new();

    TEST_ASSERT(bfu_string_dup_string(NULL, &string) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_dup_string(&string, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_dup_cstr(NULL, "x") == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_dup_cstr(&string, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_dup_cstr_sized(NULL, "x", 1) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_dup_cstr_sized(&string, NULL, 1) == BFU_ERROR_NULLPTR);

    bfu_string_delete(&string);
}

static void case_dup(void) {
    bfu_string src = bfu_string_new();
    bfu_string dst = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr(&src, "source") == BFU_SUCCESS);

    TEST_ASSERT(bfu_string_dup_string(&dst, &src) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&dst, "source");

    /* Duplicating a longer value replaces the previous contents. */
    TEST_ASSERT(bfu_string_push_cstr(&src, " longer") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_dup_string(&dst, &src) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&dst, "source longer");

    /* Duplicating a shorter value truncates. */
    TEST_ASSERT(bfu_string_dup_cstr(&dst, "new") == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&dst, "new");

    TEST_ASSERT(bfu_string_dup_cstr_sized(&dst, "abcdef", 3) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&dst, "abc");

    /* Embedded NUL through the sized variant. */
    TEST_ASSERT(bfu_string_dup_cstr_sized(&dst, "a\0b", 3) == BFU_SUCCESS);
    TEST_ASSERT(dst.size == 3);
    TEST_ASSERT(memcmp(dst.data, "a\0b", 3) == 0);

    bfu_string_delete(&dst);
    bfu_string_delete(&src);
}

static void case_dup_macros(void) {
    bfu_string src = bfu_string_new();
    bfu_string dst = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr(&src, "macro") == BFU_SUCCESS);

    TEST_ASSERT(bfu_string_dup(&dst, "literal") == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&dst, "literal");

    TEST_ASSERT(bfu_string_dup(&dst, &src) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&dst, "macro");

    TEST_ASSERT(bfu_string_dup(&dst, "abcdef", 2) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&dst, "ab");

    bfu_string_delete(&dst);
    bfu_string_delete(&src);
}

static void case_dup_oom(void) {
    bfu_string src = bfu_string_new();
    bfu_string dst = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr(&src, "source") == BFU_SUCCESS);

    test_fail_next_realloc();
    TEST_ASSERT(bfu_string_dup_string(&dst, &src) == BFU_ERROR_OOM);
    TEST_ASSERT(dst.data == NULL);
    TEST_ASSERT(dst.size == 0);

    bfu_string_delete(&dst);
    bfu_string_delete(&src);
}

/* ------------------------------------------------------------------ */
/* Comparison                                                         */
/* ------------------------------------------------------------------ */

static void case_eq(void) {
    bfu_string a = bfu_string_new();
    bfu_string b = bfu_string_new();
    bfu_string c = bfu_string_new();
    bfu_string shorter = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr(&a, "abc") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push_cstr(&b, "abc") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push_cstr(&c, "abd") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push_cstr(&shorter, "ab") == BFU_SUCCESS);

    TEST_ASSERT(!bfu_string_eq(NULL, &a));
    TEST_ASSERT(!bfu_string_eq(&a, NULL));
    TEST_ASSERT(!bfu_string_eq(NULL, NULL));

    TEST_ASSERT(bfu_string_eq(&a, &b));
    TEST_ASSERT(bfu_string_eq(&a, &a));
    TEST_ASSERT(!bfu_string_eq(&a, &c));
    TEST_ASSERT(!bfu_string_eq(&a, &shorter));

    bfu_string empty_a = bfu_string_new();
    bfu_string empty_b = bfu_string_new();
    TEST_ASSERT(bfu_string_eq(&empty_a, &empty_b));
    bfu_string_delete(&empty_a);
    bfu_string_delete(&empty_b);

    bfu_string_delete(&shorter);
    bfu_string_delete(&c);
    bfu_string_delete(&b);
    bfu_string_delete(&a);
}

static void case_cmp(void) {
    bfu_string a = bfu_string_new();
    bfu_string b = bfu_string_new();
    bfu_string c = bfu_string_new();
    bfu_string shorter = bfu_string_new();
    bfu_string longer_different = bfu_string_new();
    bfu_string empty_a = bfu_string_new();
    bfu_string empty_b = bfu_string_new();

    TEST_ASSERT(bfu_string_push_cstr(&a, "abc") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push_cstr(&b, "abc") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push_cstr(&c, "abd") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push_cstr(&shorter, "ab") == BFU_SUCCESS);
    TEST_ASSERT(bfu_string_push_cstr(&longer_different, "abdX") == BFU_SUCCESS);

    TEST_ASSERT(bfu_string_cmp(NULL, &a) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_cmp(&a, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_cmp(&a, &b) == 0);
    TEST_ASSERT(bfu_string_cmp(&a, &c) < 0);
    TEST_ASSERT(bfu_string_cmp(&c, &a) > 0);

    TEST_ASSERT(bfu_string_cmp(&a, &shorter) > 0);
    TEST_ASSERT(bfu_string_cmp(&shorter, &a) < 0);

    /* Different sizes and a differing byte: the size difference must not
       override the byte comparison result. */
    TEST_ASSERT(bfu_string_cmp(&a, &longer_different) < 0);
    TEST_ASSERT(bfu_string_cmp(&longer_different, &a) > 0);
    TEST_ASSERT(bfu_string_cmp(&empty_a, &empty_b) == 0);

    bfu_string_delete(&empty_b);
    bfu_string_delete(&empty_a);
    bfu_string_delete(&longer_different);
    bfu_string_delete(&shorter);
    bfu_string_delete(&c);
    bfu_string_delete(&b);
    bfu_string_delete(&a);
}

/* ------------------------------------------------------------------ */
/* Formatting                                                         */
/* ------------------------------------------------------------------ */

static void case_format_null(void) {
    bfu_string string = bfu_string_new();

    TEST_ASSERT(bfu_string_format(NULL, "x") == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_format(&string, NULL) == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_push_format(NULL, "x") == BFU_ERROR_NULLPTR);
    TEST_ASSERT(bfu_string_push_format(&string, NULL) == BFU_ERROR_NULLPTR);

    bfu_string_delete(&string);
}

static void case_format(void) {
    bfu_string string = bfu_string_new();

    /* Fresh string: first vsnprintf() measures, the second one writes. */
    TEST_ASSERT(bfu_string_format(&string, "[%d, %d]", 1, 22) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "[1, 22]");
    TEST_ASSERT(string.capacity >= string.size + 1);

    /* Capacity is large enough now. */
    TEST_ASSERT(bfu_string_format(&string, "%s", "ok") == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "ok");

    TEST_ASSERT(bfu_string_format(&string, "%s", "") == BFU_SUCCESS);
    TEST_ASSERT(string.size == 0);

    bfu_string_delete(&string);
}

static void case_format_invalid(void) {
    bfu_string string = bfu_string_new();

    /* An invalid wide character makes vsnprintf() return -1 on glibc. */
    TEST_ASSERT(bfu_string_format(&string, "%lc", (wint_t) 0xD800) == BFU_ERROR_OVERFLOW);
    TEST_ASSERT(string.size == 0);

    bfu_string_delete(&string);
}

static void case_push_format(void) {
    bfu_string string = bfu_string_new();

    TEST_ASSERT(bfu_string_push_format(&string, "%s", "Hello") == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "Hello");

    /* Not enough spare capacity after "Hello": growth branch. */
    TEST_ASSERT(bfu_string_push_format(&string, " %d", 42) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "Hello 42");

    TEST_ASSERT(bfu_string_push_format(&string, "%s", "") == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&string, "Hello 42");

    bfu_string_delete(&string);
}

static void case_push_format_invalid(void) {
    bfu_string string = bfu_string_new();

    TEST_ASSERT(bfu_string_push_format(&string, "%lc", (wint_t) 0xD800) == BFU_ERROR_OVERFLOW);
    TEST_ASSERT(string.size == 0);

    bfu_string_delete(&string);
}

static void case_format_second_call_fails(void) {
    bfu_string string = bfu_string_new();

    /* The first vsnprintf() measures the output; the second one, after the
       buffer has grown, fails. */
    test_fail_vsnprintf_after(1);
    TEST_ASSERT(bfu_string_format(&string, "%s", "needs growth")
                == BFU_ERROR_OVERFLOW);
    TEST_ASSERT(string.size == 0);

    bfu_string_delete(&string);
}

static void case_push_format_second_call_fails(void) {
    bfu_string string = bfu_string_new();
    TEST_ASSERT(bfu_string_push_cstr(&string, "base") == BFU_SUCCESS);

    char *long_value = malloc(BUFSIZ + 100);
    TEST_ASSERT(long_value != NULL);
    memset(long_value, 'a', BUFSIZ + 99);
    long_value[BUFSIZ + 99] = '\0';

    /* Not enough spare capacity, so the second vsnprintf() runs and fails. */
    test_fail_vsnprintf_after(1);
    TEST_ASSERT(bfu_string_push_format(&string, "%s", long_value)
                == BFU_ERROR_OVERFLOW);
    TEST_ASSERT_BFU_STR(&string, "base");

    free(long_value);
    bfu_string_delete(&string);
}

/* ------------------------------------------------------------------ */
/* Split                                                              */
/* ------------------------------------------------------------------ */

static void assert_parts(bfu_array(bfu_string) parts, const char **expected, u64 count) {
    TEST_ASSERT(bfu_array_size(parts) == count);

    for (u64 i = 0; i < count && i < bfu_array_size(parts); i++) {
        TEST_ASSERT(parts[i].size == strlen(expected[i]));
        if (parts[i].size > 0) {
            TEST_ASSERT(parts[i].data != NULL);
            TEST_ASSERT(memcmp(parts[i].data, expected[i], parts[i].size) == 0);
        }
    }
}

static void free_parts(bfu_array(bfu_string) parts) {
    for (u64 i = 0; i < bfu_array_size(parts); i++) {
        bfu_string_delete(&parts[i]);
    }
    bfu_array_delete(parts);
}

static void check_split(const char *input, char delimiter,
                        const char **expected, u64 count) {
    bfu_string string = bfu_string_new();

    TEST_ASSERT(bfu_string_dup_cstr(&string, input) == BFU_SUCCESS);

    bfu_array(bfu_string) parts = bfu_string_split(&string, delimiter);
    assert_parts(parts, expected, count);
    free_parts(parts);

    bfu_string_delete(&string);
}

static void case_split(void) {
    const char *path_expected[] = {"", "hello", "world", "!"};
    check_split("/hello/world/!", '/', path_expected, 4);

    const char *single_expected[] = {"abc"};
    check_split("abc", '/', single_expected, 1);

    /* Trailing delimiter produces a final empty part. */
    const char *trailing_expected[] = {"abc", ""};
    check_split("abc/", '/', trailing_expected, 2);

    /* Leading delimiter produces an initial empty part. */
    const char *leading_expected[] = {"", "abc"};
    check_split("/abc", '/', leading_expected, 2);

    /* Consecutive delimiters produce empty parts. */
    const char *consecutive_expected[] = {"a", "", "b"};
    check_split("a//b", '/', consecutive_expected, 3);

    const char *many_expected[] = {"a", "", "b", ""};
    check_split("a,,b,", ',', many_expected, 4);

    /* Delimiter-only input. */
    const char *only_expected[] = {"", "", "", ""};
    check_split("///", '/', only_expected, 4);

    /* Empty input yields exactly one empty part. */
    const char *empty_expected[] = {""};
    check_split("", '/', empty_expected, 1);
}

static void case_split_long_parts(void) {
    char *buffer = malloc(BUFSIZ + 1001);
    TEST_ASSERT(buffer != NULL);

    /* More than BUFSIZ characters in one part. */
    memset(buffer, 'a', BUFSIZ + 1000);
    buffer[BUFSIZ + 1000] = '\0';

    bfu_string long_value = bfu_string_new();
    TEST_ASSERT(bfu_string_dup_cstr_sized(&long_value, buffer, BUFSIZ + 1000)
                == BFU_SUCCESS);
    bfu_array(bfu_string) parts = bfu_string_split(&long_value, '/');
    TEST_ASSERT(bfu_array_size(parts) == 1);
    TEST_ASSERT(parts[0].size == BUFSIZ + 1000);
    free_parts(parts);
    bfu_string_delete(&long_value);

    /* Exactly BUFSIZ characters followed by a delimiter. */
    memset(buffer, 'a', BUFSIZ);
    buffer[BUFSIZ] = '/';
    buffer[BUFSIZ + 1] = '\0';

    bfu_string delimited = bfu_string_new();
    TEST_ASSERT(bfu_string_dup_cstr(&delimited, buffer) == BFU_SUCCESS);
    parts = bfu_string_split(&delimited, '/');
    TEST_ASSERT(bfu_array_size(parts) == 2);
    TEST_ASSERT(parts[0].size == BUFSIZ);
    TEST_ASSERT(parts[1].size == 0);
    free_parts(parts);
    bfu_string_delete(&delimited);

    /* Exactly BUFSIZ characters and no delimiter. */
    memset(buffer, 'a', BUFSIZ);
    buffer[BUFSIZ] = '\0';

    bfu_string exact = bfu_string_new();
    TEST_ASSERT(bfu_string_dup_cstr(&exact, buffer) == BFU_SUCCESS);
    parts = bfu_string_split(&exact, '/');
    TEST_ASSERT(bfu_array_size(parts) == 1);
    TEST_ASSERT(parts[0].size == BUFSIZ);
    free_parts(parts);
    bfu_string_delete(&exact);

    free(buffer);
}

static void case_split_push_oom(void) {
    bfu_string string = bfu_string_new();
    TEST_ASSERT(bfu_string_dup_cstr(&string, "a/b") == BFU_SUCCESS);

    /* The first part cannot be appended.  bfu_string_split() has no error
       return, so the part is dropped and the split continues. */
    test_fail_nth_malloc(1);
    bfu_array(bfu_string) parts = bfu_string_split(&string, '/');
    TEST_ASSERT(parts != NULL);
    TEST_ASSERT(bfu_array_size(parts) == 1);
    TEST_ASSERT(parts[0].size == 1);
    TEST_ASSERT(parts[0].data != NULL);
    TEST_ASSERT(memcmp(parts[0].data, "b", 1) == 0);

    free_parts(parts);
    bfu_string_delete(&string);
}

static void case_split_final_push_oom(void) {
    bfu_string string = bfu_string_new();
    TEST_ASSERT(bfu_string_dup_cstr(&string, "abc") == BFU_SUCCESS);

    /* The only append fails, so no array is ever allocated. */
    test_fail_nth_malloc(1);
    bfu_array(bfu_string) parts = bfu_string_split(&string, '/');
    TEST_ASSERT(parts == NULL);

    free_parts(parts);
    bfu_string_delete(&string);
}

static void case_split_cleanup_attribute(void) {
    bfu_string string = bfu_string_new();
    TEST_ASSERT(bfu_string_dup_cstr(&string, "a/b/c") == BFU_SUCCESS);

    {
        bfu_array_f(bfu_string) parts = bfu_string_split(&string, '/');
        TEST_ASSERT(bfu_array_size(parts) == 3);
        TEST_ASSERT_BFU_STR(&parts[0], "a");
        TEST_ASSERT_BFU_STR(&parts[1], "b");
        TEST_ASSERT_BFU_STR(&parts[2], "c");

        for (u64 i = 0; i < bfu_array_size(parts); i++) {
            bfu_string_delete(&parts[i]);
        }
        /* Array memory itself is released by bfu_array_bfu_string_delete(). */
    }

    bfu_string_delete(&string);
}

/* ------------------------------------------------------------------ */

void test_string_suite(void) {
    printf("\n[bfu_string]\n");

    TEST_RUN(case_delete);
    TEST_RUN(case_push_null);
    TEST_RUN(case_push_cstr);
    TEST_RUN(case_push_cstr_sized);
    TEST_RUN(case_push_string);
    TEST_RUN(case_push_oom);
    TEST_RUN(case_push_macros);
    TEST_RUN(case_push_growth);
    TEST_RUN(case_println);
    TEST_RUN(case_dup_null);
    TEST_RUN(case_dup);
    TEST_RUN(case_dup_macros);
    TEST_RUN(case_dup_oom);
    TEST_RUN(case_eq);
    TEST_RUN(case_cmp);
    TEST_RUN(case_format_null);
    TEST_RUN(case_format);
    TEST_RUN(case_format_invalid);
    TEST_RUN(case_format_second_call_fails);
    TEST_RUN(case_push_format);
    TEST_RUN(case_push_format_invalid);
    TEST_RUN(case_push_format_second_call_fails);
    TEST_RUN(case_split);
    TEST_RUN(case_split_long_parts);
    TEST_RUN(case_split_push_oom);
    TEST_RUN(case_split_final_push_oom);
    TEST_RUN(case_split_cleanup_attribute);
}
