#include "expressions.h"

void z_expr_cvt(struct z_token_t *token) {
  if (!token) return;

  for (int i = 0; i < token->children_count; i++) {
    struct z_token_t *operand = token->children[i];

    if (operand->children_count > 0 &&
        z_typecmp(operand,
          Z_TOKTYPE_CHAR |
          Z_TOKTYPE_NUMBER |
          Z_TOKTYPE_IDENTIFIER |
          Z_TOKTYPE_OPERATOR)) {

      char exprval[TOKBUFSZ] = {0};
      int exprvalptr = 0;

      for (int j = 0; j < strlen(operand->value); j++) {
        exprval[exprvalptr++] = operand->value[j];
      }

      exprval[exprvalptr++] = ' ';

      for (int j = 0; j < operand->children_count; j++) {
        for (int k = 0; k < strlen(operand->children[j]->value); k++) {
          exprval[exprvalptr++] = operand->children[j]->value[k];
        }

        if (j < operand->children_count - 1) {
          exprval[exprvalptr++] = ' ';
        }
      }

      struct z_token_t *exprtoken = z_token_new(
        token->fname, token->line, token->col, exprval, Z_TOKTYPE_EXPRESSION);
      exprtoken->memref = operand->memref;
      exprtoken->children_count = operand->children_count + 1;
      exprtoken->children = malloc(sizeof (struct z_token_t *) * exprtoken->children_count);
      exprtoken->children[0] = operand;
      for (int j = 0; j < operand->children_count; j++) {
        exprtoken->children[j+1] = operand->children[j];
      }
      token->children[i] = exprtoken;
    }
  }
}

void z_expr_eval(struct z_token_t *token, struct z_label_t *labels) {
  if (z_typecmp(token, Z_TOKTYPE_EXPRESSION)) {
    token->numval = 0xff;
  }
}
