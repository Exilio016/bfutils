# bfutils

A small, dependency-free C utility library: growable strings, type-generic
dynamic arrays, and open-addressing hash maps. Written in C11 with a handful of
GCC/Clang extensions, MIT licensed, and tested with fault injection.

```
bfu_string_f  growable, null-terminated strings with formatting and splitting
bfu_array     dynamic arrays for any type (push, pop, unordered removal)
bfu_map       hash maps with string or arbitrary keys (rapidhash under the hood)
```

It's the kind of thing you vendor into a project and forget about — one header
to include, three source files to compile, no external dependencies (except a
vendored copy of [rapidhash](https://github.com/Nicoshev/rapidhash), also MIT).

## Quick example

```c
#include <bfu.h>
#include <stdio.h>

/* Needed so bfu_array_f(int) can auto-free the array on scope exit. */
DEFINE_BFU_ARRAY_DELETE(int)

int main(void) {
    /* bfu_string_f frees the string automatically when it goes out of scope. */
    bfu_string_f msg = bfu_string_new();
    bfu_string_push(&msg, "bfu_array has ");

    /* bfu_array_f frees the array automatically when it goes out of scope. */
    bfu_array_f(int) nums = NULL;
    bfu_array_push(nums, 10);
    bfu_array_push(nums, 20);
    bfu_array_push(nums, 30);

    bfu_string_push_format(&msg, "%lu items", bfu_array_size(nums));
    bfu_string_println(&msg);   /* bfu_array has 3 items */

    int last;
    bfu_array_pop(last, nums); /* last == 30, nums now has 2 items */

    /* String-keyed map. Items are passed by value, not by pointer. */
    bfu_map_str(int) counts = NULL;
    bfu_map_init(counts);
    bfu_map_insert(counts, "apples", 1);
    bfu_map_insert(counts, "oranges", 2);

    int n = 0;
    bfu_map_get(n, counts, "apples"); /* n == 1 */
    printf("apples => %d\n", n);

    bfu_map_delete(counts);
    return 0;
}
```

Compile it:

```sh
gcc -Wall -Wextra -Iinclude example.c src/bfu_string.c src/bfu_array.c \
    src/bfu_map.c -o example
```

## Building

Requires GCC or Clang (the library uses GNU extensions like `typeof`, statement
expressions, and `__attribute__((cleanup))`).  The project is built with CMake.

```sh
cmake -S . -B build              # configure a debug build 
cmake --build build              # build the static + shared libraries and tests
cmake --build build --target test   # run all test suites (string, array, map, process)
./build/bfu_test string          # one suite only
```

Release build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Coverage build (`gcov`, enforces 100% line/branch coverage on `src/*.c`):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target coverage
```

This rebuilds with profiling instrumentation, runs all test suites, and runs
`tests/check_coverage.py`.

Install locations can be overridden, e.g.
`cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr`.

```sh
cmake --install build            # install headers, libs, pkg-config and cmake files
```

Consumers can then use `find_package(bfutils REQUIRED)` and link
`bfutils::bfutils`; a `bfutils.pc` pkg-config file is also installed.

## Using it in your project

- **Simplest:** copy `include/` and `src/` into your tree, add `-Iinclude`, and
compile the three `.c` files alongside your own. `#include <bfu.h>` pulls in
everything.

- **pkg-config:**

```sh
cc $(pkg-config --cflags --libs bfutils) myprog.c -o myprog
```

- **CMake** (after `cmake --install build`):

```cmake
find_package(bfutils REQUIRED)
target_link_libraries(myprog PRIVATE bfutils::bfutils)
```

The `cmake --install build` step also writes a `bfutils.pc` and a `bfutilsConfig.cmake`, so
both work out of the box.

## What's inside

### `bfu_string` — `include/bfu_string.h`

A growable, always-null-terminated string type (`bfu_string { size, capacity, data }`).

- `bfu_string_new()` / `bfu_string_f` — create an empty string; `_f` variant
  auto-frees on scope exit (GCC/Clang cleanup attribute).
- `bfu_string_push(&s, "text")` / `bfu_string_push(&s, "text", n)` /
  `bfu_string_push(&s, other_string)` — append, with `_Generic` dispatch.
- `bfu_string_push_format(&s, "pi ≈ %.2f", 3.14159)` — printf-style append.
- `bfu_string_format(&s, "…")` — replace contents with formatted output.
- `bfu_string_dup(&dst, src)` — copy a cstr or another string into `dst`.
- `bfu_string_eq` / `bfu_string_cmp` — equality and lexicographic comparison.
- `bfu_string_split(&s, ',')` — split on a delimiter into a `bfu_array`.
- `bfu_string_println(&s)` — print to stdout with a newline.

All functions return `bfu_result` (`BFU_SUCCESS == 0`, negative on error) and
document `BFU_ERROR_NULLPTR` / `BFU_ERROR_OOM` / `BFU_ERROR_OVERFLOW` where
applicable.

### `bfu_array` — `include/bfu_array.h`

Type-generic dynamic arrays built on macros. `bfu_array(T)` is just `T*`.
The `push`/`pop`/`remove`/`delete` macros take the array variable itself — they
pass its address internally — so you call them as `bfu_array_push(arr, item)`,
not `bfu_array_push(&arr, item)`.

- `bfu_array_push(arr, item)` — append, growing (and auto-initializing) as needed.
- `bfu_array_pop(ret, arr)` — remove and return the last item into `ret`.
- `bfu_array_remove_unordered(ret, arr, idx)` — O(1) removal, order not preserved.
- `bfu_array_size(arr)` / `bfu_array_capacity(arr)` — query.
- `bfu_array_init`, `bfu_array_dup`, `bfu_array_delete(arr)` — manual control.
- `bfu_array_f(T) arr = NULL;` — auto-frees when it goes out of scope.

### `bfu_map` — `include/bfu_map.h`

Open-addressing hash map hashed with rapidhash. Two flavors:

- `bfu_map(K, V)` — any key type, keys copied by `memcmp`.
- `bfu_map_str(V)` — shorthand for `bfu_map(char*, V)`; string keys are `strdup`ed
  and freed automatically.

The `init`/`insert`/`get`/`remove`/`delete` macros take the map variable itself,
and items are passed **by value**, not by pointer.

- `bfu_map_init(map)` — set up with deduced sizes.
- `bfu_map_insert(map, key, item)` — insert or update, growing past 75% load.
- `bfu_map_get(ret, map, key)` / `bfu_map_has_item(map, key)` — lookup.
- `bfu_map_remove(ret, map, key)` — delete and return the stored item into `ret`.
- `bfu_map_foreach(map, i)` — iterate over in-use slots.
- `bfu_map_delete(map)` — frees everything (string keys included).

### `bfu_process` — `include/bfu_process.h`

Spawn and interact with child processes. A `bfu_process` describes a child and
how each of its standard streams (stdin, stdout, stderr) is connected to the
parent:

- `BFU_PROCESS_FD_NONE` — stream connected to `/dev/null`
- `BFU_PROCESS_FD_INHERIT` — stream inherited from the parent
- `BFU_PROCESS_FD_PIPE` — pipe created between parent and child
- `BFU_PROCESS_FD_FILE` — an existing file descriptor is used

Helper functions build `bfu_process_fd` values: `bfu_process_fd_none()`,
`bfu_process_fd_inherit()`, `bfu_process_fd_pipe()`, `bfu_process_fd_from_fd()`,
and `bfu_process_fd_from_file()`.

- `bfu_process_run(&p, "cat", NULL)` — spawn a child; `execvp` runs
  `arguments[0]` with the remaining arguments, terminated by `NULL`.
- `bfu_process_run_sync(&p, ...)` — run and wait in one call.
- `bfu_process_fd_read(&out, p.stdout_fd)` — read all output from a pipe into a
  `bfu_string` until EOF.
- `bfu_process_fd_write(p.stdin_fd, &input)` — write a string's contents to a
  pipe until fully written.
- `bfu_process_wait(&p)` — close stdin's write end (signaling EOF) and reap the
  child, returning its exit status.
- `bfu_process_close(&p)` — close the parent-owned stream descriptors.

All functions return `bfu_result` (`BFU_SUCCESS == 0`, negative on error) and
document `BFU_ERROR_NULLPTR` / `BFU_ERROR_OOM` / `BFU_ERROR_OVERFLOW` /
`BFU_ERROR_NOT_FOUND` / `BFU_ERROR_OUT_OF_BOUND` / `BFU_ERROR_INVALID` /
`BFU_ERROR_ERRNO` where applicable.

## Error handling

Most functions return `bfu_result`, an `i8`:

| Macro | Value | Meaning |
| ----- | ----- | ------- |
| `BFU_SUCCESS` | 0 | OK |
| `BFU_ERROR_NULLPTR` | -1 | NULL passed where a pointer was required |
| `BFU_ERROR_OOM` | -2 | Allocation failed |
| `BFU_ERROR_OVERFLOW` | -3 | Value overflowed its range/buffer |
| `BFU_ERROR_OUT_OF_BOUND` | -4 | Index/offset out of bounds |
| `BFU_ERROR_NOT_FOUND` | -5 | Item not present |

Macros like `bfu_array_push` and `bfu_map_insert` evaluate to a `bfu_result`,
so you can write `if (bfu_array_push(arr, item)) { /* handle */ }`.

## Tests

The suite lives in `tests/` and runs each module in its own process so a crash
in one suite doesn't silence the others. Error paths (OOM, formatting failure)
are exercised by wrapping `malloc`, `realloc`, `vsnprintf` and `free`
(`tests/test_framework.c` provides the wrappers).

```sh
cmake --build build --target test   # all suites
./build/bfu_test string   # one suite only
cmake --build build --target coverage   # 100% coverage gate on src/*.c
```

## License

MIT. See `LICENSE` for details. `include/rapidhash.h` is vendored from
[https://github.com/Nicoshev/rapidhash](https://github.com/Nicoshev/rapidhash)
and is also MIT (Copyright Nicolas De Carli).

---

bfutils · Bruno Flávio Ferreira <him@brunoflaviof.com> · 2026
