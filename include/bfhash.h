#ifndef __BF_UTILS_HASH_H__
#define __BF_UTILS_HASH_H__

#ifndef BF_HASH_TABLE_INITIAL_CAPACITY
#define BF_HASH_TABLE_INITIAL_CAPACITY 128
#endif

#define BF_HASH_TABLE_DECLARE(camel_case_name, snake_case_name, key_type,      \
                              element_type)                                    \
  typedef struct {                                                             \
    int is_set;                                                                \
    key_type key;                                                              \
    element_type value;                                                        \
  } camel_case_name##Item;                                                     \
                                                                               \
  typedef struct {                                                             \
    camel_case_name##Item *items;                                              \
    size_t lenght;                                                             \
    size_t capacity;                                                           \
  } camel_case_name;                                                           \
                                                                               \
  camel_case_name snake_case_name##_init();                                    \
  void snake_case_name##_insert(camel_case_name *table, key_type key,          \
                                element_type value);                           \
  void snake_case_name##_remove(camel_case_name *table, key_type key);         \
  camel_case_name##Item *snake_case_name##_get(camel_case_name *table,         \
                                               key_type key);                  \
  camel_case_name##Item *snake_case_name##_begin(camel_case_name *table);      \
  camel_case_name##Item *snake_case_name##_end(camel_case_name *table);

#define BF_HASH_TABLE_KEY_CMP(a, b) memcmp(a, b, sizeof(*(a)))

#define BF_HASH_TABLE_IMPL(camel_case_name, snake_case_name, key_type,         \
                           element_type, hash_func)                            \
  BF_HASH_TABLE_IMPL_EXTENDED(camel_case_name, snake_case_name, key_type,      \
                              element_type, hash_func, BF_HASH_TABLE_KEY_CMP)

#define BF_HASH_TABLE_IMPL_EXTENDED(camel_case_name, snake_case_name,          \
                                    key_type, element_type, hash_func,         \
                                    key_cmp)                                   \
  camel_case_name snake_case_name##_init() {                                   \
    camel_case_name table = {0};                                               \
    table.capacity = BF_HASH_TABLE_INITIAL_CAPACITY;                           \
    table.items = (camel_case_name##Item *)calloc(                             \
        sizeof(camel_case_name##Item), table.capacity);                        \
    table.lenght = 0;                                                          \
    return table;                                                              \
  }                                                                            \
                                                                               \
  void snake_case_name##_expand(camel_case_name *table) {                      \
    size_t old_capacity = table->capacity;                                     \
    camel_case_name##Item *items = table->items;                               \
                                                                               \
    table->capacity *= 1.5;                                                    \
    table->lenght = 0;                                                         \
    table->items = (camel_case_name##Item *)calloc(                            \
        sizeof(camel_case_name##Item), table->capacity);                       \
    for (int i = 0; i < old_capacity; i++) {                                   \
      camel_case_name##Item item = items[i];                                   \
      if (item.is_set) {                                                       \
        snake_case_name##_insert(table, item.key, item.value);                 \
      }                                                                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  void snake_case_name##_insert(camel_case_name *table, key_type key,          \
                                element_type value) {                          \
    size_t hash = hash_func(key, table->capacity);                             \
                                                                               \
    if (table->lenght == table->capacity - 1) {                                \
      snake_case_name##_expand(table);                                         \
    }                                                                          \
                                                                               \
                                                                               \
    camel_case_name##Item *item = &table->items[hash];                         \
    if (!item->is_set) {                                                       \
      item->key = key;                                                         \
      item->value = value;                                                     \
      item->is_set = 1;                                                        \
      table->lenght++;                                                         \
      return;                                                                  \
    }                                                                          \
                                                                               \
    if (key_cmp(&item->key, &key) == 0) {                                      \
      item->value = value;                                                     \
      return;                                                                  \
    }                                                                          \
                                                                               \
    size_t cur = (hash + 1) % table->capacity;                                 \
    while (cur != hash) {                                                      \
      item = &table->items[cur];                                               \
      if (!item->is_set) {                                                     \
        item->key = key;                                                       \
        item->value = value;                                                   \
        item->is_set = 1;                                                      \
        table->lenght++;                                                       \
        return;                                                                \
      }                                                                        \
      if (key_cmp(&item->key, &key) == 0) {                                    \
        return;                                                                \
      }                                                                        \
      cur = (cur + 1) % table->capacity;                                       \
    }                                                                          \
  }                                                                            \
                                                                               \
  void snake_case_name##_remove(camel_case_name *table, key_type key) {        \
    size_t hash = hash_func(key, table->capacity);                             \
    camel_case_name##Item *item = table->items + hash;                         \
    if (!item->is_set) {                                                       \
      return;                                                                  \
    }                                                                          \
                                                                               \
    if (key_cmp(&item->key, &key) == 0) {                                      \
      item->is_set = 0;                                                        \
      table->lenght--;                                                         \
      return;                                                                  \
    }                                                                          \
                                                                               \
    size_t cur = (hash + 1) % table->capacity;                                 \
    while (cur != hash) {                                                      \
      item = table->items + cur;                                               \
      if (!item->is_set) {                                                     \
        return;                                                                \
      }                                                                        \
      if (key_cmp(&item->key, &key) == 0) {                                    \
        item->is_set = 0;                                                      \
        table->lenght--;                                                       \
        return;                                                                \
      }                                                                        \
      cur = (cur + 1) % table->capacity;                                       \
    }                                                                          \
  }                                                                            \
                                                                               \
  camel_case_name##Item *snake_case_name##_get(camel_case_name *table,         \
                                               key_type key) {                 \
    size_t hash = hash_func(key, table->capacity);                             \
    camel_case_name##Item *item = table->items + hash;                         \
    if (!item->is_set) {                                                       \
      return NULL;                                                             \
    }                                                                          \
    if (key_cmp(&item->key, &key) == 0) {                                      \
      return item;                                                             \
    }                                                                          \
    size_t cur = (hash + 1) % table->capacity;                                 \
    while (cur != hash) {                                                      \
      item = table->items + cur;                                               \
      if (!item->is_set) {                                                     \
        return NULL;                                                           \
      }                                                                        \
      if (key_cmp(&item->key, &key) == 0) {                                    \
        return item;                                                           \
      }                                                                        \
      cur = (cur + 1) % table->capacity;                                       \
    }                                                                          \
    return NULL;                                                               \
  }                                                                            \
                                                                               \
  camel_case_name##Item *snake_case_name##_begin(camel_case_name *table) {     \
    if (table->lenght == 0) {                                                  \
      return NULL;                                                             \
    }                                                                          \
    return table->items;                                                       \
  }                                                                            \
                                                                               \
  camel_case_name##Item *snake_case_name##_end(camel_case_name *table) {       \
    if (table->lenght == 0) {                                                  \
      return NULL;                                                             \
    }                                                                          \
    return table->items + table->capacity;                                     \
  }

#endif
