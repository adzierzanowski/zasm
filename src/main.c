
#include "main.h"


int main(int argc, char *argv[]) {
  const char *fname = NULL;
  const char *ofname = NULL;
  const char *efname = NULL;
  const char *lfname = NULL;
  bool export_defs = false;

  struct argparser_t *parser = argparser_new("zasm");
  struct option_init_t opt = {0};

  opt.short_name = "-d";
  opt.long_name = "--export-defs";
  opt.help = "export numeric defines";
  opt.required = false;
  opt.takes_arg = false;
  argparser_from_struct(parser, &opt);

  opt.short_name = "-e";
  opt.long_name = "--export-labels";
  opt.help = "export labels to a file";
  opt.required = false;
  opt.takes_arg = true;
  argparser_from_struct(parser, &opt);

  opt.short_name = "-h";
  opt.long_name = "--help";
  opt.help = "show this help message and exit";
  opt.required = false;
  opt.takes_arg = false;
  argparser_from_struct(parser, &opt);

  opt.short_name = "-l";
  opt.long_name = "--import-labels";
  opt.help = "import labels from a file";
  opt.required = false;
  opt.takes_arg = true;
  argparser_from_struct(parser, &opt);

  opt.short_name = "-o";
  opt.long_name = "--output";
  opt.help = "output filename";
  opt.required = false;
  opt.takes_arg = true;
  argparser_from_struct(parser, &opt);

  opt.short_name = "-v";
  opt.long_name = "--verbosity";
  opt.help = "verbosity_level";
  opt.required = false;
  opt.takes_arg = true;
  argparser_from_struct(parser, &opt);

  argparser_parse(parser, argc, argv);

  if (argparser_passed(parser, "-h")) {
    argparser_usage(parser);
  }

  export_defs = argparser_passed(parser, "-d");
  efname = argparser_get(parser, "-e");
  lfname = argparser_get(parser, "-l");
  ofname = argparser_get(parser, "-o");

  int vlevel = 0;
  if (argparser_passed(parser, "-v")) {
    vlevel = atoi(argparser_get(parser, "-v"));
  }
  z_config.verbose = vlevel > 0;
  z_config.very_verbose = vlevel > 1;

  if (parser->positional_count < 2) {
    z_fail(NULL, "Input filename is required.\n");
    exit(1);
  }

  fname = parser->positional[1];
  argparser_free(parser);

  struct z_label_t *labels = NULL;

  if (lfname) {
    FILE *f = fopen(lfname, "r");
    labels = z_labels_import(f);
    fclose(f);
  }

  struct z_def_t *defs = NULL;
  size_t tokcnt = 0;
  size_t bytepos = 0;
  struct z_token_t **tokens = z_tokenize(fname, &tokcnt, &labels, &defs, &bytepos);


  if (z_config.verbose) {
    if (labels) {
      printf("\x1b[38;5;4mLABELS\x1b[0m\n");
      struct z_label_t *ptr = labels;

      while (ptr != NULL) {
        printf("  %04x %s\n", ptr->value, ptr->key);
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

  }

  size_t emitsz = 0;
  uint8_t *emitted = z_emit(tokens, tokcnt, &emitsz, labels, defs, bytepos);

  if (z_config.very_verbose) {
    printf("\n");
    printf("\x1b[38;5;4m%zu TOKENS\x1b[0m\n", tokcnt);
    z_print_tokens(tokens, tokcnt);
  }

  if (z_config.verbose) {
    printf("\n\x1b[38;5;4mEMIT\x1b[0m\n  ");
    for (int i = 0; i < emitsz; i++) {
      printf("%02x ", emitted[i]);
      if ((i + 1) % 16 == 0) {
        printf("\n  ");
      }
    }
    printf("\n");
  }

  if (ofname) {
    FILE *of = fopen(ofname, "wb");
    fwrite(emitted, sizeof (uint8_t), emitsz, of);
    fclose(of);
  }

  if (efname) {
    FILE *ef = fopen(efname, "wa");
    z_labels_export(ef, labels, export_defs ? defs : NULL);
    fclose(ef);
  }

  z_tokens_free(tokens, tokcnt);
  z_labels_free(labels);
  z_defs_free(defs);
  free(emitted);

  return 0;
}

void z_print_tokens(struct z_token_t **tokens, size_t tokcnt) {
  for (int i = 0; i < tokcnt; i++) {
    struct z_token_t *token = tokens[i];

    if (token->opcode && token->opcode->size > 0) {
      printf(
        "\n     \x1b[38;5;21m %04x \x1b[0m opcode[%zu] = \x1b[1m\x1b[38;5;213m",
        token->codepos,
        token->opcode->size);

      for (int j = 0; j < token->opcode->size; j++) {
        if (j > 20) {
          printf("...");
          break;
        }

        if (j == token->label_offset && token->label_offset != 0) {
          printf("\x1b[0m");
        }

        printf("%02x ", token->opcode->bytes[j]);
      }
      printf("\x1b[0m");

    } else {
      printf("\n     \x1b[38;5;21m %04x \x1b[0m", token->codepos);
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
      printf("     %-40s \x1b[38;5;4moper    %s─%s %s%c%-9s\x1b[0m '%s%s\x1b[0m' "
             "(\x1b[38;5;245mhex\x1b[0m %04x \x1b[38;5;245mdec\x1b[0m %d)\n",
          codepos,
          j == token->children_count - 1 ? "└" : "├",
          operand->children_count ? "┬─" : "──",
          z_toktype_color(operand->type),
          operand->memref ? '*' : ' ',
          z_toktype_str(operand->type),
          z_toktype_color(operand->type),
          operand->value,
          operand->numval,
          operand->numval);

      for (int k = 0; k < operand->children_count; k++) {
        struct z_token_t *child = operand->children[k];
        sprintf(
          codepos,
          "\x1b[38;5;245m%s:%d:%d\x1b[0m",
          child->fname,
          child->line+1,
          child->col+1);
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
