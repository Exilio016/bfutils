/*bfutils_build
DESCRIPTION: 

    This is a single-header-file library that can be used as a build system for your c project.
    It generates build.ninja files and execute ninja to compile it.

DEPENDENCIES:
    
    - ninja (https://ninja-build.org/)
    - pkg-config (optional)

USAGE:
    
    To use the bfutils_build system you need to create a file named "build.c" on the project root directory.
    In the "build.c" include the following lines:
        #define BFUTILS_BUILD_IMPLEMENTATION
        #include "bfutils_build.h"
    
        void bfutils_build(int argc, char *argv[]) {
            //Your build configuration goes here...
        }

    To build your project you need to compile once the "build.c" to an executable named "build" as below:
        cc -o build build.c

    Then you just need to execute "./build". 
    "build.c" needs to be compiled only once, as it can rebuild itself before building your project.
    
    Functions (macros):
        
        bfutils_add_executable:
            void bfutils_add_executable(BFUtilsBuildCfg cfg); This function needs to be called inside bfutils_build.
            It defines an new compilation target for your project.
            It will compile an executable on target/bin with the name defined in "cfg.name".
            Each file in "cfg.files" will be compiled to a ".o" file on target/objs.
            If BFUTILS_BUILD_CFLAGS is defined it will be included on the compilation command for the ".o" files.
            If "cfg.cflags" is not NULL, it will be used instead of BFUTILS_BUILD_CFLAGS.
            If BFUTILS_BUILD_LDFLAGS is defined, it will be included on the link command for the executable file.
            If "cfg.ldflags" is not NULL, it will be used instead of BFUTILS_BUILD_LDFLAGS.

        bfutils_add_shared_library:
            void bfutils_add_shared_library(BFUtilsBuildCfg); This function needs to be called inside bfutils_build.
            It defines an new compilation target for your project.
            It will compile a shared library on target/lib with the name "lib${cfg.name}.so".
            Each file in "cfg.files" will be compiled to a ".o" file on target/objs.
            If BFUTILS_BUILD_CFLAGS is defined it will be included on the compilation command for the ".o" files.
            If "cfg.cflags" is not NULL, it will be used instead of BFUTILS_BUILD_CFLAGS.
            If BFUTILS_BUILD_LDFLAGS is defined, it will be included on the link command for the shared library.
            If "cfg.ldflags" is not NULL, it will be used instead of BFUTILS_BUILD_LDFLAGS.

    Compile-time options:
        
        #define BFUTILS_BUILD_CFLAGS cflags
        #define BFUTILS_BUILD_LDFLAGS ldflags

        These flags needs to be defined before the #include "bfutils_build.h".
        These flags sets the default CFLAGS and LDFLAGS used for compiling and linking your project.

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

#ifndef BFUTILS_BUILD_CFLAGS
#define BFUTILS_BUILD_CFLAGS ""
#endif
#ifndef BFUTILS_BUILD_LDFLAGS
#define BFUTILS_BUILD_LDFLAGS ""
#endif

typedef struct {
    char *name;
    char **files;
    int files_len;
    char **deps;
    int deps_len;
    char *cflags;
    char *ldflags;
}BFUtilsBuildCfg;

enum BFUtilsBuildError {
    BFUTILS_BUILD_ERROR_MKDIR,
    BFUTILS_BUILD_ERROR_OPEN,
    BFUTILS_BUILD_ERROR_EXEC,
    BFUTILS_BUILD_ERROR_OUTSIDE_FUNCTION,
    BFUTILS_BUILD_ERROR_MISSING_NAME,
    BFUTILS_BUILD_ERROR_MISSING_FILE,
    BFUTILS_BUILD_ERROR_INVALID_FILENAME,
    BFUTILS_BUILD_ERROR_PKG_CONFIG,
};

#define bfutils_add_executable(cfg) bfutils_add_executable_fn(cfg, __FILE__, __LINE__);
#define bfutils_add_shared_library(cfg) bfutils_add_shared_library_fn(cfg, __FILE__, __LINE__);
void bfutils_build(int argc, char *argv[]);
void bfutils_add_executable_fn(BFUtilsBuildCfg cfg, char *file, int line);
void bfutils_add_shared_library_fn(BFUtilsBuildCfg cfg, char *file, int line);

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

extern char **environ;
static FILE *bfutils_build_fp = NULL;
static char* bfutils_build_source_files[255];
static int bfutils_build_source_files_len = 0;
int main(int argc, char *argv[]) {
    char cc[255] = "gcc";
    char **env = environ;
    while (*env) {
        if (strncmp(*env, "CC=", 3) == 0) {
            strncpy(cc, (*env) + 3, 254);
            break;
        }
        env++;
    }

    if (mkdir("target", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
        perror("mkdir");
        exit(BFUTILS_BUILD_ERROR_MKDIR);
    }
    if (mkdir("target/bin", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
        perror("mkdir");
        exit(BFUTILS_BUILD_ERROR_MKDIR);
    }
    if (mkdir("target/objs", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0 && errno != EEXIST) {
        perror("mkdir");
        exit(BFUTILS_BUILD_ERROR_MKDIR);
    }
    bfutils_build_fp = fopen("target/build.ninja", "w");
    if (bfutils_build_fp == NULL) {
        perror("fopen");
        exit(BFUTILS_BUILD_ERROR_OPEN);
    }
    FILE *fp = fopen("target/stage1.ninja", "w");
    if (fp == NULL) {
        perror("fopen");
        exit(BFUTILS_BUILD_ERROR_OPEN);
    }
    fprintf(fp, "builddir = target\n");
    fprintf(fp, "cflags = %s\n", BFUTILS_BUILD_CFLAGS);
    fprintf(fp, "ldflags = %s\n", BFUTILS_BUILD_LDFLAGS);
    fprintf(fp, "rule cc\n command = %s $cflags -MD -MF target/$out.d $in -o $out\n depfile = target/$out.d\n", cc);
    fprintf(fp, "rule cc2\n command = %s -DSTAGE2 $cflags -MD -MF $out.d $in -o $out\n depfile = $out.d\n", cc);
    fprintf(fp, "rule rebuild\n command = target/build\n");
    fprintf(fp, "build build: cc build.c\n");
    fprintf(fp, "build target/build: cc2 build.c || build\n");
    fprintf(fp, "build stage2: rebuild || target/build\n");
    fclose(fp);
    #ifndef STAGE2
    if (execlp("ninja", "ninja", "-f", "./target/stage1.ninja", NULL) < 0) {
        perror("execlp");
        exit(BFUTILS_BUILD_ERROR_EXEC);
    }
    #endif //STAGE2

    fprintf(bfutils_build_fp, "builddir = target\n");
    fprintf(bfutils_build_fp, "cflags = %s\n", BFUTILS_BUILD_CFLAGS);
    fprintf(bfutils_build_fp, "ldflags = %s\n", BFUTILS_BUILD_LDFLAGS);
    fprintf(bfutils_build_fp, "rule cc\n command = %s $cflags -MD -MF $out.d -c $in -o $out\n depfile = $out.d\n", cc);
    fprintf(bfutils_build_fp, "rule link\n command = %s $in $ldflags -o $out\n", cc);
    fprintf(bfutils_build_fp, "rule lib\n command = %s -shared $in $ldflags -o $out\n", cc);
    bfutils_build(argc, argv);
    fclose(bfutils_build_fp);
    bfutils_build_fp = NULL;
    if (execlp("ninja", "ninja", "-f", "target/build.ninja", NULL) < 0) {
        perror("execlp");
        exit(BFUTILS_BUILD_ERROR_EXEC);
    }
}

char *bfutils_get_file_object(char *filename) {
    int l = strlen(filename);
    char *res = malloc((l+1) * sizeof(char));
    strncpy(res, filename, l+1);
    char *ext = strrchr(res, '.');
    if (ext == NULL || strlen(ext) < 2) {
        fprintf(stderr, "Invalid source file name <%s>\n", filename);
        exit(BFUTILS_BUILD_ERROR_INVALID_FILENAME);
    }
    ext[1] = 'o';
    ext[2] = '\0';
    return res;
}

static int bfutils_build_check_duplicate(char *file) {
    for (int i = 0; i < bfutils_build_source_files_len; i++) {
        if (strcmp(file, bfutils_build_source_files[i]) == 0) {
            return 1;
        }
    }
    bfutils_build_source_files[bfutils_build_source_files_len++] = file;
    return 0;
}

static char *bfutils_pkg_config_cflags(char *dep) {
    if(dep == NULL) {
        return NULL;
    }

    char cmd[strlen(dep) + 21];
    sprintf(cmd, "pkg-config --cflags %s", dep);
    FILE *p = popen(cmd, "r");
    
    char buffer[1024];

    char *res = NULL;
    size_t i = 0;
    size_t n;
    do {
        n = fread(buffer, sizeof(char), 1024, p);
        res = realloc(res, sizeof(char) * (i + n + 1));
        strncpy(res + i, buffer, n);
        i += n;
    } while(n == 1024);
    if (pclose(p) != 0) {
        free(res);
        exit(BFUTILS_BUILD_ERROR_PKG_CONFIG);
    }
    res[i -1] = '\0';
    return res;
}

static char *bfutils_pkg_config_ldflags(char *dep) {
    if(dep == NULL) {
        return NULL;
    }

    char cmd[strlen(dep) + 21];
    sprintf(cmd, "pkg-config --libs %s", dep);
    FILE *p = popen(cmd, "r");
    
    char buffer[1024];

    char *res = NULL;
    size_t i = 0;
    size_t n;
    do {
        n = fread(buffer, sizeof(char), 1024, p);
        res = realloc(res, sizeof(char) * (i + n + 1));
        strncpy(res + i, buffer, n);
        i += n;
    } while(n == 1024);
    if (pclose(p) != 0) {
        free(res);
        exit(BFUTILS_BUILD_ERROR_PKG_CONFIG);
    }
    res[i - 1] = '\0';
    return res;
}

void bfutils_add_shared_library_fn(BFUtilsBuildCfg cfg, char *file, int line) {
    if (bfutils_build_fp == NULL) {
        fprintf(stderr, "Error on %s:%d - bfutils_add_executable must be called inside bfutils_build function\n", file, line);
        exit(BFUTILS_BUILD_ERROR_OUTSIDE_FUNCTION);
    }
    if (cfg.name == NULL || strlen(cfg.name) == 0) {
        fprintf(stderr, "Error on %s:%d - An executable must have a valid name\n", file, line);
        exit(BFUTILS_BUILD_ERROR_MISSING_NAME);
    }
    if (cfg.files_len <= 0) {
        fprintf(stderr, "Error on %s:%d - An executable must have source files\n", file, line);
        exit(BFUTILS_BUILD_ERROR_MISSING_FILE);
    }
    char **objs = malloc(sizeof(char *) * cfg.files_len);
    int objs_len = 0;
    for (int i = 0; i < cfg.files_len; i++) {
        char *file = basename(cfg.files[i]);
        if (bfutils_build_check_duplicate(file)) {
            continue;
        }
        char *obj = bfutils_get_file_object(file);
        objs[objs_len++] = obj;
        fprintf(bfutils_build_fp, "build target/objs/%s: cc %s\n", obj, cfg.files[i]);
        if (cfg.deps_len > 0) {
            char *other_flags = cfg.cflags ? cfg.cflags : BFUTILS_BUILD_CFLAGS;
            fprintf(bfutils_build_fp, " cflags = -fPIC");
            for (int i = 0; i < cfg.deps_len; i++) {
                char *val = bfutils_pkg_config_cflags(cfg.deps[i]);
                fprintf(bfutils_build_fp, " %s", val);
                free(val);
            }
            fprintf(bfutils_build_fp, " %s\n", other_flags);
        }
        else if (cfg.cflags) {
            fprintf(bfutils_build_fp, " cflags = -fPIC %s\n", cfg.cflags);
        }
        else {
            fprintf(bfutils_build_fp, " cflags = -fPIC %s\n", BFUTILS_BUILD_CFLAGS);
        }
    }
    fprintf(bfutils_build_fp, "build target/lib/lib%s.so: lib", cfg.name);
    for (int i = 0; i < objs_len; i++) {
        fprintf(bfutils_build_fp, " target/objs/%s", objs[i]);
        free(objs[i]);
    }
    free(objs);
    fprintf(bfutils_build_fp, "\n");
    if (cfg.deps_len > 0) {
        char *other_flags = cfg.ldflags ? cfg.ldflags : BFUTILS_BUILD_LDFLAGS;
        fprintf(bfutils_build_fp, " ldflags =");
        for (int i = 0; i < cfg.deps_len; i++) {
            char *val = bfutils_pkg_config_ldflags(cfg.deps[i]);
            fprintf(bfutils_build_fp, " %s", val);
            free(val);
        }
        fprintf(bfutils_build_fp, " %s\n", other_flags);
    }
    else if (cfg.ldflags) {
        fprintf(bfutils_build_fp, " ldflags = %s\n", cfg.ldflags);
    }
}

void bfutils_add_executable_fn(BFUtilsBuildCfg cfg, char *file, int line) {
    if (bfutils_build_fp == NULL) {
        fprintf(stderr, "Error on %s:%d - bfutils_add_executable must be called inside bfutils_build function\n", file, line);
        exit(BFUTILS_BUILD_ERROR_OUTSIDE_FUNCTION);
    }
    if (cfg.name == NULL || strlen(cfg.name) == 0) {
        fprintf(stderr, "Error on %s:%d - An executable must have a valid name\n", file, line);
        exit(BFUTILS_BUILD_ERROR_MISSING_NAME);
    }
    if (cfg.files_len <= 0) {
        fprintf(stderr, "Error on %s:%d - An executable must have source files\n", file, line);
        exit(BFUTILS_BUILD_ERROR_MISSING_FILE);
    }
    char **objs = malloc(sizeof(char *) * cfg.files_len);
    int objs_len = 0;
    for (int i = 0; i < cfg.files_len; i++) {
        char *file = basename(cfg.files[i]);
        if (bfutils_build_check_duplicate(file)) {
            continue;
        }
        char *obj = bfutils_get_file_object(file);
        objs[objs_len++] = obj;
        fprintf(bfutils_build_fp, "build target/objs/%s: cc %s\n", obj, cfg.files[i]);
        if (cfg.deps_len > 0) {
            char *other_flags = cfg.cflags ? cfg.cflags : BFUTILS_BUILD_CFLAGS;
            fprintf(bfutils_build_fp, " cflags =");
            for (int i = 0; i < cfg.deps_len; i++) {
                char *val = bfutils_pkg_config_cflags(cfg.deps[i]);
                fprintf(bfutils_build_fp, " %s", val);
                free(val);
            }
            fprintf(bfutils_build_fp, " %s\n", other_flags);
        }
        else if (cfg.cflags) {
            fprintf(bfutils_build_fp, " cflags = %s\n", cfg.cflags);
        }
    }
    fprintf(bfutils_build_fp, "build target/bin/%s: link", cfg.name);
    for (int i = 0; i < objs_len; i++) {
        fprintf(bfutils_build_fp, " target/objs/%s", objs[i]);
        free(objs[i]);
    }
    free(objs);
    fprintf(bfutils_build_fp, "\n");
    if (cfg.deps_len > 0) {
        char *other_flags = cfg.ldflags ? cfg.ldflags : BFUTILS_BUILD_LDFLAGS;
        fprintf(bfutils_build_fp, " ldflags =");
        for (int i = 0; i < cfg.deps_len; i++) {
            char *val = bfutils_pkg_config_ldflags(cfg.deps[i]);
            fprintf(bfutils_build_fp, " %s", val);
            free(val);
        }
        fprintf(bfutils_build_fp, " %s\n", other_flags);
    }
    else if (cfg.ldflags) {
        fprintf(bfutils_build_fp, " ldflags = %s\n", cfg.ldflags);
    }
}
#endif //BFUTILS_BUILD_IMPLEMENTATION
