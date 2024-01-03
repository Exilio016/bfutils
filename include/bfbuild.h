#ifndef __BF_UTILS_BUILD__
#define __BF_UTILS_BUILD__

#ifndef BF_UTILS_INCLUDED

#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

int download_itself(const char *version) {
  struct stat s = {0};
  if (stat(".dep", &s) != 0) {
    if (mkdir(".dep", S_IREAD | S_IWRITE) != 0) {
      perror("mkdir");
      return -1;
    }
  }

  if (!S_ISDIR(s.st_mode)) {
    printf("ERROR: .dep exists and is not a directory");
    return -1;
  }

  execl("wget", "-O .dep/", "https://github.com/exilio016/bfutils");
}

#endif

#endif
