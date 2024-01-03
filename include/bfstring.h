#ifndef __BF_UTILS_STRING_H__
#define __BF_UTILS_STRING_H__
#include "bfarray.h"
#include <bfarray.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

BF_ARRAY_DECLARE(BFString, bf_string, char);
BF_ARRAY_DECLARE(BFStringSplit, bf_string_split, BFString);
BF_VECTOR_DECLARE(BFStringBuilder, bf_string_builder, char);

void bf_string_builder_append_c_str(BFStringBuilder *builder, char *str);
void bf_string_builder_append_str(BFStringBuilder *builder, BFString *str);
BFString bf_string_builder_to_str(BFStringBuilder *builder);
char *bf_string_to_c_str(BFString *str);
BFString bf_string_from_c_str(char *str);
BFString bf_string_join(BFString *head, BFString *tail);
int bf_string_equals(BFString *a, BFString *b);
BFStringSplit bf_string_split(BFString *str, BFString *delim);
int bf_string_cmp(BFString *a, BFString *b);
#endif
