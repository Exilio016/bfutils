/*
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

#ifndef BFUTILS_BUILD_H
#define BFUTILS_BUILD_H

typedef struct {
    char *name;
    char **files;
    int files_len;
    char *cflags;
    char *libs;
}BFUtilsBuildExecutable;

#define bfutils_add_executable(cfg) bfutils_add_executable_fn(cfg, __FILE__, __LINE__);
void bfutils_build(int argc, char *argv[]);
void bfutils_add_executable_fn(BFUtilsBuildExecutable cfg, char *file, int line);

#endif //BFUTILS_BUILD_H
#ifdef BFUTILS_BUILD_IMPLEMENTATION
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

static FILE *bfutils_build_fp = NULL;
static char *bfutils_build_cflags = "";
static char *bfutils_build_libs = "";
int main(int argc, char *argv[]) {
    if (mkdir("target", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }
    if (mkdir("target/bin", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }
    if (mkdir("target/objs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
        perror("mkdir");
        exit(1);
    }
    bfutils_build_fp = fopen("target/build.ninja", "w");
    if (bfutils_build_fp == NULL) {
        perror("fopen");
        exit(2);
    }
    FILE *fp = fopen("target/stage1.ninja", "w");
    if (fp == NULL) {
        perror("fopen");
        exit(2);
    }
    fprintf(fp, "cflags = %s\n", bfutils_build_cflags);
    fprintf(fp, "libs = %s\n", bfutils_build_libs);
    fprintf(fp, "rule cc\n command = gcc $cflags -MD -MF target/$out.d $in -o $out\n depfile = target/$out.d\n");
    fprintf(fp, "rule cc2\n command = gcc -DSTAGE2 $cflags -MD -MF $out.d $in -o $out\n depfile = $out.d\n");
    fprintf(fp, "rule rebuild\n command = target/build\n");
    fprintf(fp, "build build: cc build.c\n");
    fprintf(fp, "build target/build: cc2 build.c || build\n");
    fprintf(fp, "build stage2: rebuild || target/build\n");
    fclose(fp);
    #ifndef STAGE2
    if (execlp("ninja", "ninja", "-f", "./target/stage1.ninja", NULL) < 0) {
        perror("execlp");
        exit(5);
    }
    #endif //STAGE2

    fprintf(bfutils_build_fp, "cflags = %s\n", bfutils_build_cflags);
    fprintf(bfutils_build_fp, "libs = %s\n", bfutils_build_libs);
    fprintf(bfutils_build_fp, "rule cc\n command = gcc $cflags -MD -MF $out.d -c $in -o $out\n depfile = $out.d\n");
    fprintf(bfutils_build_fp, "rule link\n command = gcc $in $libs -o $out\n");
    bfutils_build(argc, argv);
    fclose(bfutils_build_fp);
    bfutils_build_fp = NULL;
    if (execlp("ninja", "ninja", "-f", "target/build.ninja", NULL) < 0) {
        perror("execlp");
        exit(5);
    }
}

char *bfutils_get_file_object(char *filename) {
    int l = strlen(filename);
    char *res = malloc((l+1) * sizeof(char));
    strncpy(res, filename, l);
    //TODO: validate if string has length to add .o
    char *ext = strrchr(res, '.');
    ext[1] = 'o';
    ext[2] = '\0';
    return res;
}

void bfutils_add_executable_fn(BFUtilsBuildExecutable cfg, char *file, int line) {
    if (bfutils_build_fp == NULL) {
        fprintf(stderr, "Error on %s:%d - bfutils_add_executable must be called inside bfutils_build function", file, line);
        exit(3);
    }
    if (cfg.name == NULL || strlen(cfg.name) == 0) {
        fprintf(stderr, "Error on %s:%d - An executable must have a valid name", file, line);
        exit(4);
    }
    if (cfg.files_len <= 0) {
        fprintf(stderr, "Error on %s:%d - An executable must have source files", file, line);
        exit(4);
    }
    char **objs = malloc(sizeof(char *) * cfg.files_len);
    for (int i = 0; i < cfg.files_len; i++) {
        char *file = basename(cfg.files[i]);
        char *obj = bfutils_get_file_object(file);
        objs[i] = obj;
        fprintf(bfutils_build_fp, "build target/objs/%s: cc %s\n", obj, cfg.files[i]);
        if (cfg.cflags) {
            fprintf(bfutils_build_fp, " cflags = %s\n", cfg.cflags);
        }
    }
    fprintf(bfutils_build_fp, "build target/bin/%s: link", cfg.name);
    for (int i = 0; i < cfg.files_len; i++) {
        fprintf(bfutils_build_fp, " target/objs/%s", objs[i]);
        free(objs[i]);
    }
    free(objs);
    fprintf(bfutils_build_fp, "\n");
    if (cfg.libs) {
        fprintf(bfutils_build_fp, " libs = %s\n", cfg.libs);
    }
}
#endif //BFUTILS_BUILD_IMPLEMENTATION
