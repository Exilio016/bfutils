#define BFUTILS_BUILD_IMPLEMENTATION
#include "bfutils_build.h"

void bfutils_build(int argc, char *argv[]) {
    BFUtilsBuildExecutable bfutils = {
        .name = "bfutils",
        .files = (char*[]) { "bfutils.c" },
        .files_len = 1,
        .libs = "-lssl",
    };

    bfutils_add_executable(bfutils);
}
