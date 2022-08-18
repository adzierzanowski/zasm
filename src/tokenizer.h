#ifndef TOKENIZER_H
#define TOKENIZER_H

#define TOKBUFSZ 0x1000

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "structs.h"
#include "opcodes.h"
#include "config.h"
#include "expressions.h"

struct z_token_t *z_token_new(const char *fname, size_t line, int col, char *value, int type);
struct z_token_t **tokenize(
  const char *fname, size_t *tokcnt, struct z_label_t **labels, size_t *bytepos);
const char *z_toktype_str(enum z_toktype_t type);
const char *z_toktype_color(enum z_toktype_t type);
void z_token_add_child(struct z_token_t *parent, struct z_token_t *child);
void z_token_add(struct z_token_t ***tokens, size_t *tokcnt, struct z_token_t *token);
struct z_label_t *z_label_new(char *key, uint16_t value);
void z_label_add(struct z_label_t **labels, struct z_label_t *label);
bool z_typecmp(struct z_token_t *token, int types);
void z_parse_root( struct z_token_t *token, size_t *codepos, struct z_label_t **labels);
struct z_token_t *z_get_child(struct z_token_t *token, int child_index);
struct z_label_t *z_label_get(struct z_label_t *labels, char *key);

#endif
