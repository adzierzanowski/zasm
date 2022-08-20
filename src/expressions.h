#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include "structs.h"
#include "tokenizer.h"

void z_expr_cvt(struct z_token_t *token);
void z_expr_eval(
  struct z_token_t *token, struct z_label_t *labels, uint16_t origin);

#endif
