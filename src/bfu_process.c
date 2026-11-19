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

#include "bfu_process.h"
#include "bfu_array.h"
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

bfu_result bfu_process_run_(bfu_process *p, ...) {
    bfu_array(char *) args = NULL;
    va_list ap;
    va_start(ap, p);
    char *arg = va_arg(ap, char *);
    while (arg) {
        bfu_array_push(args, arg);
        arg = va_arg(ap, char*);
    }
    bfu_array_push(args, NULL);
    va_end(ap);
    bfu_result err = bfu_process_run_v(p, args);
    bfu_array_delete(args);
    return err;
}

static i32 set_fds_nonblock(int *fds){
    if (fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL) | O_NONBLOCK) == -1){
        return 0;
    }
    if (fcntl(fds[1], F_SETFL, fcntl(fds[1], F_GETFL) | O_NONBLOCK) == -1){
        return 0;
    }
    return 1;
}

static bfu_result errno_to_result(int errnum) {
    switch (errnum) {
        case ENOMEM:
            return BFU_ERROR_OOM;
        case EMFILE:
        case ENFILE:
        case ENOSPC:
            return BFU_ERROR_OVERFLOW;
        case ENOENT:
            return BFU_ERROR_NOT_FOUND;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case EACCES:
        case EPERM:
        case EISDIR:
        case ENOTDIR:
            return BFU_ERROR_INVALID;
        default:
            return BFU_ERROR_ERRNO;
    }
}

static bfu_result prepare_fds(bfu_process_fd fd, i32 null_fd, i32 *fds) {
    switch (fd.kind) {
        case BFU_PROCESS_FD_NONE: 
            fds[0] = dup(null_fd);
            fds[1] = dup(null_fd);
            break;
        case BFU_PROCESS_FD_INHERIT:
            break;
        case BFU_PROCESS_FD_PIPE:
            if (pipe(fds) < 0) {
                return errno_to_result(errno);
            }
            break;
        case BFU_PROCESS_FD_FILE:
            fds[0] = fd.fd;
            fds[1] = fd.fd;
            break;
        default:
            return BFU_ERROR_INVALID;
    }
    return BFU_SUCCESS;
}

