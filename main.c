#include <stdio.h>
#include <unistd.h>

#include "structs.h"
#include "tokenizer.h"
#include "emitter.h"

int main(int argc, char *argv[]) {
  const char *fname = NULL;
  const char *ofname = NULL;

  char c;
  while ((c = getopt(argc, argv, "f:o:")) != -1) {
    switch(c) {
      case 'f':
        fname = optarg;
        break;

      case 'o':
        ofname = optarg;
        break;
    }
  }

  if (fname == NULL) {
    fprintf(stderr, "-f INPUT_FILENAME is a required switch");
    exit(1);
  }

  struct z_label_t *labels = NULL;
  size_t tokcnt = 0;
  struct z_token_t **tokens = tokenize(fname, &tokcnt, &labels);

  z_dprintf(fname, 0, 0, "%zu TOKENS:\n", tokcnt);

  for (int i = 0; i < tokcnt; i++) {
    struct z_token_t *token = tokens[i];

    z_dprintf(
      token->fname, token->line, token->col,
      "    %3d %-20s%c: %s\n", i, z_toktype_str(token->type), token->memref ? '*' : ' ', token->value);

    struct z_token_t *ptr = token->child;

    if (token->opcode) {
      printf("    OPCODE[%zu] = ", token->opcode->size);
      for (int i = 0; i < token->opcode->size; i++) {
        printf("%02x ", token->opcode->bytes[i]);
      }
      printf("\n");

    }

    while (ptr != NULL) {
      z_dprintf(
        ptr->fname, ptr->line, ptr->col,
        "            %-20s%c: %s\n", z_toktype_str(ptr->type), ptr->memref ? '*' : ' ', ptr->value);
      ptr = ptr->child;
    }

  }

  size_t emitsz = 0;
  uint8_t *emitted = z_emit(tokens, tokcnt, &emitsz, labels);

#ifdef SHOW_EMIT
  puts("\nEMIT:");
  for (int i = 0; i < emitsz; i++) {
    printf("%02x ", emitted[i]);
  }
  printf("\n");
#endif

  if (ofname) {
    FILE *of = fopen(ofname, "wb");
    fwrite(emitted, sizeof (uint8_t), emitsz, of);
    fclose(of);
  }

  return 0;
}
