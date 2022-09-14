#include "tokenizer.h"


struct z_token_t **z_tokenize(
    const char *fname,
    size_t *tokcnt,
    struct z_label_t **labels,
    struct z_def_t **defs,
    size_t *bytepos) {

  FILE *f = fopen(fname, "r");

  if (f == NULL) {
    z_fail(NULL, "Couldn't open file '%s'.\n", fname);
    exit(1);
  }

  struct z_token_t **tokens = NULL;

  char tokbuf[TOKBUFSZ] = {0};
  int tokbufptr = 0;
  int line = 0;
  int col = 0;

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

    if (c != -1 && c != 0 && c != 10 && !isprint(c)) {
      z_fail(NULL, "Invalid character encountered: %d.\n", c);
      exit(1);
    }

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
      // PASS

    } else if (c == '"') {
      in_string = true;

    } else if (c == '\'') {
      if (z_streq(tokbuf, "af")) {
        // PASS

      } else {
        in_char = true;
      }

    } else if (c == '[') {
      in_memref = true;

    } else if (isalnum(c) || c == '_') {
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

      if (z_typecmp(token,
          Z_TOKTYPE_DIRECTIVE | Z_TOKTYPE_INSTRUCTION | Z_TOKTYPE_LABEL)) {
        z_token_add(&tokens, tokcnt, token);

        z_parse_root(&tokens, root, bytepos, labels, defs, tokcnt);

        root = token;

      } else if (opsep || (root && !root->children_count)) {
        z_token_add_child(root, token);

        if (z_streq(token->value, "c") &&
            z_typecmp(token, Z_TOKTYPE_REGISTER_8) &&
            z_strmatch(root->value, "call", "ret", "jp", "jr", NULL)) {
          token->type = Z_TOKTYPE_CONDITION;
        }

        operand = token;
        opsep = false;

      } else {
        if (operand) {
          z_token_add_child(operand, token);
        } else {
          z_fail(token, "No parent to attach the token to.\n");
          exit(1);
        }
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
  }

  z_parse_root(&tokens, root, bytepos, labels, defs, tokcnt);

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
  sprintf(token->fname, "%s", fname);
  token->line = line;
  token->col = col - strlen(value) - 1;
  token->left_associative = true;

  if (token->type == Z_TOKTYPE_NONE) {
    if (z_strmatch_i(value, "bc", "de", "hl", "sp", "ix", "iy", "af", NULL)) {
      token->type = Z_TOKTYPE_REGISTER_16;
      z_strlower(token->value);

    } else if (
        z_strmatch_i(value, "a", "b", "c", "d", "e", "h", "l", "i", "r", NULL)) {
      token->type = Z_TOKTYPE_REGISTER_8;
      z_strlower(token->value);

    } else if (z_strmatch_i(value, "z", "nz", "c", "nc", "po", "pe", "p", "m", NULL)) {
      token->type = Z_TOKTYPE_CONDITION;
      z_strlower(token->value);

    } else if (z_strmatch_i(value,
        "ld", "push", "pop", "ex", "exx", "ldi", "ldir", "ldd", "lddr", "cpi",
        "cpir", "cpd", "cpdr", "add", "adc", "sub", "sbc", "and", "or", "xor",
        "cp", "inc", "dec", "daa", "cpl", "neg", "ccf", "scf", "nop", "halt",
        "di", "ei", "im", "rlca", "rla", "rrca", "rra", "rlc", "rl", "rrc",
        "rr", "sla", "sra", "srl", "rld", "rrd", "bit", "set", "res", "jp",
        "djnz", "call", "ret", "reti", "retn", "rst", "in", "ini", "inir",
        "ind", "indr", "out", "outi", "otir", "outd", "otdr", "jr", NULL)) {
      token->type = Z_TOKTYPE_INSTRUCTION;
      z_strlower(token->value);

    } else if (
        z_strmatch(value, "ds", "dw", "db", "def", "incbin", "include", "org", NULL)) {
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
  label->imported = false;
  return label;
}

// Adds a label to the label list.
// If a label with duplicate key already exists in the list, it is returned.
struct z_label_t *z_label_add(struct z_label_t **labels, struct z_label_t *label) {
  struct z_label_t *ptr = *labels;
  if (!ptr) {
    *labels = label;
    return NULL;
  }

  while (ptr->next != NULL) {
    if (z_streq(label->key, ptr->key)) {
      return ptr;
    }
    ptr = ptr->next;
  }
  ptr->next = label;
  return NULL;
}

bool z_typecmp(struct z_token_t *token, int types) {
  return (token->type & types);
}

void z_parse_root(
    struct z_token_t ***tokens,
    struct z_token_t *token,
    size_t *codepos,
    struct z_label_t **labels,
    struct z_def_t **defs,
    size_t *tokcnt) {
  if (!token) return;

  z_expr_cvt(token);
  token->codepos = *codepos;

  if (z_typecmp(token, Z_TOKTYPE_INSTRUCTION)) {
    struct z_opcode_t *opcode = z_opcode_match(token, *defs);
    token->opcode = opcode;
    (*codepos) += opcode->size;

  } else if (z_typecmp(token, Z_TOKTYPE_LABEL)) {
    struct z_label_t *label = z_label_new(token->value, *codepos);
    struct z_label_t *duplicate = z_label_add(labels, label);
    if (duplicate) {
      z_fail(
        token,
        "Duplicate label definition. Previously defined at address 0x%04hx%s.\n",
        duplicate->value,
        duplicate->imported ? " (imported)" : "");
      exit(1);
    }

  } else if (z_typecmp(token, Z_TOKTYPE_DIRECTIVE)) {
    if (z_streq(token->value, "db")) {
      for (int i = 0; i < token->children_count; i++) {
        struct z_token_t *op = token->children[i];

        if (z_typecmp(op, Z_TOKTYPE_NUMERIC)) {
          (*codepos)++;

        } else if (z_typecmp(op, Z_TOKTYPE_STRING)) {
          (*codepos) += strlen(op->value);

        } else {
          z_fail(
            op,
            "Wrong operand type in the 'db' directive: %s.\n",
            z_toktype_str(op->type));
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
          z_fail(
            op,
            "Wrong operand type in the 'db' directive: %s.\n",
            z_toktype_str(op->type));
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
        z_fail(
          sizetok,
          "The first operand of the 'ds' directive has to be a numeric value.\n");
        exit(1);
      }

      if (z_typecmp(sizetok, Z_TOKTYPE_EXPRESSION)) {
        z_expr_eval(sizetok, *labels, *defs, 0);
      }

      (*codepos) += sizetok->numval;

    } else if (z_streq(token->value, "def")) {
      if (token->children_count != 2) {
        z_fail(token, "'def' directive requires exactly two operands.\n");
        exit(1);
      }

      struct z_token_t *keytok = z_get_child(token, 0);
      struct z_token_t *valtok = z_get_child(token, 1);

      if (!z_typecmp(keytok, Z_TOKTYPE_IDENTIFIER)) {
        z_fail(
          keytok,
          "The first operand of the 'def' diretive must be an identifier. "
          "Got %s istead.\n",
          z_toktype_str(keytok->type));
        exit(1);
      }

      struct z_def_t *existing = z_def_get(*defs, keytok->value);
      if (existing) {
        z_fail(
          keytok,
          "Redefinition of '%s'. Previously defined here: %s:%d:%d\n",
          keytok->value,
          existing->definition->fname,
          existing->definition->line+1,
          existing->definition->col+1);
        exit(1);
      }
      struct z_label_t *existing_lbl = z_label_get(*labels, keytok->value);
      if (existing_lbl) {
        z_fail(
          keytok,
          "Redefinition of '%s'.\n",
          keytok->value);
        exit(1);
      }

      struct z_def_t *def = z_def_new(keytok->value, valtok, token);
      z_def_add(defs, def);

    } else if (z_streq(token->value, "include")) {
      if (token->children_count != 1) {
        z_fail(token, "'include' directive requires exactly one operand.\n");
        exit(1);
      }

      struct z_token_t *fname_token = z_get_child(token, 0);
      char fpath[Z_BUFSZ] = {0};
      char *dname = z_dirname(token->fname);
      if (dname) {
        sprintf(fpath, "%s/%s", dname, fname_token->value);
        free(dname);
      } else {
        sprintf(fpath, "%s", fname_token->value);
      }

      size_t new_tokcnt = 0;
      size_t final_tokcnt = 0;
      struct z_token_t **new_tokens = z_tokenize(
        fpath, &new_tokcnt, labels, defs, codepos);

      *tokens = z_tokens_merge(
        *tokens, new_tokens, *tokcnt, new_tokcnt, &final_tokcnt);
      *tokcnt = final_tokcnt;

    } else if (z_streq(token->value, "incbin")) {
      struct z_token_t *fname_token = z_get_child(token, 0);
      char fpath[Z_BUFSZ] = {0};
      char *dname = z_dirname(token->fname);

      if (dname) {
        sprintf(fpath, "%s/%s", dname, fname_token->value);
        free(dname);
      } else {
        sprintf(fpath, "%s", fname_token->value);
      }

      struct stat bin_stat;
      int stat_res = stat(fpath, &bin_stat);
      if (stat_res != 0) {
        z_fail(token, "Error while testing file '%s': %s\n", fpath, strerror(errno));
        exit(1);
      }

      size_t bin_size = bin_stat.st_size;
      (*codepos) += bin_size;

      #ifdef DEBUG
      printf("incbin: %s: %zu bytes\n", fpath, bin_size);
      #endif

      sprintf(token->fname, "%s", fpath);
    }
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

struct z_def_t *z_def_new(
    char *key, struct z_token_t *value, struct z_token_t *deftok) {
  struct z_def_t *def = malloc(sizeof (struct z_def_t));
  strcpy(def->key, key);
  def->value = value;
  def->definition = deftok;
  def->next = NULL;
  return def;
}

void z_def_add(struct z_def_t **defs, struct z_def_t *def) {
  struct z_def_t *ptr = *defs;

  if (!ptr) {
    *defs = def;
    return;
  }

  while (ptr->next != NULL) {
    ptr = ptr->next;
  }

  ptr->next = def;
}

struct z_def_t *z_def_get(struct z_def_t *defs, char *key) {
  struct z_def_t *ptr = defs;

  while (ptr != NULL){
    if (z_streq(ptr->key, key)) {
      return ptr;
    }

    ptr = ptr->next;
  }

  return NULL;
}

struct z_token_t **z_tokens_merge(
    struct z_token_t **tokens1,
    struct z_token_t **tokens2,
    size_t tokcnt1,
    size_t tokcnt2,
    size_t *tokcnt_out) {
  *tokcnt_out = tokcnt1 + tokcnt2;
  struct z_token_t **out = realloc(tokens1, *tokcnt_out * sizeof (struct z_token_t *));

  for (int i = 0; i < tokcnt2; i++) {
    out[i+tokcnt1] = tokens2[i];
  }

  free(tokens2);

  return out;
}

void z_labels_free(struct z_label_t *labels) {
  struct z_label_t *ptr = labels;

  while (ptr != NULL) {
    free(ptr);
    ptr = ptr->next;
  }
}

void z_defs_free(struct z_def_t *defs) {
  if (!defs) return;

  struct z_def_t *ptr = defs;

  while (ptr != NULL) {
    free(ptr);
    ptr = ptr->next;
  }
}

void z_tokens_free(struct z_token_t **tokens, size_t tokcnt) {
  for (int i = 0; i < tokcnt; i++) {
    struct z_token_t *tok = tokens[i];

    for (int j = 0; j < tok->children_count; j++) {
      struct z_token_t *child = tok->children[j];

      for (int k = 0; k < child->children_count; k++) {
        struct z_token_t *grandchild = child->children[k];
        free(grandchild);
      }

      if (child->children) {
        free(child->children);
        child->children = NULL;
      }

      free(child);
    }

    if (tok->children) {
      free(tok->children);
      tok->children = NULL;
    }

    if (tok->opcode) {
      free(tok->opcode);
    }

    free(tok);
  }

  free(tokens);
}

void z_labels_export(FILE *f, struct z_label_t *labels, struct z_def_t *defs) {
  struct z_label_t *ptr = labels;

  if (labels == NULL) {
    return;
  }

  char buf[Z_BUFSZ] = {0};

  while (ptr != NULL) {
    if (ptr->key[0] != '_' && !ptr->imported) {
      sprintf(buf, "%s %d\n", ptr->key, ptr->value);
      fwrite(buf, sizeof (char), strlen(buf), f);
    }
    ptr = ptr->next;
  }

  if (defs != NULL) {
    struct z_def_t *dptr = defs;
    while (dptr != NULL) {
      if (z_typecmp(dptr->value, Z_TOKTYPE_NUMERIC) && dptr->key[0] != '_') {
        sprintf(buf, "%s %d\n", dptr->key, dptr->value->numval);
        fwrite(buf, sizeof (char), strlen(buf), f);
      }

      dptr = dptr->next;
    }
  }
}

struct z_label_t *z_labels_import(FILE *f) {
  char *buf = NULL;
  size_t bufsz = 0;

  struct z_label_t *labels = NULL;

  int res = 0;

  while (res != -1) {
    res = getline(&buf, &bufsz, f);
    char *key = strtok(buf, " ");
    char *val = strtok(NULL, "\n");
    if (key != NULL && val != NULL) {
      struct z_label_t *label = z_label_new(key, atoi(val));
      label->imported = true;
      struct z_label_t *duplicate = z_label_add(&labels, label);
      if (duplicate) {
        z_fail(
          NULL,
          "Label currently being imported has already been defined at address 0x%04hx.\n",
          duplicate->value);
        exit(1);
      }
    }
  }

  if (buf) {
    free(buf);
    buf = NULL;
  }

  return labels;
}

int *z_lbldef_resolve(
    struct z_label_t *labels,
    struct z_def_t *defs,
    uint16_t origin,
    char *key) {
  struct z_label_t *label = z_label_get(labels, key);

  if (label) {
    int *out = malloc(sizeof (int));
    *out = label->value + origin;
    return out;
  }

  struct z_def_t *def = z_def_get(defs, key);

  if (def)  {
    int *out = malloc(sizeof (int));
    *out = def->value->numval;
    return out;
  }

  return NULL;
}
