#include "tokenizer.h"


struct z_token_t **tokenize(
  const char *fname, size_t *tokcnt, struct z_label_t **labels, size_t *bytepos) {

  FILE *f = fopen(fname, "r");

  struct z_token_t **tokens = NULL;

  char tokbuf[TOKBUFSZ] = {0};
  int tokbufptr = 0;
  int line = 0;
  int col = 0;
  *bytepos = 0;

  bool in_comment = false;
  bool in_string = false;
  bool in_char = false;
  bool in_memref = false;
  bool opsep = false;

  struct z_token_t *root = NULL;
  struct z_token_t *operand = NULL;

  int c = 0;
  while (c != EOF) {
    c = fgetc(f);
    col++;

    struct z_token_t *token = NULL;


    if (in_string) {
      if (c == '"') {
        if (strlen(tokbuf) > 0) {
          token = z_token_new(fname, line, col, tokbuf, Z_TOKTYPE_STRING);
          memset(tokbuf, 0, TOKBUFSZ);
          tokbufptr = 0;
        }
        in_string = false;

      } else {
        tokbuf[tokbufptr++] = c;
      }

    } else if (in_char) {
      if (c == '\'') {
        in_char = false;

        if (strlen(tokbuf) > 0) {
          token = z_token_new(fname, line, col, tokbuf, Z_TOKTYPE_CHAR);
          token->numval = tokbuf[0];
          memset(tokbuf, 0, TOKBUFSZ);
          tokbufptr = 0;
        }

      } else {
        tokbuf[tokbufptr++] = c;
      }

    } else if (in_comment) {
      //continue;


    } else if (c == '"') {
      in_string = true;
      //continue;

    } else if (c == '\'') {
      if (z_streq(tokbuf, "af")) {
        // PASS

      } else {
        in_char = true;
      }

      //continue;
    } else if (c == '[') {
      in_memref = true;

    } else if (isalnum(c)) {
      tokbuf[tokbufptr++] = c;

    } else if (c == ';') {
      in_comment = true;

    } else if (z_indexof("+-*/()~^&|%", c) > -1) {
      if (strlen(tokbuf) > 0) {
        token = z_token_new(fname, line, col, tokbuf, Z_TOKTYPE_NONE);
        z_token_add_child(operand, token);
      }

      char buf[2] = { c, 0 };
      token = z_token_new(fname, line, col, buf, Z_TOKTYPE_OPERATOR);

    } else if (c == ',') {
      if (strlen(tokbuf) > 0) {
        token = z_token_new(fname, line, col, tokbuf, Z_TOKTYPE_NONE);
      }

    } else if (isspace(c) || c == ']') {
      if (strlen(tokbuf) > 0) {
        token = z_token_new(fname, line, col, tokbuf, Z_TOKTYPE_NONE);
      }

    } else if (c == ':') {
      if (strlen(tokbuf) > 0) {
        token = z_token_new(fname, line, col, tokbuf, Z_TOKTYPE_LABEL);
      }
    } else if (c == '$') {
      tokbuf[tokbufptr++] = c;
      if (strlen(tokbuf) == 1) {
        token = z_token_new(fname, line, col, tokbuf, Z_TOKTYPE_NUMBER);
        token->numval = *bytepos;
      }
    }

    if (token != NULL) {
      if (in_memref) {
        token->memref = true;
      }
      memset(tokbuf, 0, TOKBUFSZ);
      tokbufptr = 0;

      //printf("\x1b[38;5;1mADDING TOKEN %s\x1b[0m\n", token->value);
      //printf("\x1b[38;5;1m  OPSEP %d\x1b[0m\n", opsep);

      if (z_typecmp(token,
          Z_TOKTYPE_DIRECTIVE | Z_TOKTYPE_INSTRUCTION | Z_TOKTYPE_LABEL)) {
        z_token_add(&tokens, tokcnt, token);

        z_parse_root(root, bytepos, labels);

        root = token;
      //printf("\x1b[38;5;1m  ROOT\x1b[0m\n");

      } else if (opsep || !root->children_count) {
      //printf("\x1b[38;5;1m  OPERAND\x1b[0m\n");
        z_token_add_child(root, token);

        if (z_streq(token->value, "c") &&
            z_typecmp(token, Z_TOKTYPE_REGISTER_8) &&
            z_strmatch(root->value, "call", "ret", "jp", "jr", NULL)) {
          token->type = Z_TOKTYPE_CONDITION;
        }

        operand = token;
        opsep = false;

      } else {
      //printf("\x1b[38;5;1m  OPCHILD\x1b[0m\n");
        z_token_add_child(operand, token);
      }
    }

    if (c == ',') {
      opsep = true;

    } else if (in_memref && c == ']') {
      in_memref =  false;

    } else if (c == '\n') {
      line++;
      col = 0;
      in_comment = false;
      in_memref = false;
      in_char= false;
    }

    //char tokbufbuf[0x1000] = {0};
    //sprintf(tokbufbuf, "'%s'", tokbuf);
    //printf("c = %c  tokbuf %-20s   com %d   str %d   chr %d   ref %d  opsep %d\n",
    //    c, tokbufbuf, in_comment, in_string, in_char, in_memref, opsep);
  }

  z_parse_root(root, bytepos, labels);

  return tokens;
}

