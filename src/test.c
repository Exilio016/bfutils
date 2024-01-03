#include "bfarray.h"
#include "bfstring.h"
#include "bfhash.h"
#include <assert.h>
#include <bfhash.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

size_t str_hash(char *key, size_t size) {
  size_t hash = 0;
  for (int i = 0; i < strlen(key); i++) {
    hash += key[i] * (i+1);
  }
  return hash % size;
}

char *int_to_str(int val) {
  int lenght = snprintf(NULL, 0, "%d", val);
  char *str = malloc(sizeof(char) * (lenght + 1));
  sprintf(str, "%d", val);
  str[lenght] = '\0';
  return str;
}

#define C_STR_CMP(a, b) strcmp(*(a), *(b))
BF_HASH_TABLE_DECLARE(BFHashTable, bf_hash_table, char*, char*);
BF_HASH_TABLE_IMPL_EXTENDED(BFHashTable, bf_hash_table, char*, char*, str_hash, C_STR_CMP);

void test_hash() {
  BFHashTable table = bf_hash_table_init();
  bf_hash_table_insert(&table, "foo", "bar");
  BFHashTableItem* val = bf_hash_table_get(&table, "foo");
  assert(strcmp(val->value, "bar") == 0);

  for (int i = 0; i < 1000; i++) { 
    bf_hash_table_insert(&table, int_to_str(i), int_to_str(i * 3));
  }
  assert(table.lenght == 1001);
  for (int i = 0; i < 1000; i++) {
     BFHashTableItem *val = bf_hash_table_get(&table, int_to_str(i));
     assert(strcmp(int_to_str(i * 3), val->value) == 0);
  }

  bf_hash_table_remove(&table, "foo");
  assert(table.lenght == 1000);
  for (BFHashTableItem *item = table.items; item < &table.items[table.capacity]; item++) {
    if(item->is_set) {
      free(item->key);
      free(item->value);
    }
  }
  free(table.items);
}

size_t colision_hash(int i, size_t size) { return 12; }

BF_HASH_TABLE_DECLARE(ColisionTestHash, colision_test_hash, int, int)
BF_HASH_TABLE_IMPL(ColisionTestHash, colision_test_hash, int, int, colision_hash)

void test_hash_colision() {
  ColisionTestHash table = colision_test_hash_init();
  for(int i = 0; i < 1000; i++) {
    colision_test_hash_insert(&table, i, i*3);
  }
  for(int i = 0; i < 1000; i++) {
    ColisionTestHashItem *item = colision_test_hash_get(&table, i);
    assert(item->value == i*3);
  }
  assert(table.lenght == 1000);
  ColisionTestHashItem *item = colision_test_hash_get(&table, 989);
  assert(item);
  assert(item->key == 989);
  
  colision_test_hash_remove(&table, 989);
  assert(table.lenght == 999);
  item = colision_test_hash_get(&table, 989);
  assert(!item);
  
  free(table.items);
}
  
void test_string_builder() {
  BFStringBuilder builder = {0};
  bf_string_builder_append(&builder, 't');
  bf_string_builder_append_c_str(&builder, "est");
  BFString suffix = bf_string_from_c_str(" string");
  bf_string_builder_append_str(&builder, &suffix);
  BFString str = bf_string_builder_to_str(&builder);
  assert(strcmp("test string", bf_string_to_c_str(&str)) == 0);

  free(str.items);
  free(builder.items);
}

void test_vector_iterator() {
  BFStringBuilder b = {0};
  for(int i = 0; i < 23; i++) {
    bf_string_builder_append(&b, 'a' + i);
  };
  int i = 0;
  for (char *c = bf_string_builder_begin(&b);
       c < bf_string_builder_end(&b);
       c++) {
    assert(*c == ('a' + i) );
    i++;
  }
  assert(i == b.lenght);

  free(b.items);
}

void test_array_iterator() {
  BFString s = bf_string_from_c_str("test");
  int i = 0;
  for (char *c = bf_string_begin(&s);
       c < bf_string_end(&s);
       c++) {
    assert(*c == ("test"[i]) );
    i++;
  }
  assert(i == s.lenght);
}

void test_vector_remove() {
  BFStringBuilder b = {0};
  bf_string_builder_append_c_str(&b, "hello,world!");
  bf_string_builder_swap_remove(&b, ',');
  BFString swaped = bf_string_builder_to_str(&b);
  assert(strcmp(bf_string_to_c_str(&swaped), "hello!world") == 0);
  free(swaped.items);
  
  bf_string_builder_remove(&b, '!');
  swaped = bf_string_builder_to_str(&b);
  assert(strcmp(bf_string_to_c_str(&swaped), "helloworld") == 0);
  free(swaped.items);
  free(b.items);
  b.capacity = 0;
  b.lenght = 0;

  bf_string_builder_append_c_str(&b, "abc");
  bf_string_builder_remove(&b, 'c');
  swaped = bf_string_builder_to_str(&b);
  assert(strcmp(bf_string_to_c_str(&swaped), "ab") == 0);
  free(swaped.items);
  
  bf_string_builder_swap_remove(&b, 'b');
  swaped = bf_string_builder_to_str(&b);
  assert(strcmp(bf_string_to_c_str(&swaped), "a") == 0);
  free(b.items);
}

void test_split() {
  BFString source = bf_string_from_c_str("!Test!split!value!");
  BFString delim = bf_string_from_c_str("!");
  BFStringSplit result = bf_string_split(&source, &delim);
  assert(result.lenght == 5);
  assert(strcmp(result.items[0].items, "") == 0);
  assert(strcmp(result.items[1].items, "Test") == 0);
  assert(strcmp(result.items[2].items, "split") == 0);
  assert(strcmp(result.items[3].items, "value") == 0);
  assert(strcmp(result.items[4].items, "") == 0);

  BFString joined = bf_string_join(result.items + 1, result.items + 3);
  BFString expected = bf_string_from_c_str("Testvalue");
  assert(bf_string_equals(&joined, &expected));
}

int main() {
  test_hash();
  test_string_builder();
  test_vector_iterator();
  test_array_iterator();
  test_vector_remove();
  test_hash_colision();
  test_split();
}
