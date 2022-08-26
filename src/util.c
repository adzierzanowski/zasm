#include "util.h"

bool z_strmatch(const char *str, ...) {
  va_list args;
  va_start(args, str);

  const char *ptr = va_arg(args, const char *);

  while (ptr != NULL) {
    if (strcmp(str, ptr) == 0) {
      va_end(args);
      return true;
    }

    ptr = va_arg(args, const char *);
  }

  va_end(args);

  return false;
}

void z_fail(struct z_token_t *token, const char *fmt, ...) {
  char buf[0x1000] = {0};

  va_list args;
  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);

  if (token) {
  fprintf(
    stderr, "\x1b[38;5;1mERROR: %s:%d:%d %s\x1b[0m",
    token->fname, token->line+1, token->col+1, buf);
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

bool z_streq(char *str1, char *str2) {
  return strcmp(str1, str2) == 0;
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
