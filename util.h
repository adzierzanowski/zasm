#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

void z_dprintf(const char *fname, size_t line, int col, const char *fmt, ...);
bool z_strmatch(const char *str, ...);
void z_fail(const char *fname, size_t line, int col, const char *fmt, ...);
int z_indexof(char *haystack, char needle);
bool z_streq(char *str1, char *str2);

#endif
