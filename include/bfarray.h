#ifndef __BF_UTILS_ARRAY_H__
#define __BF_UTILS_ARRAY_H__
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef BF_VECTOR_INITIAL_CAPACITY
#define BF_VECTOR_INITIAL_CAPACITY 128
#endif

#define BF_ARRAY_IMPL(camel_case_name, snake_case_name, element_type)          \
  element_type *snake_case_name##_begin(camel_case_name *array) {              \
    if (array->lenght == 0) {                                                  \
      return NULL;                                                             \
    }                                                                          \
    return array->items;                                                       \
  }                                                                            \
                                                                               \
  element_type *snake_case_name##_end(camel_case_name *array) {                \
    if (array->lenght == 0) {                                                  \
      return NULL;                                                             \
    }                                                                          \
    return array->items + array->lenght;                                       \
  }

#define BF_ARRAY_DECLARE(camel_case_name, snake_case_name, element_type)       \
  typedef struct {                                                             \
    element_type *items;                                                       \
    size_t lenght;                                                             \
  } camel_case_name;                                                           \
                                                                               \
  element_type *snake_case_name##_begin(camel_case_name *array);               \
  element_type *snake_case_name##_end(camel_case_name *array);

#define BF_VECTOR_DECLARE(camel_case_name, snake_case_name, element_type)      \
  typedef struct {                                                             \
    element_type *items;                                                       \
    size_t lenght;                                                             \
    size_t capacity;                                                           \
  } camel_case_name;                                                           \
                                                                               \
  void snake_case_name##_append(camel_case_name *vector,                       \
                                element_type element);                         \
  element_type *snake_case_name##_begin(camel_case_name *vector);              \
  element_type *snake_case_name##_end(camel_case_name *vector);                \
  void snake_case_name##_swap_remove(camel_case_name *vector,                  \
                                     element_type element);                    \
  void snake_case_name##_remove(camel_case_name *vector, element_type element);

#define BF_VECTOR_ELEMENT_EQ(a, b) memcmp(a, b, sizeof(*(a))) == 0
#define BF_VECTOR_IMPL(camel_case_name, snake_case_name, element_type)         \
  BF_VECTOR_IMPL_EXTENDED(camel_case_name, snake_case_name, element_type,      \
                          BF_VECTOR_ELEMENT_EQ)

#define BF_VECTOR_IMPL_EXTENDED(camel_case_name, snake_case_name,              \
                                element_type, element_equals)                  \
  void snake_case_name##_append(camel_case_name *vector,                       \
                                element_type element) {                        \
    if ((vector)->capacity == 0) {                                             \
      (vector)->capacity = BF_VECTOR_INITIAL_CAPACITY;                         \
      (vector)->items = malloc(sizeof(*(vector)->items) * (vector)->capacity); \
    }                                                                          \
    if ((vector)->lenght + 1 >= (vector)->capacity) {                          \
      (vector)->capacity *= 1.5;                                               \
      (vector)->items = realloc((vector)->items, sizeof(*(vector)->items) *    \
                                                     (vector)->capacity);      \
    }                                                                          \
    (vector)->items[(vector)->lenght] = element;                               \
    (vector)->lenght++;                                                        \
  }                                                                            \
                                                                               \
  element_type *snake_case_name##_begin(camel_case_name *vector) {             \
    if (vector->lenght == 0) {                                                 \
      return NULL;                                                             \
    }                                                                          \
    return vector->items;                                                      \
  }                                                                            \
                                                                               \
  element_type *snake_case_name##_end(camel_case_name *vector) {               \
    if (vector->lenght == 0) {                                                 \
      return NULL;                                                             \
    }                                                                          \
    return vector->items + vector->lenght;                                     \
  }                                                                            \
                                                                               \
  void snake_case_name##_swap_remove(camel_case_name *vector,                  \
                                     element_type element) {                   \
    for (element_type *ptr = snake_case_name##_begin(vector);                  \
         ptr < snake_case_name##_end(vector); ptr++) {                         \
      if (element_equals(ptr, &element)) {                                     \
        element_type *last = snake_case_name##_end(vector) - 1;                \
        *ptr = *last;                                                          \
        vector->lenght--;                                                      \
        return;                                                                \
      }                                                                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  void snake_case_name##_remove(camel_case_name *vector,                       \
                                element_type element) {                        \
    int is_set = 0;                                                            \
    element_type *end = snake_case_name##_end(vector);                         \
    for (element_type *ptr = snake_case_name##_begin(vector); ptr < end;       \
         ptr++) {                                                              \
      if (element_equals(ptr, &element)) {                                     \
        is_set = 1;                                                            \
      }                                                                        \
      if (is_set && ptr < (end - 1)) {                                         \
        *ptr = *(ptr + 1);                                                     \
      }                                                                        \
    }                                                                          \
    if (vector->lenght > 0) {                                                  \
      vector->lenght--;                                                        \
    }                                                                          \
  }

#endif
