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

void z_expr_eval(struct z_token_t *token, struct z_label_t *labels, uint16_t origin) {
  if (z_typecmp(token, Z_TOKTYPE_EXPRESSION)) {
    struct z_token_t *outq[TOKBUFSZ] = {0};
    struct z_token_t *opstack[TOKBUFSZ] = {0};
    int qptr = 0;
    int sptr = 0;

    for (int i = 0; i < token->children_count; i++) {
      struct z_token_t *tok = z_get_child(token, i);

      if (z_typecmp(tok, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
        outq[qptr++] = tok;

      } else if (z_typecmp(tok, Z_TOKTYPE_IDENTIFIER)) {
        struct z_label_t *label = z_label_get(labels, tok->value);

        if (label) {
          tok->numval = label->value;
          outq[qptr++] = tok;

        } else {
          z_fail(tok, "Couldn't retrieve label: '%s'.\n", tok->value);
          exit(1);
        }

      } else if (z_typecmp(tok, Z_TOKTYPE_OPERATOR)) {
        // TODO: implement shunting yard

      } else {
        z_fail(tok, "Unexpected token type in expression: %s.\n", z_toktype_str(tok->type));
        exit(1);
      }
    }

    token->numval = 0x8000 + origin;
  }
}