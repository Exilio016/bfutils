#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#define BFUTILS_VECTOR_IMPLEMENTATION
#include "bfutils_vector.h"
#define BFUTILS_HASHMAP_IMPLEMENTATION
#include "bfutils_hash.h"
#define BFUTILS_PROCESS_IMPLEMENTATION
#include "bfutils_process.h"

typedef struct {
    int key;
    int value;
} IntNode;

typedef struct {
    char *key;
    int value;
} Node;

void test_hash() {
    IntNode *hashmap = NULL;
    hashmap_push(hashmap, 8, 120);
    assert(120 == hashmap_get(hashmap, 8));
    for(int i = 0 ; i < 200; i++) {
        hashmap_push(hashmap, i, i*5);
        if (i == 96) {
            assert(96 * 5 == hashmap_remove(hashmap, i));
        }
    }
    for(int i = 0 ; i < 200; i++) {
        if (i == 96) {
            assert(!hashmap_contains(hashmap, i));
        }
        else {
            int val = hashmap_get(hashmap, i);
            assert(val == (i*5));
        }
    }
    for (int i = 10; i < 200; i++) {
        hashmap_remove(hashmap, i);
    }
    HashmapIterator it = hashmap_iterator(hashmap);
    while(hashmap_iterator_has_next(&it)) {
        IntNode n = hashmap_iterator_next(hashmap, &it);
        printf("%d = %d\n", n.key, n.value);
    }
    printf("%ld\n", hashmap_header(hashmap)->length);

    Node *smap = NULL;
    string_hashmap_push(smap, "Test", 10);
    assert(10 == string_hashmap_get(smap, "Test"));
    string_hashmap_push(smap, "Foo", 1);
    string_hashmap_push(smap, "BAR", 2);
    assert(1 == string_hashmap_get(smap, "Foo"));
    string_hashmap_push(smap, "Foo", 3);
    assert(2 == string_hashmap_get(smap, "BAR"));
    assert(3 == string_hashmap_get(smap, "Foo"));
    assert(3 == hashmap_header(smap)->insert_count);

    it = hashmap_iterator(smap);
    while(hashmap_iterator_has_next(&it)) {
        Node n = hashmap_iterator_next(smap, &it);
        printf("%s = %d\n", n.key, n.value);
    }
    HashmapIterator itr = hashmap_iterator_reverse(smap);
    while(hashmap_iterator_has_previous(&itr)) {
        Node n = hashmap_iterator_previous(smap, &itr);
        printf("%s = %d\n", n.key, n.value);
    }

    hashmap_free(smap);
    hashmap_free(hashmap);
}

void process_test() {
    char *out;
    char *err;
    
    char **cmd = (char*[]){"file", "-i", "bfutils_process.h", NULL};
    int status = process_sync(cmd, NULL, &out, &err);
    assert(status == 0);
    assert(0 == strcmp("", err));
    assert(0 == strcmp("bfutils_process.h: text/x-c; charset=utf-8\n", out));
    free(out);
    free(err);

    status = process_sync((char*[]){"base64", NULL}, "abc", &out, &err);
    assert(status == 0);
    assert(0 == strcmp("", err));
    assert(0 == strcmp("YWJj\n", out));
    free(out);
    free(err);
}

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
    printf("Length: %d\tCapacity: %d - %s\n", (int) vector_length(builder), (int) vector_capacity(builder), builder);
    char *new = NULL;
    string_push(new, builder);
    printf("Length: %d\tCapacity: %d - %s\n", (int) vector_length(new), (int) vector_capacity(new), new);

    char *source = "Hello, world, foo, bar, bazz";
    char **list = string_split(source, ", ");
    printf("Splitted into %d chunks\n", (int) vector_length(list));
    for (int i =0; i < vector_length(list); i++) {
        printf("%s\n", list[i]);
        vector_free(list[i]);
    }
    test_hash();
    char *test = string_format("Hello world %d", vector_length(list));
    assert(0 == strcmp("Hello world 5", test));

    process_test();
    vector_free(test);
    vector_free(builder);
    vector_free(new);
    vector_free(list);
    return 0;
}
