#ifndef OPCODES_H
#define OPCODES_H

#include "util.h"
#include "structs.h"
#include "tokenizer.h"


void z_opcode_set(struct z_opcode_t *opcode, size_t size, ...);
struct z_opcode_t *z_opcode_match(struct z_token_t *token, struct z_def_t *defs);

#endif
