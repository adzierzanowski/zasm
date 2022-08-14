#include "tokenizer.h"


struct z_token_t **tokenize(const char *fname, size_t *tokcnt, struct z_label_t **labels) {
  FILE *f = fopen(fname, "r");
  if (f == NULL) {
    fprintf(stderr, "Couldn't open '%s'.\n", fname);
    exit(1);
  }

  int codepos = 0;

  struct z_token_t **tokens = NULL;
  *tokcnt = 0;
  struct z_token_t *last_token = NULL;
  struct z_token_t *last_root_token = NULL;
  *labels = NULL;

  char tokbuf[TOKBUFSZ] = {0};
  int col = 0;
  int codecol = 0;

  size_t line = 0;
  uint16_t origin;

  bool last_char_isspace = false;
  bool in_string = false;
  bool in_char = false;
  bool in_memref = false;
  bool in_comment = false;
  uint32_t expected_type = Z_TOKTYPE_ANY;

  for (;;) {
    char c = fgetc(f);
  #if SHOW_TOKCHARS
    printf("c=%c [comment %d] [string %d] [memref %d] [char %d] [tokbuf '%s']\n", c, in_comment, in_string, in_memref, in_char, tokbuf);
  #endif

    codecol++;

    if (in_comment) {
      if (c == '\n') {
        line++;
        codepos = 0;
        in_comment = false;
      }
      continue;
    }

    if (!in_string && c == '"') {
      in_string = true;

    } else if (!in_char && c == '\'' && !z_streq(tokbuf, "af")) {
      in_char = true;

    } else if (!in_comment && c == ';') {
      in_comment = true;

    } else if (isspace(c) || z_indexof(":,\"[]'", c) > -1) {
      if (c == '\n') {
        line++;
        codecol = 0;
        in_comment = false;
      }

      if ((in_string && c != '"') || (in_char && c != '\'')) {
        tokbuf[col++] = c;
        continue;
      }

      if (c == '[') {
        in_memref = true;
        continue;
      }

      if (last_char_isspace && c != '[') {
        continue;
      }

      last_char_isspace = true;

      struct z_token_t *token = z_token_new(fname, line, codecol, tokbuf);

      if (c == ']' && in_memref) {
        token->memref = true;
        in_memref = false;
      }

      if (c == '"' && in_string) {
        token->type = Z_TOKTYPE_STRING;
        z_check_type(f, fname, line, codecol, expected_type, token);
        expected_type = Z_TOKTYPE_ANY;
        z_token_link(last_token, token);
        in_string = false;

      } else if (c == '\'' && in_char) {
        token->type = Z_TOKTYPE_CHAR;
        token->numval = (uint8_t) token->value[0];
        z_check_type(f, fname, line, codecol, expected_type, token);
        expected_type = Z_TOKTYPE_ANY;
        z_token_link(last_token, token);
        in_char = false;

      } else if (c == ':') {
        token->type = Z_TOKTYPE_LABEL;
        z_check_type(f, fname, line, codecol, expected_type, token);
        expected_type = Z_TOKTYPE_ANY;
        z_parse_root(last_root_token, &codepos, labels);
        last_root_token = token;
        z_token_add(&tokens, tokcnt, token);

      } else if (z_strmatch(
          tokbuf,
          "nop", "rlca", "rrca", "rla", "rra", "daa", "cpl", "scf", "ccf",
          "halt", "ei", "di", "ldi", "cpi", "ini", "outi", "ldd", "cpd", "ind",
          "outd", "ldir", "cpir", "inir", "otir", "lddr", "cpdr", "indr", "otdr",
          "neg", "retn", "reti", "rrd", "rld",
          NULL)) {
        token->type = Z_TOKTYPE_INSTRUCTION;
        z_check_type(f, fname, line, codecol, expected_type, token);
        expected_type = Z_TOKTYPE_ANY;
        z_parse_root(last_root_token, &codepos, labels);
        last_root_token = token;
        z_token_add(&tokens, tokcnt, token);

      } else if (z_strmatch(
          tokbuf,
          "ld", "inc", "dec", "ex", "add", "djnz", "jr", "adc", "sub", "sbc",
          "and", "or", "xor", "cp", "ret", "pop", "push", "rst", "exx", "jp",
          "call", "out", "in", "im",
          "rlc", "rrc", "rl", "rr", "sla", "sra", "srl", "bit", "res", "set",
          NULL)) {
        token->type = Z_TOKTYPE_INSTRUCTION;
        z_check_type(f, fname, line, codecol, expected_type, token);

        if (z_streq(tokbuf, "ld")) {
          expected_type = Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER | Z_TOKTYPE_REGISTER_8 | Z_TOKTYPE_REGISTER_16;

        } else if (z_streq(tokbuf, "inc")) {
          expected_type = Z_TOKTYPE_REGISTER_8 | Z_TOKTYPE_REGISTER_16;

        } else {
          expected_type = Z_TOKTYPE_ANY;
        }

        z_parse_root(last_root_token, &codepos, labels);
        last_root_token = token;
        z_token_add(&tokens, tokcnt, token);

      } else if (z_strmatch(tokbuf, "org", "db", "dw", "ds", "include", "def", NULL)) {
        token->type = Z_TOKTYPE_DIRECTIVE;
        z_check_type(f, fname, line, codecol, expected_type, token);

        if (z_streq(tokbuf, "org")) {
          expected_type = Z_TOKTYPE_NUMBER;

        } else if (z_strmatch(tokbuf, "db", "dw", "ds", NULL)) {
          expected_type = Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER | Z_TOKTYPE_STRING;

        } else if (z_streq(tokbuf, "include")) {
          expected_type = Z_TOKTYPE_STRING;

        } else if (z_streq(tokbuf, "def")) {
          expected_type = Z_TOKTYPE_IDENTIFIER;

        } else {
          expected_type = Z_TOKTYPE_ANY;
        }

        z_parse_root(last_root_token, &codepos, labels);
        last_root_token = token;
        z_token_add(&tokens, tokcnt, token);

      } else if (z_strmatch(tokbuf, "bc", "de", "hl", "sp", "ix", "iy", "af", "af'", NULL)) {
        token->type = Z_TOKTYPE_REGISTER_16;
        z_check_type(f, fname, line, codecol, expected_type, token);
        expected_type = Z_TOKTYPE_ANY;
        z_token_link(last_token, token);

      } else if (z_streq(tokbuf, "c")) {
        if (z_strmatch(last_root_token->value, "jr", "ret", "call", "jp", NULL)) {
          token->type = Z_TOKTYPE_CONDITION;
        } else {
          token->type = Z_TOKTYPE_REGISTER_8;
        }

        z_check_type(f, fname, line, codecol, expected_type, token);
        expected_type = Z_TOKTYPE_ANY;
        z_token_link(last_token, token);

      } else if (z_strmatch(tokbuf, "a", "b", "d", "e", "h", "l", "i", "r", NULL)) {
        token->type = Z_TOKTYPE_REGISTER_8;
        z_check_type(f, fname, line, codecol, expected_type, token);
        expected_type = Z_TOKTYPE_ANY;
        z_token_link(last_token, token);

      } else if (z_strmatch(tokbuf, "z", "nz", "p", "m", "po", "pe", "c", "nc", NULL)) {
        token->type = Z_TOKTYPE_CONDITION;
        z_check_type(f, fname, line, codecol, expected_type, token);
        expected_type = Z_TOKTYPE_ANY;
        z_token_link(last_token, token);

      } else {
        char *endptr = NULL;
        uint32_t num = strtoul(tokbuf, &endptr, 0);
        //z_dprintf("Possible number: val=%s num=%u (endptr==tokbuf? %s)\n", tokbuf, num, endptr == tokbuf ? "true" : "false");

        if ((num == 0 && endptr != tokbuf) || num) {
          token->type = Z_TOKTYPE_NUMBER;
          token->numval = num;
          z_check_type(f, fname, line, codecol, expected_type, token);
          expected_type = Z_TOKTYPE_ANY;
          z_token_link(last_token, token);

          if (token->parent->type == Z_TOKTYPE_DIRECTIVE && z_streq(token->parent->value, "org")) {
            origin = num;
          }

        } else if (z_streq(tokbuf, "$")) {
          token->type = Z_TOKTYPE_NUMBER;
          token->numval = codepos + origin;
          z_token_link(last_token, token);

        } else {
          token->type = Z_TOKTYPE_IDENTIFIER;
          z_check_type(f, fname, line, codecol, expected_type, token);
          expected_type = Z_TOKTYPE_ANY;
          z_token_link(last_token, token);
        }
      }

#ifdef SHOW_NEW_TOKENS
      z_dprintf(fname, line, col,
        "Tokenizer: NEW TOKEN: %s TYPE: %s\n\n",
        token->value, z_toktype_str(token->type));
#endif

      memset(tokbuf, 0, TOKBUFSZ);
      col = 0;
      last_token = token;

    } else if (c == ',') {
      last_char_isspace = true;

    } else {
      last_char_isspace = false;
      tokbuf[col++] = c;

      if (in_char && strlen(tokbuf) > 1) {
        z_fail(fname, line, codecol, "Character has more than one byte: '%s'.", tokbuf);
        exit(1);
      }
    }

    if (c == EOF) {
      // TODO: make sure this doesn't fail
      z_parse_root(last_root_token, &codepos, labels);

      z_dprintf(fname, line, codecol, "End of file\n");
      fclose(f);

      printf("\n\nLABELS\n\n");
      struct z_label_t *ptr = *labels;
      while (ptr != NULL) {
        printf("LABEL %s = %hu\n", ptr->key, ptr->value);
        ptr = ptr->next;
      }
      printf("\n\n");

      return tokens;
    }
  }

