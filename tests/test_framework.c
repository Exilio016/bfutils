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

#include <stdarg.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Linker provided originals (see -Wl,--wrap=... in the Makefile)      */
/* ------------------------------------------------------------------ */

extern void *__real_malloc(size_t size);
extern void *__real_realloc(void *ptr, size_t size);
extern void __real_free(void *ptr);
extern int __real_vsnprintf(char *buf, size_t size, const char *format,
                            va_list ap);

/* ------------------------------------------------------------------ */
/* Fault injection                                                    */
/* ------------------------------------------------------------------ */

static int pending_malloc_failures = 0;
static int pending_realloc_failures = 0;
static int malloc_countdown = -1;      /* -1 = disabled */
static int vsnprintf_countdown = -1;   /* -1 = disabled */
static int free_calls = 0;

void test_fail_next_malloc(void) {
    pending_malloc_failures++;
}

void test_fail_next_realloc(void) {
    pending_realloc_failures++;
}

void test_fail_nth_malloc(int n) {
    malloc_countdown = n;
}

void test_fail_vsnprintf_after(int successful_calls) {
    vsnprintf_countdown = successful_calls;
}

void test_clear_faults(void) {
    pending_malloc_failures = 0;
    pending_realloc_failures = 0;
    malloc_countdown = -1;
    vsnprintf_countdown = -1;
}

void *__wrap_malloc(size_t size) {
    if (pending_malloc_failures > 0) {
        pending_malloc_failures--;
        return NULL;
    }
    if (malloc_countdown >= 0) {
        if (malloc_countdown == 1) {
            malloc_countdown = -1;
            return NULL;
        }
        malloc_countdown--;
    }
    return __real_malloc(size);
}

void *__wrap_realloc(void *ptr, size_t size) {
    if (pending_realloc_failures > 0) {
        pending_realloc_failures--;
        return NULL;
    }
    return __real_realloc(ptr, size);
}

int __wrap_vsnprintf(char *buf, size_t size, const char *format, va_list ap) {
    if (vsnprintf_countdown >= 0) {
        if (vsnprintf_countdown == 0) {
            vsnprintf_countdown = -1;
            return -1;
        }
        vsnprintf_countdown--;
    }
    return __real_vsnprintf(buf, size, format, ap);
}

void __wrap_free(void *ptr) {
    if (ptr != NULL) {
        free_calls++;
    }
    __real_free(ptr);
}

int test_free_count(void) {
    return free_calls;
}

void test_reset_free_count(void) {
    free_calls = 0;
}

/* ------------------------------------------------------------------ */
/* Counters / assertions                                              */
/* ------------------------------------------------------------------ */

static int checks = 0;
static int failures = 0;
static int cases = 0;
static int case_failures_before = 0;
static const char *current_case = "";

int test_checks_run(void) { return checks; }
int test_failures(void) { return failures; }
int test_cases_run(void) { return cases; }

void test_begin_case(const char *name) {
    current_case = name;
    case_failures_before = failures;
    cases++;
    test_clear_faults();
    test_reset_free_count();
    fflush(stdout);
}

void test_end_case(void) {
    int failed = failures - case_failures_before;
    if (failed == 0) {
        printf("  ok    %s\n", current_case);
    } else {
        printf("  FAIL  %s (%d failure%s)\n", current_case, failed,
               failed == 1 ? "" : "s");
    }
    current_case = "";
}

void test_assert_impl(bool ok, const char *expr, const char *file, int line) {
    checks++;
    if (!ok) {
        failures++;
        printf("        %s:%d: assertion failed: %s\n", file, line, expr);
        fflush(stdout);
    }
}

void test_assert_str_impl(const char *actual, const char *expected,
                          const char *file, int line) {
    checks++;
    bool equal = actual != NULL && expected != NULL && strcmp(actual, expected) == 0;
    if (!equal) {
        failures++;
        printf("        %s:%d: string mismatch\n", file, line);
        printf("          expected: \"%s\"\n", expected ? expected : "(null)");
        printf("          actual:   \"%s\"\n", actual ? actual : "(null)");
        fflush(stdout);
    }
}

/* ------------------------------------------------------------------ */
/* stdout capturing                                                   */
/* ------------------------------------------------------------------ */

char *test_capture_stdout(void (*fn)(void *), void *arg) {
    fflush(stdout);

    FILE *capture = tmpfile();
    if (capture == NULL) return NULL;

    int saved_fd = dup(fileno(stdout));
    if (saved_fd < 0) {
        fclose(capture);
        return NULL;
    }

    if (dup2(fileno(capture), fileno(stdout)) < 0) {
        close(saved_fd);
        fclose(capture);
        return NULL;
    }

    fn(arg);
    fflush(stdout);

    dup2(saved_fd, fileno(stdout));
    close(saved_fd);

    if (fseek(capture, 0, SEEK_END) != 0) {
        fclose(capture);
        return NULL;
    }
    long length = ftell(capture);
    if (length < 0) {
        fclose(capture);
        return NULL;
    }
    rewind(capture);

    char *result = malloc((size_t) length + 1);
    if (result == NULL) {
        fclose(capture);
        return NULL;
    }

    size_t read = fread(result, 1, (size_t) length, capture);
    result[read] = '\0';
    fclose(capture);
    return result;
}
