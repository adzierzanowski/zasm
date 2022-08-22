#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "structs.h"

bool z_strmatch(const char *str, ...);
void z_fail(struct z_token_t *token, const char *fmt, ...);
int z_indexof(char *haystack, char needle);
bool z_streq(char *str1, char *str2);
char *z_dirname(const char *fname);

#endif
