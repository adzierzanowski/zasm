#ifndef EMITTER_H
#define EMITTER_H


#include "structs.h"

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
          z_fail(token->fname, token->line, token->col, "Couldn't find label token.\n");
          exit(1);
        }

        struct z_label_t *label = z_label_get(labels, lblptr->value);
        if (!label) {
          z_fail(token->fname, token->line, token->col, "Couldn't resolve label '%s'.\n", lblptr->value);
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
      } else if (z_streq(token->value, "db")) {
        struct z_token_t *ptr = token->child;

        while (ptr != NULL) {
          if (ptr->type == Z_TOKTYPE_NUMBER) {
            uint8_t number = ptr->numval;
            (*emitsz)++;
            out = realloc(out, sizeof (uint8_t) * *emitsz);
            out[*emitsz - 1] = number;

          } else if (ptr->type == Z_TOKTYPE_STRING) {
            (*emitsz) += strlen(ptr->value);
            out = realloc(out, sizeof (uint8_t) * *emitsz);

            for (int i = 0; i < strlen(ptr->value); i++) {
              out[*emitsz - strlen(ptr->value) + i] = ptr->value[i];
            }

          } else if (ptr->type == Z_TOKTYPE_CHAR) {
            (*emitsz)++;
            out = realloc(out, sizeof (uint8_t *) * *emitsz);
            out[*emitsz - 1] = ptr->value[0];

          }

          ptr = ptr->child;
        }
      }
    }
  }

  return out;
}


#endif
