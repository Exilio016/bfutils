#define BFUTILS_BUILD_CFLAGS "-Wall -O3"
#define BFUTILS_BUILD_IMPLEMENTATION
#include "bfutils_build.h"

void bfutils_build(int argc, char *argv[]) {
    BFUtilsBuildCfg bfutils = {
        .name = "bfutils",
        .files = (char*[]) { "bfutils.c" },
        .files_len = 1,
        .deps = (char*[]) { "openssl" },
        .deps_len = 1
    };
    bfutils_add_executable(bfutils);
    
    BFUtilsBuildCfg test = {
        .name = "test",
        .ldflags = "-fprofile-arcs",
        .cflags = "-fPIC -fprofile-arcs -ftest-coverage",
        .files = (char*[]) { "test.c" },
        .files_len = 1,
    };
    bfutils_add_executable(test);
}
