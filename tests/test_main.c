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

void test_array_suite(void);
void test_string_suite(void);
void test_map_suite(void);
void test_process_suite(void);

int main(int argc, char **argv) {
    const char *only = argc > 1 ? argv[1] : NULL;

    /* Unbuffered so assertion details survive a crash in the library. */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("bfutils test suite\n");

    if (only == NULL || strcmp(only, "string") == 0) test_string_suite();
    if (only == NULL || strcmp(only, "array") == 0) test_array_suite();
    if (only == NULL || strcmp(only, "map") == 0) test_map_suite();
    if (only == NULL || strcmp(only, "process") == 0) test_process_suite();

    printf("\n----------------------------------------\n");
    printf("%d cases, %d checks, %d failure%s\n",
           test_cases_run(), test_checks_run(), test_failures(),
           test_failures() == 1 ? "" : "s");
    printf("%s\n", test_failures() == 0 ? "PASS" : "FAIL");

    return test_failures() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