  fclose(f);



  return tokens;
}

const char *z_toktype_str(enum z_toktype_t type) {
  switch (type) {
    case Z_TOKTYPE_NONE: return "(none)";
    case Z_TOKTYPE_DIRECTIVE: return "DIR";
    case Z_TOKTYPE_INSTRUCTION: return "INS";
    case Z_TOKTYPE_IDENTIFIER: return "ID";
    case Z_TOKTYPE_LABEL: return "LBL";
    case Z_TOKTYPE_NUMBER: return "NUM";
    case Z_TOKTYPE_REGISTER_8: return "REG8";
    case Z_TOKTYPE_REGISTER_16: return "REG16";
    case Z_TOKTYPE_CONDITION: return "COND";
    case Z_TOKTYPE_STRING: return "STR";
    case Z_TOKTYPE_CHAR: return "CHR";
    case Z_TOKTYPE_ANY: return "(any)";
  }
}

struct z_token_t *z_token_new(const char *fname, size_t line, int col, char *value) {
  struct z_token_t *token = malloc(sizeof (struct z_token_t));

  strcpy(token->value, value);
  token->memref = false;
  token->fname = fname;
  token->line = line;
  token->col = col;
  token->child = NULL;
  token->parent = NULL;
  token->numval = 0;
  token->opcode = NULL;
  token->label_offset = 0;

