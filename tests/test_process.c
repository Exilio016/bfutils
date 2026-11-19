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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bfu.h"

/* ------------------------------------------------------------------ */
/* bfu_process_run / bfu_process_run_v argument validation            */
/* ------------------------------------------------------------------ */

static void case_run_null_process(void) {
    TEST_ASSERT(bfu_process_run(NULL, "echo", "x") == BFU_ERROR_NULLPTR);
}

static void case_run_null_argument(void) {
    bfu_process p = {0};

    TEST_ASSERT(bfu_process_run(&p, NULL) == BFU_ERROR_NULLPTR);
}

static void case_run_v_null_process(void) {
    char *arguments[] = {"echo", "x", NULL};

    TEST_ASSERT(bfu_process_run_v(NULL, arguments) == BFU_ERROR_NULLPTR);
}

static void case_run_v_null_arguments(void) {
    bfu_process p = {0};

    TEST_ASSERT(bfu_process_run_v(&p, NULL) == BFU_ERROR_NULLPTR);
}

static void case_run_v_empty_arguments(void) {
    bfu_process p = {0};

    TEST_ASSERT(bfu_process_run_v(&p, (char *[]){NULL}) == BFU_ERROR_NULLPTR);
}

static void case_run_fd_inherit(void) {
    bfu_process p = {
        .stdin_fd  = bfu_process_fd_inherit(),
        .stdout_fd = bfu_process_fd_pipe(),
        .stderr_fd = bfu_process_fd_none(),
    };

    TEST_ASSERT(bfu_process_run(&p, "echo", "inherit stdin") == BFU_SUCCESS);

    bfu_string_f out = {0};
    TEST_ASSERT(bfu_process_fd_read(&out, p.stdout_fd) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&out, "inherit stdin\n");

    TEST_ASSERT(bfu_process_wait(&p) == 0);
}

static void case_run_fd_file(void) {
    /* BFU_PROCESS_FD_FILE: stdout goes straight to a real file descriptor. */
    const char *path = "/tmp/bfu_process_test_file.txt";
    unlink(path);

    /* Pre-create the file: from_file() passes flags straight to open(2),
       which requires a mode argument when O_CREAT is used. */
    FILE *pre = fopen(path, "w");
    TEST_ASSERT(pre != NULL);
    fclose(pre);

    bfu_process p = {
        .stdout_fd = bfu_process_fd_from_file((char *)path, O_WRONLY | O_TRUNC),
        .stderr_fd = bfu_process_fd_none(),
    };

    TEST_ASSERT(bfu_process_run(&p, "echo", "file output") == BFU_SUCCESS);
    TEST_ASSERT(bfu_process_wait(&p) == 0);

    FILE *f = fopen(path, "r");
    TEST_ASSERT(f != NULL);
    char buf[64];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    unlink(path);
    TEST_ASSERT(strstr(buf, "file output") != NULL);
}

static void case_run_invalid_fd_kind(void) {
    /* An fd kind outside the valid range hits prepare_fds() default case. */
    bfu_process p = {
        .stdout_fd = bfu_process_fd_pipe(),
        .stderr_fd = { 99, -1 },
    };

    TEST_ASSERT(bfu_process_run(&p, "echo", "x") == BFU_ERROR_INVALID);
}

static void case_wait_signaled(void) {
    bfu_process p = {
        .stdout_fd = bfu_process_fd_none(),
        .stderr_fd = bfu_process_fd_none(),
    };

    /* The child kills itself with SIGKILL; wait() reports the signal. */
    TEST_ASSERT(bfu_process_run(&p, "sh", "-c", "kill -9 $$") == BFU_SUCCESS);

    i8 status = bfu_process_wait(&p);
    TEST_ASSERT(status != 0);
}

/* ------------------------------------------------------------------ */
/* Spawning a simple command and reading its output                   */
/* ------------------------------------------------------------------ */