static inline void close_pair(int *fd) {
    close(fd[0]);
    close(fd[1]);
}
bfu_result bfu_process_run_v(bfu_process *p, char **arguments) {
    if (p == NULL || arguments == NULL || *arguments == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    bfu_result res = BFU_SUCCESS;

    i32 dev_null = open("/dev/null", O_RDWR);
    if (dev_null < 0) {
        return errno_to_result(errno);
    }
    i32 stdin_fds[2];
    i32 stdout_fds[2];
    i32 stderr_fds[2];

    res = prepare_fds(p->stdin_fd, dev_null, stdin_fds);
    if (res) goto err;
    res = prepare_fds(p->stdout_fd, dev_null, stdout_fds);
    if (res) goto err;
    res = prepare_fds(p->stderr_fd, dev_null, stderr_fds);
    if (res) goto err;
    
    if (p->stdout_fd.kind == BFU_PROCESS_FD_PIPE) {
        if (!set_fds_nonblock(stdout_fds)) {
            return errno_to_result(errno);
        }
    }
    if (p->stderr_fd.kind == BFU_PROCESS_FD_PIPE) {
        if (!set_fds_nonblock(stderr_fds)) {
            return errno_to_result(errno);
        }
    }

    p->pid = fork();
    if (p->pid < 0) {
        res = errno_to_result(errno);
        goto err;
    }
    if (p->pid == 0) { //child
        if (p->stdin_fd.kind != BFU_PROCESS_FD_INHERIT) {
            if (dup2(stdin_fds[0], STDIN_FILENO) < 0) {
                perror("dup2");
                exit(1);
            }
            close_pair(stdin_fds);
        }

        if (p->stdout_fd.kind != BFU_PROCESS_FD_INHERIT) {
            if (dup2(stdout_fds[1], STDOUT_FILENO) < 0) {
                perror("dup2");
                exit(1);
            }
            close_pair(stdout_fds);
        }

        if (p->stderr_fd.kind != BFU_PROCESS_FD_INHERIT) {
            if (dup2(stderr_fds[1], STDERR_FILENO) < 0) {
                perror("dup2");
                exit(1);
            }
            close_pair(stderr_fds);
        }
        close(dev_null);

        execvp(arguments[0], arguments);
    }

    if (p->stdin_fd.kind == BFU_PROCESS_FD_PIPE) {
        p->stdin_fd.fd = stdin_fds[1];
        close(stdin_fds[0]);
    }
    if (p->stdout_fd.kind == BFU_PROCESS_FD_PIPE) {
        p->stdout_fd.fd = stdout_fds[0];
        close(stdout_fds[1]);
    }
    if (p->stderr_fd.kind == BFU_PROCESS_FD_PIPE) {
        p->stderr_fd.fd = stderr_fds[0];
        close(stderr_fds[1]);
    }

    if (p->stdin_fd.kind == BFU_PROCESS_FD_NONE) {
        close_pair(stdin_fds);
    }
    if (p->stdout_fd.kind == BFU_PROCESS_FD_NONE) {
        close_pair(stdout_fds);
    }
    if (p->stderr_fd.kind == BFU_PROCESS_FD_NONE) {
        close_pair(stderr_fds);
    }

    close(dev_null);
    return res;
err:
    close(dev_null);
    close_pair(stdin_fds);
    close_pair(stdout_fds);
    close_pair(stderr_fds);
    return res;
}

i8 bfu_process_wait(bfu_process *p) {
    int status;

    if (p->stdin_fd.kind == BFU_PROCESS_FD_PIPE) {
        close(p->stdin_fd.fd);
    }

    while (true) {
        i32 wpid = waitpid(p->pid, &status, 0);
        if (wpid < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            return BFU_ERROR_INVALID;
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status)) {
            return WTERMSIG(status);
        }
        else if (WIFSTOPPED(status)) {
            return WSTOPSIG(status);
        }
        return status;
    }
}

bfu_result bfu_process_fd_read(bfu_string *s, bfu_process_fd fd) {
    if (s == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    if (fd.kind != BFU_PROCESS_FD_PIPE) {
        return BFU_ERROR_INVALID;
    }
    char buf[BUFSIZ];
    while (true) {
        i32 n = read(fd.fd, buf, BUFSIZ);
        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return errno_to_result(errno);
        }
        if (n == 0) {
            return BFU_SUCCESS;
        }
        bfu_result err = bfu_string_push(s, buf, n);
        if (err) return err;
    }
}

bfu_result bfu_process_fd_write(bfu_process_fd fd, bfu_string *s) {
    if (s == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    if (fd.kind != BFU_PROCESS_FD_PIPE) {
        return BFU_ERROR_INVALID;
    }
    u64 remaining = s->size;
    u64 idx = 0;
    while (true) {
        i32 n = write(fd.fd, s->data + idx, remaining);
        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return errno_to_result(errno);
        }
        remaining -= n;
        idx += n;
        if (remaining == 0) {
            return BFU_SUCCESS;
        }
    }
}

bfu_result bfu_process_close(bfu_process *p) {
    if (p == NULL) {
        return BFU_ERROR_NULLPTR;
    }
    if (p->stdin_fd.kind == BFU_PROCESS_FD_PIPE || p->stdin_fd.kind == BFU_PROCESS_FD_FILE) {
        close(p->stdin_fd.fd);
    }
    if (p->stdout_fd.kind == BFU_PROCESS_FD_PIPE || p->stdout_fd.kind == BFU_PROCESS_FD_FILE) {
        close(p->stdout_fd.fd);
    }
    if (p->stderr_fd.kind == BFU_PROCESS_FD_PIPE || p->stderr_fd.kind == BFU_PROCESS_FD_FILE) {
        close(p->stderr_fd.fd);
    }
    return BFU_SUCCESS;
}
