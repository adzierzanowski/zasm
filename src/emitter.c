#include "emitter.h"


struct z_label_t *z_label_get(struct z_label_t *labels, char *key) {
  struct z_label_t *ptr = labels;

  while (ptr != NULL) {
    if (z_streq(ptr->key, key)) {
      return ptr;
    }

    ptr = ptr->next;
  }

  return NULL;
}

uint8_t *z_emit(struct z_token_t **tokens, size_t tokcnt, size_t *emitsz, struct z_label_t *labels, size_t bytepos) {
  uint8_t *out = calloc(bytepos, sizeof (uint8_t));

  *emitsz = bytepos;

  int emitptr = 0;


  for (int i = 0; i < tokcnt; i++) {
    struct z_token_t *token = tokens[i];

    if (z_typecmp(token, Z_TOKTYPE_INSTRUCTION)) {
      if (token->opcode) {
        struct z_opcode_t *opcode = token->opcode;

        if (opcode->size > 0) {
          for (int j = 0; j < opcode->size; j++) {
            out[emitptr++] = opcode->bytes[j];
          }

          if (token->label_offset) {
            int oplen = opcode->size - token->label_offset;
            int opstart = emitptr - opcode->size + token->label_offset;
            struct z_token_t *operand = token->numop;

            z_expr_eval(operand, labels);

            if (oplen == 1 || opcode->bytes[1] == 0xcb) {
              if (z_strmatch(token->value, "jr", "djnz", NULL)) {
                out[opstart] = operand->numval - 2;

              } else {
                out[opstart] = operand->numval;
              }

            } else if (oplen ==  2) {
              out[opstart] = operand->numval & 0xff;
              out[opstart + 1] = operand->numval >> 8;
            }
          }
        }
      }

    } else if (z_typecmp(token, Z_TOKTYPE_DIRECTIVE)) {
      if (z_streq(token->value, "db")) {
        for (int i = 0; i < token->children_count; i++) {
          struct z_token_t *op = token->children[i];

          if (z_typecmp(op, Z_TOKTYPE_NUMERIC)) {
            out[emitptr++] = op->numval;

          } else if (z_typecmp(op, Z_TOKTYPE_STRING)) {
            for (int j = 0; j < strlen(op->value); j++) {
              out[emitptr++] = op->value[j];
            }
          } else {
            z_fail(op, "Bad 'db' operand.\n");
            exit(1);
          }
        }

      } else if (z_streq(token->value, "dw")) {
        for (int i = 0; i < token->children_count; i++) {
          struct z_token_t *op = token->children[i];

          if (z_typecmp(op, Z_TOKTYPE_NUMERIC)) {
            out[emitptr++] = op->numval & 0xff;
            out[emitptr++] = op->numval >> 8;

          } else if (z_typecmp(op, Z_TOKTYPE_STRING)) {
            for (int j = 0; j < strlen(op->value); j++) {
              out[emitptr++] = op->value[j] & 0xff;
              out[emitptr++] = op->value[j] >> 8;
            }

          } else {
            z_fail(op, "Bad 'dw' operand.\n");
            exit(1);
          }
        }

      } else if (z_streq(token->value, "ds")) {
        struct z_token_t *sizeop = z_get_child(token, 0);

        uint8_t emitval = 0;
        if (token->children_count == 2) {
          struct z_token_t *emitop = z_get_child(token, 1);
          emitval = emitop->numval;
        }

        for (int i = 0; i < sizeop->numval; i++) {
          out[emitptr++] = emitval;
        }
      }
    }
  }

  return out;
}
