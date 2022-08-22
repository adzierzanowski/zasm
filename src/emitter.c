#include "emitter.h"


uint8_t *z_emit(
    struct z_token_t **tokens,
    size_t tokcnt,
    size_t *emitsz,
    struct z_label_t *labels,
    struct z_def_t *defs,
    size_t bytepos) {
  uint8_t *out = calloc(bytepos, sizeof (uint8_t));

  *emitsz = bytepos;

  int emitptr = 0;
  uint16_t origin = 0;

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

            if (z_typecmp(operand, Z_TOKTYPE_EXPRESSION)) {
              z_expr_eval(operand, labels, defs, origin);

            } else if (z_typecmp(operand, Z_TOKTYPE_IDENTIFIER)) {
              struct z_label_t *label = z_label_get(labels, operand->value);
              if (label) {
                operand->numval = origin + label->value;
              } else {
                z_fail(operand, "Couldn't resolve label: '%s'.\n", operand->value);
                #ifndef DEBUG
                exit(1);
                #endif
              }
            }

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
      if (z_streq(token->value, "org")) {
        if (token->children_count != 1) {
          z_fail(token, "'org' directive requires an operand.\n");
          exit(1);
        }

        struct z_token_t *op = z_get_child(token, 0);

        if (z_typecmp(op, Z_TOKTYPE_NUMBER)) {
          origin = op->numval & 0xffff;

        } else {
          z_fail(
            op,
            "'org' directive operand should be a number, got %s instead.\n",
            z_toktype_str(op->type));
          exit(1);
        }

      } else if (z_streq(token->value, "db")) {
        for (int i = 0; i < token->children_count; i++) {
          struct z_token_t *op = token->children[i];

          if (z_typecmp(op, Z_TOKTYPE_EXPRESSION)) {
            z_expr_eval(op, labels, defs, origin);
            out[emitptr++] = op->numval & 0xff;

          } else if (z_typecmp(op, Z_TOKTYPE_NUMERIC)) {
            if (z_typecmp(op, Z_TOKTYPE_IDENTIFIER))  {
              int *numval = z_lbldef_resolve(labels, defs, origin, op->value);
              if (numval) {
                out[emitptr++] = *numval & 0xff;
                free(numval);

              } else {
                z_fail(op, "Couldn't resolve identifier '%s'\n", op->value);
              }

            } else {
              out[emitptr++] = op->numval & 0xff;
            }

          } else if (z_typecmp(op, Z_TOKTYPE_STRING)) {
            for (int j = 0; j < strlen(op->value); j++) {
              out[emitptr++] = op->value[j] & 0xff;
            }

          } else {
            z_fail(op, "Bad 'db' operand.\n");
            exit(1);
          }
        }

      } else if (z_streq(token->value, "dw")) {
        for (int i = 0; i < token->children_count; i++) {
          struct z_token_t *op = token->children[i];

          if (z_typecmp(op, Z_TOKTYPE_EXPRESSION)) {
            z_expr_eval(op, labels, defs, origin);
            out[emitptr++] = op->numval & 0xff;
            out[emitptr++] = op->numval >> 8;

          } else if (z_typecmp(op, Z_TOKTYPE_NUMERIC)) {
            if (z_typecmp(op, Z_TOKTYPE_IDENTIFIER))  {
              int *numval = z_lbldef_resolve(labels, defs, origin, op->value);
              if (numval) {
                out[emitptr++] = *numval & 0xff;
                out[emitptr++] = *numval >> 8;
                free(numval);

              } else {
                z_fail(op, "Couldn't resolve identifier '%s'\n", op->value);
              }

            } else {
              out[emitptr++] = op->numval & 0xff;
              out[emitptr++] = op->numval >> 8;
            }

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

        if (z_typecmp(sizeop, Z_TOKTYPE_EXPRESSION)) {
          z_expr_eval(sizeop, labels, defs, origin);
        }

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
