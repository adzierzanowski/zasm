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

uint8_t *z_emit(struct z_token_t **tokens, size_t tokcnt, size_t *emitsz, struct z_label_t *labels) {
  uint8_t *out = NULL;
  *emitsz = 0;
  uint16_t origin = 0;

  for (int i = 0; i < tokcnt; i++) {
    struct z_token_t *token = tokens[i];

    if (token->type == Z_TOKTYPE_INSTRUCTION) {
      if (token->label_offset) {

        struct z_token_t *ptr = token->child;
        struct z_token_t *lblptr = NULL;
        while (ptr != NULL) {
          if (ptr->type == Z_TOKTYPE_IDENTIFIER) {
            lblptr = ptr;
            break;
          }
          ptr = ptr->child;
        }

        if (!ptr) {
          z_fail(token, "Couldn't find label token.\n");
          exit(1);
        }

        struct z_label_t *label = z_label_get(labels, lblptr->value);
        if (!label) {
          z_fail(token, "Couldn't resolve label '%s'.\n", lblptr->value);
          exit(1);
        }

        uint16_t lblval = label->value + origin;

        token->opcode->bytes[token->label_offset] = lblval & 0xff;
        token->opcode->bytes[token->label_offset+1] = lblval >> 8;
      }


      (*emitsz) += token->opcode->size;
      out = realloc(out, sizeof (uint8_t) * *emitsz);
      for (int i = 0; i < token->opcode->size; i++) {
        out[*emitsz - token->opcode->size + i] = token->opcode->bytes[i];
      }


    } else if (token->type == Z_TOKTYPE_DIRECTIVE) {
      if (z_streq(token->value, "org")) {
        struct z_token_t *org = token->child;
        origin = org->numval;

      } else if (z_streq(token->value, "ds")) {
        struct z_token_t *size = token->child;
        int fillsize = size->numval;
        (*emitsz) += fillsize;

        struct z_token_t *filler = size->child;
        uint8_t fillval = 0;

        if (filler) {
          if (z_typecmp(filler, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
            fillval = filler->numval;

          } else {
            z_fail(filler,
              "Fill value in the ds directive has to be a char or a number.\n");
          }
        }

        out = realloc(out, sizeof (uint8_t) * *emitsz);
        for (int i = 0; i < fillsize; i++) {
          out[*emitsz - fillsize + i] = fillval;
        }

      } else if (z_streq(token->value, "db") || z_streq(token->value, "dw")) {
        struct z_token_t *ptr = token->child;

        while (ptr != NULL) {
          if (ptr->type == Z_TOKTYPE_NUMBER) {
            uint16_t number = ptr->numval;

            if (z_streq(token->value, "db")) {
              (*emitsz)++;
              out = realloc(out, sizeof (uint8_t) * *emitsz);
              out[*emitsz - 1] = number;

            } else if (z_streq(token->value, "dw")) {
              (*emitsz) += 2;
              out = realloc(out, sizeof (uint8_t) * *emitsz);
              out[*emitsz - 2] = number & 0xff;
              out[*emitsz - 1] = number >> 8;
            }

          } else if (ptr->type == Z_TOKTYPE_STRING) {
            if (z_streq(token->value, "db")) {
              (*emitsz) += strlen(ptr->value);
              out = realloc(out, sizeof (uint8_t) * *emitsz);

              for (int i = 0; i < strlen(ptr->value); i++) {
                out[*emitsz - strlen(ptr->value) + i] = ptr->value[i];
              }

            } else if (z_streq(token->value, "dw")) {
              z_fail(ptr, "Can't use dw with strings \n");
              exit(1);
                /*
              (*emitsz) += strlen(ptr->value) * 2;
              out = realloc(out, sizeof (uint8_t) * *emitsz);

              for (int i = 0; i < strlen(ptr->value); i++) {
                out[*emitsz - strlen(ptr->value) + i * 2] = ptr->value[i] & 0xff;
                out[*emitsz - strlen(ptr->value) + i * 2 + 1] = ptr->value[i] >> 8;
              }
              */
            }

          } else if (ptr->type == Z_TOKTYPE_CHAR) {
            if (z_streq(token->value, "db")) {
              (*emitsz)++;
              out = realloc(out, sizeof (uint8_t *) * *emitsz);
              out[*emitsz - 1] = ptr->value[0];

            } else if (z_streq(token->value, "dw")) {
              (*emitsz) += 2;
              out = realloc(out, sizeof (uint8_t *) * *emitsz);
              out[*emitsz - 2] = ptr->value[0] & 0xff;
              out[*emitsz - 1] = ptr->value[0] >> 8;
            }
          }

          ptr = ptr->child;
        }
      }
    }
  }

  return out;
}
