#ifndef TOKENIZER_H
#define TOKENIZER_H

#define TOKBUFSZ 0x1000

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "structs.h"
#include "opcodes.h"

struct z_token_t *z_token_new(const char *fname, size_t line, int col, char *value);
struct z_token_t **tokenize(const char *fname, size_t *tokcnt, struct z_label_t **labels);
const char *z_toktype_str(enum z_toktype_t type);
void z_check_type(
  FILE *f,
  const char *fname,
  size_t line,
  int col,
  uint32_t expected,
  struct z_token_t *token);
void z_token_add(struct z_token_t ***tokens, size_t *tokcnt, struct z_token_t *token);
void z_parse_root(struct z_token_t *token, int *codepos, struct z_label_t **labels);
void z_token_link(struct z_token_t *parent, struct z_token_t *child);
struct z_label_t *z_label_new(char *key, uint16_t value);
void z_label_add(struct z_label_t **labels, struct z_label_t *label);

#endif
