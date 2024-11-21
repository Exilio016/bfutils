#include <stdio.h>
#include <stddef.h>
#define BFUTILS_VECTOR_IMPLEMENTATION
#include "bfutils_vector.h"


int main(int argc, char **argv) {
    int *v = NULL;
    printf("Length: %d\tCapacity: %d\n", (int) vector_length(v), (int) vector_capacity(v));
    for (int i = 0; i < 200; i++) {
        vector_push(v, i);
        printf("Length: %d\tCapacity: %d\n", (int) vector_length(v), (int) vector_capacity(v));
    }
    printf("v[124] = %d\n", v[124]);
    printf("pop %d\n", vector_pop(v));
    printf("pop %d\n", vector_pop(v));
    printf("pop %d\n", vector_pop(v));

    vector_ensure_capacity(v, 1024);
    printf("Length: %d\tCapacity: %d\n", (int) vector_length(v), (int) vector_capacity(v));
    vector_free(v);

    char *builder = NULL;
    string_push_cstr(builder, "Test");
    printf("Length: %d\tCapacity: %d\n", (int) vector_length(builder), (int) vector_capacity(builder));
    char *new = NULL;
    string_push(new, builder);
    printf("Length: %d\tCapacity: %d\n", (int) vector_length(new), (int) vector_capacity(new));

    vector_free(builder);
    vector_free(new);
    return 0;
}
