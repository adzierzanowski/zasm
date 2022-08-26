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
  char value[BUFSZ];            // Raw string value of the token
  struct z_token_t **children;  // Child tokens array
  struct z_token_t *numop;      // Opcode to be used as a source when filling in the value
  struct z_opcode_t *opcode;    // Used in instruction tokens to specify emitted values
  const char *fname;            // Source filename
  size_t children_count;        // Number of children
  enum z_toktype_t type;        // Type of the token
  int numval;                   // Used in numerical tokens to specify numerical value
  int line;                     // Source code line
  int col;                      // Source code column
  int label_offset;             // Used in emitter to fill in the numerical value in the opcode
  int precedence;               // Used in operator tokens
  int left_associative;         // Used in operator tokens
  uint16_t codepos;             // Position in the bytecode
  bool memref;                  // Was it in memory reference brackets? ("[", "]")
};

struct z_label_t {
  char key[BUFSZ];
  struct z_label_t *next;
  uint16_t value;
  bool imported;
};

struct z_def_t {
  char key[BUFSZ];
  struct z_token_t *value;
  struct z_def_t *next;
  struct z_token_t *definition;
};

struct z_opcode_t {
  size_t size;
  uint8_t bytes[BUFSZ];
};

#endif