static void case_run_echo(void) {
    bfu_process p = {
        .stdout_fd = bfu_process_fd_pipe(),
        .stderr_fd = bfu_process_fd_none(),
    };

    TEST_ASSERT(bfu_process_run(&p, "echo", "hello world") == BFU_SUCCESS);

    bfu_string_f out = {0};
    TEST_ASSERT(bfu_process_fd_read(&out, p.stdout_fd) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&out, "hello world\n");

    TEST_ASSERT(bfu_process_wait(&p) == 0);
}

static void case_run_true_no_output(void) {
    bfu_process p = {
        .stdout_fd = bfu_process_fd_pipe(),
        .stderr_fd = bfu_process_fd_none(),
    };

    TEST_ASSERT(bfu_process_run(&p, "true") == BFU_SUCCESS);

    bfu_string_f out = {0};
    TEST_ASSERT(bfu_process_fd_read(&out, p.stdout_fd) == BFU_SUCCESS);
    TEST_ASSERT(out.size == 0);

    TEST_ASSERT(bfu_process_wait(&p) == 0);
}

static void case_run_unknown_command(void) {
    bfu_process p = {
        .stdout_fd = bfu_process_fd_pipe(),
        .stderr_fd = bfu_process_fd_none(),
    };

    /* The command cannot be exec'd; run() still forks successfully and wait()
       reports the child's non-zero exit status. */
    TEST_ASSERT(bfu_process_run(&p, "this_command_does_not_exist_xyz")
                == BFU_SUCCESS);

    bfu_string_f out = {0};
    TEST_ASSERT(bfu_process_fd_read(&out, p.stdout_fd) == BFU_SUCCESS);

    TEST_ASSERT(bfu_process_wait(&p) != 0);
}

/* ------------------------------------------------------------------ */
/* Writing to stdin and reading from stdout                           */
/* ------------------------------------------------------------------ */

static void case_run_cat_stdin_stdout(void) {
    bfu_process p = {
        .stdin_fd  = bfu_process_fd_pipe(),
        .stdout_fd = bfu_process_fd_pipe(),
        .stderr_fd = bfu_process_fd_none(),
    };

    TEST_ASSERT(bfu_process_run(&p, "cat") == BFU_SUCCESS);

    bfu_string_f input = {0};
    TEST_ASSERT(bfu_string_push_cstr(&input, "some input\n") == BFU_SUCCESS);
    TEST_ASSERT(bfu_process_fd_write(p.stdin_fd, &input) == BFU_SUCCESS);

    /* Closing the write end signals EOF to the child. */
    close(p.stdin_fd.fd);

    bfu_string_f out = {0};
    TEST_ASSERT(bfu_process_fd_read(&out, p.stdout_fd) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&out, "some input\n");

    TEST_ASSERT(bfu_process_wait(&p) == 0);
}

/* ------------------------------------------------------------------ */
/* bfu_process_fd_read / bfu_process_fd_write on a raw pipe           */
/* ------------------------------------------------------------------ */

static void case_fd_write_then_read(void) {
    int fds[2];
    TEST_ASSERT(pipe(fds) == 0);

    bfu_process_fd write_fd = { BFU_PROCESS_FD_PIPE, fds[1] };
    bfu_process_fd read_fd  = { BFU_PROCESS_FD_PIPE, fds[0] };

    bfu_string_f s = {0};
    TEST_ASSERT(bfu_string_push(&s, "pipe data") == BFU_SUCCESS);
    TEST_ASSERT(bfu_process_fd_write(write_fd, &s) == BFU_SUCCESS);

    close(fds[1]);

    bfu_string_f out = {0};
    TEST_ASSERT(bfu_process_fd_read(&out, read_fd) == BFU_SUCCESS);
    TEST_ASSERT_BFU_STR(&out, "pipe data");

    close(fds[0]);
}

/* ------------------------------------------------------------------ */
/* bfu_process_fd_read argument validation                            */
/* ------------------------------------------------------------------ */

