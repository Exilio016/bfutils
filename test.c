#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include "bfutils_test.h"
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
    IntNode *map = NULL;
    hashmap_push(map, 8, 120);
    assert(120 == hashmap_get(map, 8));
    for(int i = 0 ; i < 200; i++) {
        hashmap_push(map, i, i*5);
        if (i == 96) {
            assert(96 * 5 == hashmap_remove(map, i));
        }
    }
    for(int i = 0 ; i < 200; i++) {
        if (i == 96) {
            assert(!hashmap_contains(map, i));
        }
        else {
            int val = hashmap_get(map, i);
            assert(val == (i*5));
        }
    }
    for (int i = 10; i < 200; i++) {
        hashmap_remove(map, i);
    }
    int count = 0;
    HashmapIterator it = hashmap_iterator(map);
    while(hashmap_iterator_has_next(&it)) {
        IntNode n = hashmap_iterator_next(map, &it);
        (void) n;
        count++;
    }
    assert(10 == count);

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

    count = 0;
    it = hashmap_iterator(smap);
    while(hashmap_iterator_has_next(&it)) {
        Node n = hashmap_iterator_next(smap, &it);
        (void) n;
        count++;
    }
    assert(3 == count);

    count = 0;
    HashmapIterator itr = hashmap_iterator_reverse(smap);
    while(hashmap_iterator_has_previous(&itr)) {
        Node n = hashmap_iterator_previous(smap, &itr);
        (void) n;
        count++;
    }
    assert(3 == count);

    hashmap_free(smap);
    hashmap_free(map);
}

void test_process() {
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

    Process p = process_async((char*[]) {"sleep", "3", NULL});
    int s;
    while(process_is_running(&p, &s)){
        sleep(1);
    }
    assert(s == 0);
    process_close(&p);
}

void test_vector() {
    int *v = NULL;
    assert(0 == vector_length(v));
    assert(0 == vector_capacity(v));

    for (int i = 0; i < 200; i++) {
        vector_push(v, i);
        assert(i+1 == vector_length(v));
    }
    assert(124 == v[124]);
    assert(199 == vector_pop(v));
    assert(198 == vector_pop(v));
    assert(197 == vector_pop(v));

    vector_ensure_capacity(v, 1024);
    assert(197 == vector_length(v));
    assert(1024 == vector_capacity(v));
    vector_free(v);

    char *builder = NULL;
    string_push_cstr(builder, "Test");
    assert(0 == strcmp("Test", builder));
    char *new = NULL;
    string_push(new, builder);
    assert(0 == strcmp("Test", new));

    char *source = "Hello, world, foo, bar, bazz";
    char **list = string_split(source, ", ");
    assert(5 == vector_length(list));

    assert(0 == strcmp("Hello", list[0]));
    vector_free(list[0]);
    assert(0 == strcmp("world", list[1]));
    vector_free(list[1]);
    assert(0 == strcmp("foo", list[2]));
    vector_free(list[2]);
    assert(0 == strcmp("bar", list[3]));
    vector_free(list[3]);
    assert(0 == strcmp("bazz", list[4]));
    vector_free(list[4]);

    char *test = string_format("Hello world %d", vector_length(list));
    assert(0 == strcmp("Hello world 5", test));

    vector_free(test);
    vector_free(builder);
    vector_free(new);
    vector_free(list);
}

BFUTILS_VECTOR_FREE_WRAPPER(free_matrix_element, int*, vector_free)
void test_element_free() {
    int **matrix = vector(free_matrix_element);
    for (int i = 0; i < 10; i++) {
        vector_push(matrix, NULL);
        for (int j = 0; j < 10; j++) {
            vector_push(matrix[i], i + j);
        }
    }
    vector_free(matrix);
    assert(matrix == NULL);
}

typedef struct {
    int key;
    int *value;
} NodeWithPtr;

void free_node_with_ptr(void *obj) {
    NodeWithPtr *node = (NodeWithPtr*) obj;
    vector_free(node->value);
}

void test_hash_element_free() {
    NodeWithPtr *map = hashmap(free_node_with_ptr);
    for (int i = 0; i < 10; i++) {
        int *vec = NULL;
        for (int j = 0; j < 10; j++) {
            vector_push(vec, i + j);
        }
        hashmap_push(map, i, vec);
    }
    int *vec = NULL;
    vector_push(vec, 10);
    hashmap_push(map, 2, vec);

    hashmap_free(map);
}

static int test_count;
static int success_count;

void before_all() {
    printf("Unit tests for BFUTILS libraries\n");
    test_count = 0;
    success_count = 0;
}

void before_each() {
    test_count++;
}

void after_each() {
    if (bfutils_test_success) {
        success_count++;
    }
}

void after_all() {
    printf("%d tests executed, with %d success\n", test_count, success_count);
}

#define BFUTILS_TEST_BEFORE_ALL \
    X(before_all)

#define BFUTILS_TEST_BEFORE_EACH \
    X(before_each)

#define BFUTILS_TEST_AFTER_EACH \
    X(after_each)

#define BFUTILS_TEST_AFTER_ALL \
    X(after_all) 

#define BFUTILS_TEST_LIST \
    X("bfutils_vector", test_vector) \
    X("bfutils_vector element free", test_element_free)\
    X("bfutils_hash", test_hash) \
    X("bfutils_hash element free", test_hash_element_free)\
    X("bfutils_process", test_process)


#define BFUTILS_TEST_MAIN
#include "bfutils_test.h"
