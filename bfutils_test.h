/* bfutils_test.h

DESCRIPTION: 

    This is a single-header-file library that provides a test framework to C.

USAGE:
    
    After declate all test funtions you can use the following X-macros to setup the tests:
       
       BFUTILS_TEST_LIST: A list of all test functions.
            #define BFUTILS_TEST_LIST \
                X("Test Name", test_function), \
                ...
       
       BFUTILS_TEST_BEFORE_ALL: A list of functions to be executed once before all tests.
            #define BFUTILS_TEST_BEFORE_ALL \
                X(before_function), \
                ...
       
       BFUTILS_TEST_BEFORE_EACH: A list of functions to be executed before each test.
            #define BFUTILS_TEST_BEFORE_EACH \
                X(before_function), \
                ...
        
       BFUTILS_TEST_AFTER_EACH: A list of functions to be executed after each test.
            #define BFUTILS_TEST_AFTER_EACH \
                X(after_function), \
                ...

       BFUTILS_TEST_AFTER_ALL: A list of functions to be executed one after all tests finished.
            #define BFUTILS_TEST_AFTER_EACH \
                X(after_function), \
                ...

    At the end of the file you need to declare the following:
        #define BFUTILS_TEST_MAIN
        #include "bfutils_test.h"
    
    This will create a main function that will execute all functions and print the result in the screen.
    
LICENSE:

    MIT License
    
    Copyright (c) 2024 Bruno Flávio Ferreira 
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

*/

#ifndef BFUTILS_TEST_H
#define BFUTILS_TEST_H

typedef void (*BFUtilsTestFunction)();

typedef struct {
    char *name;
    BFUtilsTestFunction func;
} BFUtilsTest;

static int bfutils_test_success = 1;

#define assert(exp) if (!(exp)) {printf("\tFAILED: %s\n", #exp); bfutils_test_success = 0; return; }
#endif // TEST_H
#ifdef BFUTILS_TEST_MAIN
#include <stdio.h>
#define ARRAY_LEN(a) (sizeof(a) / sizeof(*a))

int main(int argc, char *argv[]){ 
    BFUtilsTestFunction __before_all[] = {
        #ifdef BFUTILS_TEST_BEFORE_ALL
        #define X(func) func,
        BFUTILS_TEST_BEFORE_ALL
        #undef X
        #endif
    };
    BFUtilsTestFunction __before_each[] = {
        #ifdef BFUTILS_TEST_BEFORE_EACH
        #define X(func) func,
        BFUTILS_TEST_BEFORE_EACH
        #undef X
        #endif
    };
    BFUtilsTestFunction __after_all[] = {
        #ifdef BFUTILS_TEST_AFTER_ALL
        #define X(func) func,
        BFUTILS_TEST_AFTER_ALL
        #undef X
        #endif
    };
    BFUtilsTestFunction __after_each[] = {
        #ifdef BFUTILS_TEST_AFTER_EACH
        #define X(func) func,
        BFUTILS_TEST_AFTER_EACH
        #undef X
        #endif
    };
    BFUtilsTest __tests[] = {
        #ifdef BFUTILS_TEST_LIST
        #define X(n, f) (BFUtilsTest) {.name = n, .func = f},
        BFUTILS_TEST_LIST
        #undef X
        #endif
    };

    for (int i = 0; i < ARRAY_LEN(__before_all); i++) {
        __before_all[i]();
    }
    for (int i = 0; i < ARRAY_LEN(__tests); i++) {
        for (int j = 0; j < ARRAY_LEN(__before_each); j++) {
            __before_each[j]();
        }
        bfutils_test_success = 1;
        printf("Test '%s':\n", __tests[i].name);
        __tests[i].func();
        if (bfutils_test_success) {
            printf("\tPASSED\n");
        }
        for (int j = 0; j < ARRAY_LEN(__after_each); j++) {
            __after_each[j]();
        }
    }
    for (int i = 0; i < ARRAY_LEN(__after_all); i++) {
        __after_all[i]();
    }
}

#endif //BFUTILS_TEST_MAIN