const char *z_toktype_str(enum z_toktype_t type) {
  switch (type) {
    case Z_TOKTYPE_NONE: return "(none)";
    case Z_TOKTYPE_DIRECTIVE: return "DIREC";
    case Z_TOKTYPE_INSTRUCTION: return "INSTR";
    case Z_TOKTYPE_IDENTIFIER: return "IDENT";
    case Z_TOKTYPE_LABEL: return "LABEL";
    case Z_TOKTYPE_NUMBER: return "NUMBR";
    case Z_TOKTYPE_REGISTER_8: return "REG8";
    case Z_TOKTYPE_REGISTER_16: return "REG16";
    case Z_TOKTYPE_CONDITION: return "COND";
    case Z_TOKTYPE_STRING: return "STRING";
    case Z_TOKTYPE_CHAR: return "CHAR";
    case Z_TOKTYPE_OPERATOR: return "OPER";
    case Z_TOKTYPE_EXPRESSION: return "EXPR";
    case Z_TOKTYPE_ANY: return "(any)";
  }
}

const char *z_toktype_color(enum z_toktype_t type) {
  switch (type) {
    case Z_TOKTYPE_NONE: return "";
    case Z_TOKTYPE_DIRECTIVE: return "\x1b[38;5;92m";
    case Z_TOKTYPE_INSTRUCTION: return "\x1b[38;5;213m";
    case Z_TOKTYPE_IDENTIFIER: return "\x1b[38;5;4m";
    case Z_TOKTYPE_LABEL: return "\x1b[38;5;4m";
    case Z_TOKTYPE_NUMBER: return "\x1b[38;5;202m";
    case Z_TOKTYPE_EXPRESSION: return "\x1b[38;5;202m";
    case Z_TOKTYPE_REGISTER_8: return "\x1b[38;5;220m";
    case Z_TOKTYPE_REGISTER_16: return "\x1b[38;5;220m";
    case Z_TOKTYPE_CONDITION: return "\x1b[38;5;207m";
    case Z_TOKTYPE_STRING: return "\x1b[38;5;2m";
    case Z_TOKTYPE_CHAR: return "\x1b[38;5;34m";
    case Z_TOKTYPE_OPERATOR: return "\x1b[38;5;159m";
    case Z_TOKTYPE_ANY: return "(any)";
  }
}

struct z_token_t *z_token_new(
    const char *fname, size_t line, int col, char *value, int type) {
  struct z_token_t *token = calloc(1, sizeof (struct z_token_t));

  strcpy(token->value, value);
  token->type = type;
  token->memref = false;
  token->fname = fname;
  token->line = line;
  token->col = col - strlen(value) - 1;
  token->left_associative = true;

  if (token->type == Z_TOKTYPE_NONE) {
    if (z_strmatch(value, "bc", "de", "hl", "sp", "ix", "iy", "af", NULL)) {
      token->type = Z_TOKTYPE_REGISTER_16;

    } else if (
        z_strmatch(value, "a", "b", "c", "d", "e", "h", "l", "i", "r", NULL)) {
      token->type = Z_TOKTYPE_REGISTER_8;

    } else if (z_strmatch(value, "z", "nz", "c", "nc", "po", "pe", "p", "m", NULL)) {
      token->type = Z_TOKTYPE_CONDITION;

    } else if (z_strmatch(value,
        "ld", "push", "pop", "ex", "exx", "ldi", "ldir", "ldd", "lddr", "cpi",
        "cpir", "cpd", "cpdr", "add", "adc", "sub", "sbc", "and", "or", "xor",
        "cp", "inc", "dec", "daa", "cpl", "neg", "ccf", "scf", "nop", "halt",
        "di", "ei", "im", "rlca", "rla", "rrca", "rra", "rlc", "rl", "rrc",
        "rr", "sla", "sra", "srl", "rld", "rrd", "bit", "set", "res", "jp",
        "djnz", "call", "ret", "reti", "retn", "rst", "in", "ini", "inir",
        "ind", "indr", "out", "outi", "otir", "outd", "otdr", "jr", NULL)) {
      token->type = Z_TOKTYPE_INSTRUCTION;

    } else if (
        z_strmatch(value, "ds", "dw", "db", "def", "include", "org", NULL)) {
      token->type = Z_TOKTYPE_DIRECTIVE;

    } else if (isdigit(value[0])) {
      char *endptr = NULL;
      int numval = strtoul(value, &endptr, 0);
      token->type = Z_TOKTYPE_NUMBER;
      token->numval = numval;

    } else {
      token->type = Z_TOKTYPE_IDENTIFIER;
    }

  } else if (token->type == Z_TOKTYPE_OPERATOR) {
    switch (token->value[0]) {
      case '(':
      case ')':
        token->precedence = 1;
        break;
      case '~':
        token->precedence = 2;
        break;
      case '*':
      case '/':
      case '%':
        token->precedence = 3;
        break;
      case '+':
      case '-':
        token->precedence = 4;
        break;
      case '&':
        token->precedence = 5;
        break;
      case '^':
        token->precedence = 6;
        break;
      case '|':
        token->precedence = 7;
        break;
    }
  }

  return token;
}