  return token;
}

void z_check_type(
    FILE *f, const char *fname, size_t line, int col, uint32_t expected, struct z_token_t *token) {


  if (!(expected & token->type)) {
    z_fail(
      fname, line, col,
      "Unexpected token: '%s' (%s). Expected one of:\n",
      token->value, z_toktype_str(token->type), expected);
    for (int64_t i = 1; i < Z_TOKTYPE_ANY; i *= 2) {
      if (i & expected) {
        fprintf(stderr, "  - %s\n", z_toktype_str(i));
      }
    }

    fclose(f);
    exit(1);
  }
}

void z_token_add(struct z_token_t ***tokens, size_t *tokcnt, struct z_token_t *token) {
  (*tokcnt)++;
  *tokens = realloc(*tokens, sizeof (struct z_token_t *) * *tokcnt);
  (*tokens)[*tokcnt - 1] = token;
}

void z_parse_root(struct z_token_t *token, int *codepos, struct z_label_t **labels) {
  if (!token) return;

  if (token->type == Z_TOKTYPE_INSTRUCTION) {
    struct z_opcode_t *opcode = z_opcode_match(token);
    token->opcode = opcode;
    *codepos += opcode->size;

  } else if (token->type == Z_TOKTYPE_LABEL) {
    struct z_label_t *label = z_label_new(token->value, *codepos);
    z_label_add(labels, label);

  } else if (token->type == Z_TOKTYPE_DIRECTIVE) {
    struct z_token_t *ptr = token->child;

    while (ptr != NULL) {
      if (ptr->type == Z_TOKTYPE_CHAR) {
        (*codepos)++;

      } else if (ptr->type == Z_TOKTYPE_STRING) {
        (*codepos) += strlen(ptr->value);

      } else if (ptr->type == Z_TOKTYPE_NUMBER) {
        if (z_streq(token->value, "db")) {
          (*codepos)++;
        } else if (z_streq(token->value, "dw")) {
          (*codepos) += 2;
        }
      }

      ptr = ptr->child;
    }
  }
}

void z_token_link(struct z_token_t *parent, struct z_token_t *child) {
  if (!parent) {
    z_fail(child->fname, child->line, child->col, "No parent token to link to.\n");
    exit(1);
  }

  parent->child = child;
  child->parent = parent;
}

struct z_label_t *z_label_new(char *key, uint16_t value) {
  struct z_label_t *label = malloc(sizeof (struct z_label_t));
  strcpy(label->key, key);
  label->value = value;
  label->next = NULL;
  return label;
}

void z_label_add(struct z_label_t **labels, struct z_label_t *label) {
  struct z_label_t *ptr = *labels;
  if (!ptr) {
    *labels = label;
    return;
  }

  while (ptr->next != NULL) {
    ptr = ptr->next;
  }
  ptr->next = label;
}

bool z_typecmp(struct z_token_t *token, int types) {
  return (token->type & types);
}