static void case_fd_read_null_string(void) {
    TEST_ASSERT(bfu_process_fd_read(NULL, bfu_process_fd_pipe())
                == BFU_ERROR_NULLPTR);
}

static void case_fd_read_not_a_pipe(void) {
    bfu_string_f s = {0};

    TEST_ASSERT(bfu_process_fd_read(&s, bfu_process_fd_none())
                == BFU_ERROR_INVALID);
    TEST_ASSERT(bfu_process_fd_read(&s, bfu_process_fd_inherit())
                == BFU_ERROR_INVALID);
    TEST_ASSERT(bfu_process_fd_read(&s, bfu_process_fd_from_file("/dev/null",
                                                                  O_RDONLY))
                == BFU_ERROR_INVALID);
}

/* ------------------------------------------------------------------ */
/* bfu_process_fd_write argument validation                           */
/* ------------------------------------------------------------------ */

static void case_fd_write_null_string(void) {
    TEST_ASSERT(bfu_process_fd_write(bfu_process_fd_pipe(), NULL)
                == BFU_ERROR_NULLPTR);
}

static void case_fd_write_not_a_pipe(void) {
    bfu_string_f s = {0};
    TEST_ASSERT(bfu_string_push_cstr(&s, "x") == BFU_SUCCESS);

    TEST_ASSERT(bfu_process_fd_write(bfu_process_fd_none(), &s)
                == BFU_ERROR_INVALID);
    TEST_ASSERT(bfu_process_fd_write(bfu_process_fd_inherit(), &s)
                == BFU_ERROR_INVALID);
}

/* ------------------------------------------------------------------ */
/* System call errors mapped to semantic errors                       */
/* ------------------------------------------------------------------ */

static void case_fd_read_ebadf_maps_to_invalid(void) {
    /* Reading from a closed pipe yields EBADF, which now maps to
       BFU_ERROR_INVALID instead of the generic BFU_ERROR_ERRNO. */
    int fds[2];
    TEST_ASSERT(pipe(fds) == 0);
    close(fds[0]);

    bfu_process_fd read_fd = { BFU_PROCESS_FD_PIPE, fds[0] };
    bfu_string_f s = {0};

    TEST_ASSERT(bfu_process_fd_read(&s, read_fd) == BFU_ERROR_INVALID);

    close(fds[1]);
}

static void case_fd_write_ebadf_maps_to_invalid(void) {
    /* Writing to a closed pipe yields EBADF, which now maps to
       BFU_ERROR_INVALID instead of the generic BFU_ERROR_ERRNO. */
    int fds[2];
    TEST_ASSERT(pipe(fds) == 0);
    close(fds[1]);

    bfu_process_fd write_fd = { BFU_PROCESS_FD_PIPE, fds[1] };
    bfu_string_f s = {0};
    TEST_ASSERT(bfu_string_push_cstr(&s, "x") == BFU_SUCCESS);

    TEST_ASSERT(bfu_process_fd_write(write_fd, &s) == BFU_ERROR_INVALID);

    close(fds[0]);
}

/* ------------------------------------------------------------------ */
/* bfu_process_close                                                  */
/* ------------------------------------------------------------------ */

static void case_close_null(void) {
    TEST_ASSERT(bfu_process_close(NULL) == BFU_ERROR_NULLPTR);
}

static void case_close_pipe_fds(void) {
    int fds[2];
    TEST_ASSERT(pipe(fds) == 0);

    bfu_process p = {
        .stdin_fd  = { BFU_PROCESS_FD_PIPE, fds[1] },
        .stdout_fd = { BFU_PROCESS_FD_PIPE, fds[0] },
        .stderr_fd = { BFU_PROCESS_FD_PIPE, fds[1] },
    };

    TEST_ASSERT(bfu_process_close(&p) == BFU_SUCCESS);

    /* All pipe ends should now be closed: reading from them fails with EBADF. */
    char buf[1];
    TEST_ASSERT(read(fds[0], buf, 1) < 0);
    TEST_ASSERT(errno == EBADF);
    TEST_ASSERT(read(fds[1], buf, 1) < 0);
    TEST_ASSERT(errno == EBADF);
}

