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

/*
 * Minimal, dependency-free test harness for bfutils.
 *
 * Tests assert the documented/expected behaviour of the public API.  If the
 * library is wrong (including crashing), the test run fails.
 *
 * Allocation-failure injection relies on the linker's --wrap option (see the
 * Makefile), which redirects the library's malloc/realloc references to the
 * __wrap_malloc/__wrap_realloc implementations in test_framework.c.  This is
 * only used to check the BFU_ERROR_OOM paths.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bfu.h"

/* ------------------------------------------------------------------ */
/* Assertions / cases                                                 */
/* ------------------------------------------------------------------ */

void test_begin_case(const char *name);
void test_end_case(void);

void test_assert_impl(bool ok, const char *expr, const char *file, int line);
void test_assert_str_impl(const char *actual, const char *expected,
                          const char *file, int line);

int test_checks_run(void);
int test_failures(void);
int test_cases_run(void);

#define TEST_ASSERT(cond) \
    test_assert_impl(!!(cond), #cond, __FILE__, __LINE__)

#define TEST_ASSERT_STR(actual, expected) \
    test_assert_str_impl((actual), (expected), __FILE__, __LINE__)

#define TEST_RUN(case_fn) do { \
    test_begin_case(#case_fn); \
    case_fn(); \
    test_end_case(); \
} while (0)

/* ------------------------------------------------------------------ */
/* Fault injection                                                    */
/* ------------------------------------------------------------------ */

/* Make the next malloc()/realloc() call fail with NULL.  Each helper only
   arms a single failure; test_begin_case() disarms anything left over. */
void test_fail_next_malloc(void);
void test_fail_next_realloc(void);

/* Make the Nth malloc() call fail (1 means the very next call). */
void test_fail_nth_malloc(int n);

/* Make the next vsnprintf() call fail with -1 after `successful_calls` calls
   have been allowed through (0 means the very next call). */
void test_fail_vsnprintf_after(int successful_calls);

/* Disarm every armed fault. */
void test_clear_faults(void);

/* Count calls to free() made through the wrapper since the last reset.
   test_begin_case() resets the counter. */
int test_free_count(void);
void test_reset_free_count(void);

/* ------------------------------------------------------------------ */
/* stdout capturing                                                   */
/* ------------------------------------------------------------------ */

/* Runs fn(arg) with stdout redirected into a temporary file and returns a
   heap allocated copy of everything that was written.  Free with free(). */
char *test_capture_stdout(void (*fn)(void *), void *arg);

/* ------------------------------------------------------------------ */
/* Small helpers                                                      */
/* ------------------------------------------------------------------ */

static inline bool test_bfu_string_equals(bfu_string *string, const char *expected) {
    size_t len = strlen(expected);
    if (string == NULL) return false;
    if (string->size != (u64) len) return false;
    if (len == 0) return true;
    if (string->data == NULL) return false;
    return memcmp(string->data, expected, len) == 0;
}

#define TEST_ASSERT_BFU_STR(string, expected) \
    test_assert_impl(test_bfu_string_equals((string), (expected)), \
                     "bfu_string equals \"" expected "\"", __FILE__, __LINE__)
