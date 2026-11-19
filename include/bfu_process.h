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

/* Spawning and interacting with child processes.
 *
 * A `bfu_process` describes a child process and how each of its standard
 * streams (stdin, stdout, stderr) is connected to the parent:
 *
 *   - BFU_PROCESS_FD_NONE    stream connected to /dev/null
 *   - BFU_PROCESS_FD_INHERIT stream inherited from the parent
 *   - BFU_PROCESS_FD_PIPE    pipe created between parent and child
 *   - BFU_PROCESS_FD_FILE    an existing file descriptor is used
 *
 * Use `bfu_process_run` to spawn the child, `bfu_process_fd_read` /
 * `bfu_process_fd_write` to exchange data over pipes, and `bfu_process_wait`
 * to reap the child and obtain its exit status.
 */

#include "bfu_string.h"
#include "bfu_types.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

/* Describes how a standard stream (stdin, stdout, stderr) of a child process
 * is connected. */
enum {
    /* The stream is connected to /dev/null. */
    BFU_PROCESS_FD_NONE,
    /* The stream is inherited from the parent process. */
    BFU_PROCESS_FD_INHERIT,
    /* A pipe is created between the parent and the child. */
    BFU_PROCESS_FD_PIPE,
    /* An existing file descriptor is used. */
    BFU_PROCESS_FD_FILE
};

/* Describes one end of a standard stream of a child process.
 *
 * `kind` is one of the BFU_PROCESS_FD_* constants. `fd` holds the file
 * descriptor in use: for `BFU_PROCESS_FD_FILE` it is the descriptor supplied
 * by the caller; after `bfu_process_run` it holds the pipe end owned by the
 * parent for `BFU_PROCESS_FD_PIPE` and `BFU_PROCESS_FD_NONE`. */
typedef struct {
    i8 kind;
    i32 fd;
} bfu_process_fd;

/* Describes a child process and how its standard streams are connected. */
typedef struct {
    pid_t pid;            /* Process id of the child. */
    bfu_process_fd stdin_fd;   /* Standard input. */
    bfu_process_fd stdout_fd;  /* Standard output. */
    bfu_process_fd stderr_fd;  /* Standard error. */
} bfu_process;

/* Returns a `bfu_process_fd` that opens `name` as a file.
 *
 * The file is opened with `flags` (passed to open(2), e.g. O_RDONLY,
 * O_WRONLY, O_RDWR). If opening fails `fd` is -1 and `bfu_process_run`
 * reports BFU_ERROR_ERRNO.
 *
 * @param name  Path of the file to open. Must not be NULL.
 * @param flags  Flags passed to open(2).
 * @return      A bfu_process_fd of kind BFU_PROCESS_FD_FILE.
 */
static inline bfu_process_fd bfu_process_fd_from_file(char *name, i32 flags) {
    return (bfu_process_fd) { BFU_PROCESS_FD_FILE, open(name, flags)};
}

/* Returns a `bfu_process_fd` wrapping the existing file descriptor `fd`.
 *
 * @param fd  File descriptor to use.
 * @return    A bfu_process_fd of kind BFU_PROCESS_FD_FILE.
 */
static inline bfu_process_fd bfu_process_fd_from_fd(i32 fd) {
    return (bfu_process_fd) { BFU_PROCESS_FD_FILE, fd };
}

/* Returns a `bfu_process_fd` of kind BFU_PROCESS_FD_NONE.
 *
 * The stream is connected to /dev/null. */
static inline bfu_process_fd bfu_process_fd_none() {
    return (bfu_process_fd) { .kind = BFU_PROCESS_FD_NONE };
}

/* Returns a `bfu_process_fd` of kind BFU_PROCESS_FD_PIPE.
 *
 * A pipe is created between the parent and the child by `bfu_process_run`. */
static inline bfu_process_fd bfu_process_fd_pipe() {
    return (bfu_process_fd) { .kind = BFU_PROCESS_FD_PIPE };
}

/* Returns a `bfu_process_fd` of kind BFU_PROCESS_FD_INHERIT.
 *
 * The stream is inherited from the parent process. */
static inline bfu_process_fd bfu_process_fd_inherit() {
    return (bfu_process_fd) { .kind = BFU_PROCESS_FD_INHERIT };
}

