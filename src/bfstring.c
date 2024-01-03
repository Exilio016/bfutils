#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "bfarray.h"
#include "bfstring.h"

BF_ARRAY_IMPL(BFString, bf_string, char)
BF_VECTOR_IMPL(BFStringBuilder, bf_string_builder, char)

void bf_string_builder_append_c_str(BFStringBuilder *builder, char *str) {
  for (int i = 0; i < strlen(str); i++) {
    bf_string_builder_append(builder, str[i]);
  }
}

void bf_string_builder_append_str(BFStringBuilder *builder, BFString *str) {
  for (int i = 0; i < str->lenght; i++) {
    bf_string_builder_append(builder, str->items[i]);
  }
}

BFString bf_string_builder_to_str(BFStringBuilder *builder) {
  BFString dest;
  dest.items = malloc(sizeof(char) * (builder->lenght + 1));
  dest.lenght = builder->lenght;
  memcpy(dest.items, builder->items, sizeof(char) * builder->lenght);
  dest.items[dest.lenght] = '\0';
  return dest;
}

char *bf_string_to_c_str(BFString *str) {
  str->items[str->lenght] = '\0';
  return str->items;
}

typedef struct {
  BFString *items;
  size_t lenght;
  size_t capacity;
} BFStringSplitBuilder;

int bf_string_equals(BFString *a, BFString *b) {
  if (a->lenght != b->lenght) {
    return 0;
  }
  return strncmp(a->items, b->items, a->lenght) == 0;
}

BF_VECTOR_IMPL_EXTENDED(BFStringSplitBuilder, bf_string_split_builder, BFString,
               bf_string_equals)
BF_ARRAY_IMPL(BFStringSplit, bf_string_split, BFString)

BFStringSplit bf_string_split(BFString *str, BFString *delim) {
  BFStringSplitBuilder builder = {0};
  int start = 0;
  int i;
  for (i = 0; i < str->lenght; i++) {
    char *it = str->items + i;
    if (delim->lenght > (str->lenght - i)) {
      break;
    }
    if (strncmp(delim->items, it, delim->lenght) == 0) {
      BFString tmp = {0};
      tmp.items = malloc(sizeof(char) * (i - start + 1));
      tmp.lenght = i - start;
      tmp.items[tmp.lenght] = '\0';
      memcpy(tmp.items, str->items + start, sizeof(char) * (i - start));
      bf_string_split_builder_append(&builder, tmp);
      start = i + delim->lenght;
      i += delim->lenght;
    }
  }
  BFString tmp = {0};
  tmp.items = malloc(sizeof(char) * (i - start + 1));
  tmp.lenght = i - start;
  tmp.items[tmp.lenght] = '\0';
  memcpy(tmp.items, str->items + start, sizeof(char) * (i - start));
  bf_string_split_builder_append(&builder, tmp);

  BFStringSplit result;
  result.lenght = builder.lenght;
  result.items = builder.items;
  return result;
}

BFString bf_string_join(BFString *head, BFString *tail) {
  if (head == NULL || tail == NULL) {
    return (BFString) {0};
  }
  BFString result;
  result.items = malloc(sizeof(char) * (head->lenght + tail->lenght + 1));
  result.lenght = head->lenght + tail->lenght;
  memcpy(result.items, head->items, sizeof(char) * head->lenght);
  memcpy(result.items + head->lenght, tail->items,
         sizeof(char) * tail->lenght);
  result.items[result.lenght] = '\0';
  return result;
}

BFString bf_string_from_c_str(char *str) {
  BFString result = {0};
  result.lenght = strlen(str);
  result.items = str;
  return result;
}

int bf_string_cmp(BFString *a, BFString *b) {
  if(a->lenght > b->lenght) {
    return strncmp(a->items, b->items, b->lenght);
  }
  return strncmp(a->items, b->items, a->lenght);
}
