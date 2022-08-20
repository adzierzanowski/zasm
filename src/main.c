#include <stdio.h>
#include <unistd.h>

#include "structs.h"
#include "tokenizer.h"
#include "emitter.h"
#include "config.h"


void usage(void);
void print_tokens(struct z_token_t **tokens, size_t tokcnt);

int main(int argc, char *argv[]) {
  const char *fname = NULL;
  const char *ofname = NULL;

  char c;
  while ((c = getopt(argc, argv, "f:ho:v")) != -1) {
    switch(c) {
      case 'f':
        fname = optarg;
        break;

      case 'h':
        usage();
        exit(0);

      case 'o':
        ofname = optarg;
        break;

      case 'v':
        if (z_config.verbose) {
          z_config.very_verbose = true;

        } else {
          z_config.verbose = true;
        }
        break;
    }
  }

  if (fname == NULL) {
    fprintf(stderr, "\x1b[38;5;1mERROR: -f INPUT_FILENAME is a required switch.\x1b[0m\n");
    exit(1);
  }

  struct z_label_t *labels = NULL;
  struct z_def_t *defs = NULL;
  size_t tokcnt = 0;
  size_t bytepos = 0;
  struct z_token_t **tokens = tokenize(fname, &tokcnt, &labels, &defs, &bytepos);

  if (z_config.very_verbose) {
    if (labels) {
      printf("\x1b[38;5;4mLABELS\x1b[0m\n");
      struct z_label_t *ptr = labels;

      while (ptr != NULL) {
        printf("   %04x %s\n", ptr->value, ptr->key);
        ptr = ptr->next;
      }
    }

    if (defs) {
      printf("\n");
      printf("\x1b[38;5;4mDEFINES\x1b[0m\n");

      struct z_def_t *ptr = defs;
      while (ptr != NULL) {
        printf("  %s: %s\n", ptr->key, ptr->value->value);
        ptr = ptr->next;
      }
    }

    printf("\n");
    printf("\x1b[38;5;4m%zu TOKENS\x1b[0m\n", tokcnt);
    print_tokens(tokens, tokcnt);
  }

  size_t emitsz = 0;
  uint8_t *emitted = z_emit(tokens, tokcnt, &emitsz, labels, bytepos);

  if (z_config.verbose) {
    printf("\n\x1b[38;5;4mEMIT\x1b[0m\n    ");
    for (int i = 0; i < emitsz; i++) {
      printf("%02x ", emitted[i]);
      if ((i + 1) % 16 == 0) {
        printf("\n    ");
      }
    }
    printf("\n");
  }

  if (ofname) {
    FILE *of = fopen(ofname, "wb");
    fwrite(emitted, sizeof (uint8_t), emitsz, of);
    fclose(of);
  }

  tokens_free(tokens, tokcnt);
  labels_free(labels);
  defs_free(defs);
  free(emitted);

  return 0;
}

void usage() {
  puts("zasm");
  puts("    Zilog Z80 assembler.");
  puts("");
  puts("Usage: zasm -f INPUT [-o OUTPUT] [-v [-v]]");
  puts("");
  puts("Flags:");
  puts("    -f INPUT     input filename");
  puts("    -o OUTPUT    output filename");
  puts("    -v           be verbose");
  puts("    -vv          be very verbose");
}

void print_tokens(struct z_token_t **tokens, size_t tokcnt) {
  for (int i = 0; i < tokcnt; i++) {
    struct z_token_t *token = tokens[i];

    if (token->opcode && token->opcode->size > 0) {
      printf("\n     opcode[%zu] = ", token->opcode->size);
      for (int j = 0; j < token->opcode->size; j++) {
        printf("%02x ", token->opcode->bytes[j]);
      }
    }

    char codepos[0x1000] = {0};
    sprintf(
      codepos,
      "\x1b[38;5;245m%s:%d:%d\x1b[0m",
      token->fname,
      token->line+1,
      token->col+1);

    printf("\n%4d %-40s \x1b[38;5;1mroot    %s%-10s\x1b[0m      '%s%s\x1b[0m'\n",
      i,
      codepos,
      z_toktype_color(token->type),
      z_toktype_str(token->type),
      z_toktype_color(token->type),
      token->value);

    for (int j = 0; j < token->children_count; j++) {
      struct z_token_t *operand = token->children[j];
      sprintf(
        codepos,
        "\x1b[38;5;245m%s:%d:%d\x1b[0m",
        operand->fname,
        operand->line+1,
        operand->col+1);
      printf("     %-40s \x1b[38;5;4moper    %s─%s %s%c%-9s\x1b[0m '%s%s\x1b[0m' (%d)\n",
          codepos,
          j == token->children_count - 1 ? "└" : "├",
          operand->children_count ? "┬─" : "──",
          z_toktype_color(operand->type),
          operand->memref ? '*' : ' ',
          z_toktype_str(operand->type),
          z_toktype_color(operand->type),
          operand->value,
          operand->numval);

      for (int k = 0; k < operand->children_count; k++) {
        struct z_token_t *child = operand->children[k];
        sprintf(
          codepos, "\x1b[38;5;245m%s:%d:%d\x1b[0m", child->fname, child->line+1, child->col+1);
        printf("     %-40s         \x1b[38;5;4m%s %s─── %s%-8s\x1b[0m '%s%s\x1b[0m'\n",
            codepos,
            j == token->children_count - 1 ? " " : "│",
            k == operand->children_count - 1 ? "└" : "├",
            z_toktype_color(child->type),
            z_toktype_str(child->type),
            z_toktype_color(child->type),
            child->value);
      }
    }
  }
}
