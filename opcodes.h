#ifndef OPCODES_H
#define OPCODES_H

#include "util.h"
#include "structs.h"

struct z_opcode_t {
  size_t size;
  uint8_t bytes[4];
};


struct z_opcode_t *z_opcode_match(struct z_token_t *token);

#endif