/* Spawns a child process and connects its standard streams.
 *
 * The child executes `arguments[0]` with the remaining arguments via
 * execvp.  The argument list is terminated by NULL.
 *
 * Example:
 *     bfu_process p = { .stdin_fd  = bfu_process_fd_pipe(),
 *                       .stdout_fd = bfu_process_fd_pipe(),
 *                       .stderr_fd = bfu_process_fd_inherit() };
 *     bfu_result err = bfu_process_run(&p, "cat");
 *
 * @param p          Process to spawn. Must not be NULL.
 * @param ...        Command and arguments.
 * @return           BFU_SUCCESS on success, BFU_ERROR_NULLPTR if `p` or the
 *                   argument list is NULL, BFU_ERROR_OOM if memory runs out,
 *                   BFU_ERROR_OVERFLOW if too many file descriptors are open,
 *                   BFU_ERROR_NOT_FOUND if the command cannot be found,
 *                   BFU_ERROR_INVALID if a stream kind or fd is bad, or
 *                   BFU_ERROR_ERRNO for any other system call failure.
 */
bfu_result bfu_process_run_(bfu_process *p, ...);
bfu_result bfu_process_run_v(bfu_process *p, char **arguments);
#define bfu_process_run(p, ...) bfu_process_run_((p), __VA_ARGS__, NULL)
#define bfu_process_run_sync(p, ...) ({ \
    i32 _err = bfu_process_run_((p), __VA_ARGS__, NULL); \
    if (_err == BFU_SUCCESS) { \
        _err = bfu_process_wait((p)); \
    } \
    _err; \
})

/* Waits for the child process `p` to exit and returns its exit status.
 *
 * If `p`'s stdin is a pipe, its write end is closed first, so the child sees
 * EOF on standard input.  The caller should drain the child's stdout/stderr
 * pipes with `bfu_process_fd_read` before calling this function, otherwise a
 * child that produces a lot of output may block forever filling a pipe
 * buffer.
 *
 * @param p  Process to wait for. Must not be NULL.
 * @return   The child's exit status (WEXITSTATUS), or the signal that
 *           terminated or stopped it (WTERMSIG/WSTOPSIG), or
 *           BFU_ERROR_INVALID on waitpid failure.
 */
i8 bfu_process_wait(bfu_process *p);

/* Closes the standard stream file descriptors owned by the parent side of a
 * child process `p`.
 *
 * For each of stdin, stdout, stderr, the descriptor is closed when its kind is
 * BFU_PROCESS_FD_PIPE (a pipe created by `bfu_process_run`) or
 * BFU_PROCESS_FD_FILE (a descriptor supplied by the caller). Streams of kind
 * BFU_PROCESS_FD_NONE and BFU_PROCESS_FD_INHERIT don't need closing
 *
 * After calling this function, the descriptors in `p` should not be used
 * again.
 *
 * @param p  Process whose standard stream descriptors are closed. May be NULL,
 *           in which case the function returns BFU_ERROR_NULLPTR without doing
 *           anything.
 * @return   BFU_SUCCESS on success, or BFU_ERROR_NULLPTR if `p` is NULL.
 */
bfu_result bfu_process_close(bfu_process *p);

/* Reads all output from a pipe into `s` until EOF.
 *
 * `fd` must be a pipe owned by the parent (kind BFU_PROCESS_FD_PIPE), as set
 * up by `bfu_process_run` for a stdout/stderr stream.
 *
 * @param s   String to append the read data to. Must not be NULL.
 * @param fd  Pipe to read from. Must be of kind BFU_PROCESS_FD_PIPE.
 * @return    BFU_SUCCESS on success (EOF reached), BFU_ERROR_NULLPTR if `s`
 *            is NULL, BFU_ERROR_INVALID if `fd` is not a pipe or is closed,
 *                   BFU_ERROR_OOM if appending to `s` runs out of memory, or
 *            BFU_ERROR_ERRNO on any other read failure.
 */
bfu_result bfu_process_fd_read(bfu_string *s, bfu_process_fd fd);

/* Writes the contents of `s` to a pipe until fully written.
 *
 * `fd` must be a pipe owned by the parent (kind BFU_PROCESS_FD_PIPE), as set
 * up by `bfu_process_run` for a stdin stream.
 *
 * @param fd  Pipe to write to. Must be of kind BFU_PROCESS_FD_PIPE.
 * @param s   String whose contents are written. Must not be NULL.
 * @return    BFU_SUCCESS on success, BFU_ERROR_NULLPTR if `s` is NULL,
 *            BFU_ERROR_INVALID if `fd` is not a pipe or is closed, or
 *            BFU_ERROR_ERRNO on any other write failure.
 */
bfu_result bfu_process_fd_write(bfu_process_fd fd, bfu_string *s);

