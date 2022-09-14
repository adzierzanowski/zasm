#include "util.h"


static bool z_strmatch_(bool case_sensitive, char *str, va_list args) {
  char *ptr = va_arg(args, char *);

  while (ptr != NULL) {
    if ((case_sensitive && z_streq(str, ptr)) ||
        (!case_sensitive && z_streq_i(str, ptr))) {
      return true;
    }

    ptr = va_arg(args, char *);
  }

  return false;
}

bool z_strmatch(char *str, ...) {
  va_list args;
  va_start(args, str);
  bool res = z_strmatch_(true, str, args);
  va_end(args);
  return res;
}

bool z_strmatch_i(char *str, ...) {
  va_list args;
  va_start(args, str);
  bool res = z_strmatch_(false, str, args);
  va_end(args);
  return res;
}

void z_fail(struct z_token_t *token, const char *fmt, ...) {
  char buf[0x1000] = {0};

  va_list args;
  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);

  if (token) {
  fprintf(
    stderr, "\x1b[38;5;1mERROR: %s:%d:%d [%s:`%s`] %s\x1b[0m",
    token->fname, token->line+1, token->col+1, z_toktype_str(token->type), token->value, buf);
  } else {
    fprintf(stderr, "\x1b[38;5;1mERROR: %s\x1b[0m", buf);
  }
}

int z_indexof(char *haystack, char needle) {
  int res = -1;

  for (int i = 0; i < strlen(haystack); i++) {
    if (haystack[i] == needle) {
      return i;
    }
  }

  return res;
}

static bool z_streq_(char *str1, char *str2, bool case_sensitive) {
  char buf1[Z_BUFSZ] = {0};
  char buf2[Z_BUFSZ] = {0};

  strncpy(buf1, str1, Z_BUFSZ);
  strncpy(buf2, str2, Z_BUFSZ);

  if (!case_sensitive) {
    z_strlower(buf1);
    z_strlower(buf2);
  }


  return strcmp(buf1, buf2) == 0;
}

bool z_streq(char *str1, char *str2) {
  return z_streq_(str1, str2, true);
}

bool z_streq_i(char *str1, char *str2) {
  return z_streq_(str1, str2, false);
}

char *z_dirname(const char *fname) {
  char *out = calloc(strlen(fname) + 1, sizeof (char));
  strcpy(out, fname);
  char *last_slash = strrchr(out, '/');
  if (last_slash) {
    *last_slash = 0;
  } else {
    free(out);
    return NULL;
  }
  return out;
}

void z_strlower(char *str) {
  for (int i = 0; i < strlen(str); i++) {
    str[i] = tolower(str[i]);
  }
}