static void case_close_file_fds(void) {
    const char *path = "/tmp/bfu_process_test_close_file.txt";
    FILE *pre = fopen(path, "w");
    TEST_ASSERT(pre != NULL);
    fclose(pre);

    int fd = open(path, O_RDONLY);
    TEST_ASSERT(fd >= 0);

    bfu_process p = {
        .stdin_fd  = { BFU_PROCESS_FD_FILE, fd },
        .stdout_fd = { BFU_PROCESS_FD_FILE, fd },
        .stderr_fd = { BFU_PROCESS_FD_FILE, fd },
    };

    TEST_ASSERT(bfu_process_close(&p) == BFU_SUCCESS);

    /* The fd should now be closed. */
    TEST_ASSERT(close(fd) < 0);

    unlink(path);
}

static void case_close_none_inherit_fds(void) {
    /* NONE and INHERIT fds are not closed by bfu_process_close. */
    int fd = open("/dev/null", O_RDONLY);
    TEST_ASSERT(fd >= 0);

    bfu_process p = {
        .stdin_fd  = { BFU_PROCESS_FD_NONE, -1 },
        .stdout_fd = { BFU_PROCESS_FD_INHERIT, fd },
        .stderr_fd = { BFU_PROCESS_FD_NONE, -1 },
    };

    TEST_ASSERT(bfu_process_close(&p) == BFU_SUCCESS);

    /* INHERIT fd is left open. */
    TEST_ASSERT(fcntl(fd, F_GETFD) >= 0);
    close(fd);
}

static void case_close_after_run_and_wait(void) {
    bfu_process p = {
        .stdout_fd = bfu_process_fd_pipe(),
        .stderr_fd = bfu_process_fd_none(),
    };

    TEST_ASSERT(bfu_process_run(&p, "true") == BFU_SUCCESS);
    TEST_ASSERT(bfu_process_wait(&p) == 0);

    TEST_ASSERT(bfu_process_close(&p) == BFU_SUCCESS);

    /* The stdout pipe read end should now be closed: read fails with EBADF. */
    char buf[1];
    TEST_ASSERT(read(p.stdout_fd.fd, buf, 1) < 0);
    TEST_ASSERT(errno == EBADF);
}

/* ------------------------------------------------------------------ */

void test_process_suite(void) {
    printf("\n[bfu_process]\n");

    TEST_RUN(case_run_null_process);
    TEST_RUN(case_run_null_argument);
    TEST_RUN(case_run_v_null_process);
    TEST_RUN(case_run_v_null_arguments);
    TEST_RUN(case_run_v_empty_arguments);
    TEST_RUN(case_run_fd_inherit);
    TEST_RUN(case_run_fd_file);
    TEST_RUN(case_run_invalid_fd_kind);

    TEST_RUN(case_run_echo);
    TEST_RUN(case_run_true_no_output);
    TEST_RUN(case_run_unknown_command);
    TEST_RUN(case_run_cat_stdin_stdout);
    TEST_RUN(case_wait_signaled);

    TEST_RUN(case_fd_write_then_read);

    TEST_RUN(case_fd_read_null_string);
    TEST_RUN(case_fd_read_not_a_pipe);
    TEST_RUN(case_fd_read_ebadf_maps_to_invalid);
    TEST_RUN(case_fd_write_null_string);
    TEST_RUN(case_fd_write_not_a_pipe);
    TEST_RUN(case_fd_write_ebadf_maps_to_invalid);

    TEST_RUN(case_close_null);
    TEST_RUN(case_close_pipe_fds);
    TEST_RUN(case_close_file_fds);
    TEST_RUN(case_close_none_inherit_fds);
    TEST_RUN(case_close_after_run_and_wait);
}
