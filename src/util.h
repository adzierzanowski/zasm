#ifndef UTIL_H
#define UTIL_H

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "structs.h"
#include "tokenizer.h"

bool z_strmatch(char *str, ...);
bool z_strmatch_i(char *str, ...);
void z_fail(struct z_token_t *token, const char *fmt, ...);
int z_indexof(char *haystack, char needle);
bool z_streq(char *str1, char *str2);
bool z_streq_i(char *str1, char *str2);
char *z_dirname(const char *fname);
void z_strlower(char *str);

#endif
