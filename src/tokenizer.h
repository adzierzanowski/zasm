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

// Constructors
struct z_token_t *z_token_new(
  const char *fname, size_t line, int col, char *value, int type);
struct z_label_t *z_label_new(char *key, uint16_t value);
struct z_def_t *z_def_new(
  char *key, struct z_token_t *value, struct z_token_t *deftok);

// Destructors
void z_labels_free(struct z_label_t *labels);
void z_defs_free(struct z_def_t *defs);
void z_tokens_free(struct z_token_t **tokens, size_t tokcnt);

// Util functions
const char *z_toktype_str(enum z_toktype_t type);
const char *z_toktype_color(enum z_toktype_t type);
bool z_typecmp(struct z_token_t *token, int types);


// List utils
void z_token_add_child(struct z_token_t *parent, struct z_token_t *child);
void z_token_add(
  struct z_token_t ***tokens, size_t *tokcnt, struct z_token_t *token);
void z_label_add(struct z_label_t **labels, struct z_label_t *label);
void z_def_add(struct z_def_t **defs, struct z_def_t *def);
struct z_token_t *z_get_child(struct z_token_t *token, int child_index);
struct z_label_t *z_label_get(struct z_label_t *labels, char *key);
struct z_def_t *z_def_get(struct z_def_t *defs, char *key);
int *z_lbldef_resolve(
  struct z_label_t *labels,
  struct z_def_t *defs,
  uint16_t origin,
  char *key);

struct z_token_t **z_tokenize(
    const char *fname,
    size_t *tokcnt,
    struct z_label_t **labels,
    struct z_def_t **defs,
    size_t *bytepos);
void z_parse_root(
  struct z_token_t ***tokens,
  struct z_token_t *token,
  size_t *codepos,
  struct z_label_t **labels,
  struct z_def_t **defs,
  size_t *tokcnt);
struct z_token_t **z_tokens_merge(
  struct z_token_t **tokens1,
  struct z_token_t **tokens2,
  size_t tokcnt1,
  size_t tokcnt2,
  size_t *tokcnt_out);
void z_labels_export(FILE *f, struct z_label_t *labels, struct z_def_t *defs);
struct z_label_t *z_labels_import(FILE *f);

#endif
