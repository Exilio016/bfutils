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

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float   f32;
typedef double  f64;

/* Result type returned by most bfutils functions. BFU_SUCCESS (0) indicates
 * success; any negative value indicates an error. */
typedef i8 bfu_result;

/* Operation completed successfully. */
#define BFU_SUCCESS 0
/* A NULL pointer was passed where a valid pointer was required. */
#define BFU_ERROR_NULLPTR -1
/* Memory allocation failed (out of memory). */
#define BFU_ERROR_OOM -2
/* A value overflowed its allowed range or buffer. */
#define BFU_ERROR_OVERFLOW -3
/* An index or offset was out of bounds. */
#define BFU_ERROR_OUT_OF_BOUND -4
/* The requested item was not found. */
#define BFU_ERROR_NOT_FOUND -5
/* A invalid value was passed */
#define BFU_ERROR_INVALID -6
/* An internal function failed and set errno */
#define BFU_ERROR_ERRNO -7