void z_token_add(struct z_token_t ***tokens, size_t *tokcnt, struct z_token_t *token) {
  (*tokcnt)++;
  *tokens = realloc(*tokens, sizeof (struct z_token_t *) * *tokcnt);
  (*tokens)[*tokcnt - 1] = token;
}

void z_token_add_child(struct z_token_t *parent, struct z_token_t *child) {
  parent->children_count++;
  parent->children = realloc(
    parent->children, sizeof (struct z_token_t *) * parent->children_count);
  parent->children[parent->children_count - 1] = child;
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

void z_parse_root(
    struct z_token_t *token, size_t *codepos, struct z_label_t **labels) {
  if (!token) return;

  z_expr_cvt(token);

  if (z_typecmp(token, Z_TOKTYPE_INSTRUCTION)) {
    struct z_opcode_t *opcode = z_opcode_match(token);
    token->opcode = opcode;
    (*codepos) += opcode->size;

  } else if (z_typecmp(token, Z_TOKTYPE_LABEL)) {
    struct z_label_t *label = z_label_new(token->value, *codepos);
    z_label_add(labels, label);

  } else if (z_typecmp(token, Z_TOKTYPE_DIRECTIVE)) {
    if (z_streq(token->value, "db")) {
      for (int i = 0; i < token->children_count; i++) {
        struct z_token_t *op = token->children[i];

        if (z_typecmp(op, Z_TOKTYPE_NUMERIC)) {
          (*codepos)++;

        } else if (z_typecmp(op, Z_TOKTYPE_STRING)) {
          (*codepos) += strlen(op->value);

        } else {
          z_fail(op, "Wrong operand type in the 'db' directive: %s.\n", z_toktype_str(op->type));
          exit(1);
        }
      }

    } else if (z_streq(token->value, "dw")) {
      for (int i = 0; i < token->children_count; i++) {
        struct z_token_t *op = token->children[i];

        if (z_typecmp(op, Z_TOKTYPE_NUMERIC)) {
          (*codepos) += 2;

        } else if (z_typecmp(op, Z_TOKTYPE_STRING)) {
          (*codepos) += 2 * strlen(op->value);

        } else {
          z_fail(op, "Wrong operand type in the 'db' directive: %s.\n", z_toktype_str(op->type));
          exit(1);
        }
      }

    } else if (z_streq(token->value, "ds")) {
      if (token->children_count < 1 || token->children_count > 2) {
        z_fail(
          token,
          "'ds' directive requires 1-2 operand(s) but %d were given.\n",
          token->children_count);
        exit(1);
      }

      struct z_token_t *sizetok = z_get_child(token, 0);

      if (!z_typecmp(sizetok, Z_TOKTYPE_NUMERIC)) {
        z_fail(sizetok, "The first operand of the 'ds' directive has to be a numeric value.\n");
        exit(1);
      }

      (*codepos) += sizetok->numval;
    }

  } else if (z_streq(token->value, "def")) {

  }
}

struct z_token_t *z_get_child(struct z_token_t *token, int child_index) {
  if (child_index >= token->children_count) {
    z_fail(token, "Couldn't retrieve child #%d of the '%s' token.\n", child_index, token->value);
    exit(1);
  }

  return token->children[child_index];
}

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
