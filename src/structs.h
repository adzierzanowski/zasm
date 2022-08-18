#ifndef STRUCTS_H
#define STRUCTS_H

#define BUFSZ 0x1000

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum z_toktype_t {
  Z_TOKTYPE_NONE = 0,
  Z_TOKTYPE_INSTRUCTION = 1,
  Z_TOKTYPE_STRING = 2,
  Z_TOKTYPE_NUMBER = 4,
  Z_TOKTYPE_DIRECTIVE = 8,
  Z_TOKTYPE_REGISTER_16 = 16,
  Z_TOKTYPE_IDENTIFIER = 32,
  Z_TOKTYPE_LABEL = 64,
  Z_TOKTYPE_REGISTER_8 = 128,
  Z_TOKTYPE_CONDITION = 256,
  Z_TOKTYPE_CHAR = 512,
  Z_TOKTYPE_OPERATOR = 1024,
  Z_TOKTYPE_EXPRESSION = 2048,
  Z_TOKTYPE_ANY = 0x7fffffff
};

#define Z_TOKTYPE_NUMERIC (Z_TOKTYPE_EXPRESSION | Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR)

struct z_token_t {
  char value[BUFSZ];
  const char *fname;
  size_t children_count;
  struct z_token_t **children;
  struct z_token_t *parent;
  struct z_opcode_t *opcode;
  int numval;
  int line;
  enum z_toktype_t type;
  bool memref;
  int col;
  int label_offset;
  struct z_token_t *numop;
};

struct z_label_t {
  char key[BUFSZ];
  uint16_t value;
  struct z_label_t *next;
};

struct z_opcode_t {
  size_t size;
  uint8_t bytes[4];
};

#endif
