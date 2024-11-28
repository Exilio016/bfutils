/* bfutils_process.h

DESCRIPTION: 

    This is a single-header-file library that provides utility funtions to work with process.

USAGE:
    
    In one source file put:
        #define BFUTILS_PROCESS_IMPLEMENTATION
        #include "bfutils_process.h"
    
    Other source files should contain only the import line.

    Functions (macros):
    
        sync_process:
        int process_sync(char *const *cmd, char *in, char **out, char **err); This function starts and waits for the process execution.
            "cmd" needs to be a null-terminated array containing the process and its arguments.
            if "in" is not NULL, its content will be send to the process STDIN. 
            if "out", or "err" is not NULL, a null-terminated string will be placed at *out or *err with the contents of STDOUT and STDERR respectively.
            The caller needs to free *out and *err.
            The return value is the process status code.
    
    Compile-time options:
        
        #define BFUTILS_PROCESS_NO_SHORT_NAME
        
            This flag needs to be set globally.
            By default this file exposes functions without bfutils_ prefix.
            By defining this flag, this library will expose only functions prefixed with bfutils_

        #define BFUTILS_PROCESS_MALLOC another_malloc
        #define BFUTILS_PROCESS_CALLOC another_calloc
        #define BFUTILS_PROCESS_REALLOC another_realloc
        #define BFUTILS_PROCESS_FREE another_free

            These flags needs to be set only in the file containing #define BFUTILS_PROCESS_IMPLEMENTATION
            If you don't want to use 'stdlib.h' memory functions you can define these flags with custom functions.

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

#ifndef BFUTILS_PROCESS_H
#define BFUTILS_PROCESS_H

#define process_sync bfutils_process_sync

#ifndef BFUTILS_PROCESS_NO_SHORT_NAME

#endif //BFUTILS_PROCESS_NO_SHORT_NAME

#if ((defined(BFUTILS_PROCESS_REALLOC) && (!defined(BFUTILS_PROCESS_MALLOC) || !defined(BFUTILS_PROCESS_CALLOC) || !defined(BFUTILS_PROCESS_FREE))) \
    || (defined(BFUTILS_PROCESS_MALLOC) && (!defined(BFUTILS_PROCESS_REALLOC) || !defined(BFUTILS_PROCESS_CALLOC) || !defined(BFUTILS_PROCESS_FREE))) \
    || (defined(BFUTILS_PROCESS_CALLOC) && (!defined(BFUTILS_PROCESS_MALLOC) || !defined(BFUTILS_PROCESS_REALLOC) || !defined(BFUTILS_PROCESS_FREE))) \
    || (defined(BFUTILS_PROCESS_FREE) && (!defined(BFUTILS_PROCESS_MALLOC) || !defined(BFUTILS_PROCESS_REALLOC) || !defined(BFUTILS_PROCESS_CALLOC))))
#error "You must define all BFUTILS_PROCESS_REALLOC, BFUTILS_PROCESS_CALLOC, BFUTILS_PROCESS_MALLOC, BFUTILS_PROCESS_FREE or neither."
#endif

#ifndef BFUTILS_PROCESS_REALLOC
#include <stdlib.h>
#define BFUTILS_PROCESS_REALLOC realloc
#define BFUTILS_PROCESS_CALLOC calloc
#define BFUTILS_PROCESS_MALLOC malloc
#define BFUTILS_PROCESS_FREE free
#endif //BFUTILS_PROCESS_REALLOC

extern int bfutils_process_sync(char *const *cmd, const char *in, char **out, char **err);

#endif // PROCESS_H
#ifdef BFUTILS_PROCESS_IMPLEMENTATION
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

void read_fd(int fd, char **res) {
    char buffer[1024];
    int count = 0;
    int length; 
    *res = NULL;
    do {
        length = read(fd, buffer, 1024);
        *res = (char*) BFUTILS_PROCESS_REALLOC(*res, count + length + 1);
        if (length > 0)
            strncpy(*res + count, buffer, length);
        count += length;
    } while(length > 0);
    (*res)[count] = '\0';
}

void close_pair(int *fd) {
    close(fd[0]);
    close(fd[1]);
}

int bfutils_process_sync(char *const *cmd, const char *in, char **out, char **err) {
    if(cmd == NULL || *cmd == NULL) {
        return -1;
    }
    int stdin_fd[2];
    int stdout_fd[2];
    int stderr_fd[2];

    pipe(stdin_fd);
    pipe(stdout_fd);
    pipe(stderr_fd);

    pid_t pid;
    pid = fork();
    switch (pid) {
        case -1:
            return -1;
        case 0:
            dup2(stdin_fd[0], STDIN_FILENO);
            close_pair(stdin_fd);
            dup2(stdout_fd[1], STDOUT_FILENO);
            close_pair(stdout_fd);
            dup2(stderr_fd[1], STDERR_FILENO);
            close_pair(stderr_fd);
            execvp(cmd[0], cmd);
            break;
        default:
            if (in != NULL) {
                write(stdin_fd[1], in, strlen(in));
            }
            close_pair(stdin_fd);
            
            int status;
            waitpid(pid, &status, 0);
            
            close(stdout_fd[1]);
            if (out != NULL) {
                read_fd(stdout_fd[0], out);
            }
            close(stdout_fd[0]);
            close(stderr_fd[1]);
            if (err != NULL) {
                read_fd(stderr_fd[0], err);
            }
            close(stderr_fd[0]);

            return status;
    }
    return -1;
}

#endif //BFUTILS_PROCESS_IMPLEMENTATION
