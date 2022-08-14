#include <stdio.h>
#include <unistd.h>

#include "structs.h"
#include "tokenizer.h"
#include "emitter.h"

#include "config.h"

void usage(void);

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
  size_t tokcnt = 0;
  struct z_token_t **tokens = tokenize(fname, &tokcnt, &labels);

  if (z_config.very_verbose) {
    printf("\x1b[38;5;4mTOKENS\x1b[0m\n");

    for (int i = 0; i < tokcnt; i++) {
      struct z_token_t *token = tokens[i];

      char buf[0x1000] = {0};
      sprintf(buf, "\x1b[38;5;4m%s:%lu:%u\x1b[0m",
        token->fname,
        token->line + 1,
        token->col + 1);

      printf(
        "%-30s    %3d %s%-15s %c%-30s\x1b[0m",
        buf,
        i,
        z_toktype_color(token->type),
        z_toktype_str(token->type),
        token->memref ? '*' : ' ', token->value);

      if (token->opcode) {
        for (int i = 0; i < token->opcode->size; i++) {
          printf("%02x ", token->opcode->bytes[i]);
        }
      }
      printf("\n");

      struct z_token_t *ptr = token->child;
      while (ptr != NULL) {
        sprintf(buf, "\x1b[38;5;4m%s:%lu:%u\x1b[0m",
          ptr->fname,
          ptr->line + 1,
          ptr->col + 1);

        printf(
          "%-30s          %s%-18s %c%s%c\x1b[0m\n",
          buf,
          z_toktype_color(ptr->type),
          z_toktype_str(ptr->type),
          ptr->memref ? '[' : ' ',
          ptr->value,
          ptr->memref ? ']' : ' '
        );
        ptr = ptr->child;
      }
    }
  }

  size_t emitsz = 0;
  uint8_t *emitted = z_emit(tokens, tokcnt, &emitsz, labels);

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
