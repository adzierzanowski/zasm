#ifndef OPCODES_H
#define OPCODES_H

#include "util.h"
#include "structs.h"
#include "tokenizer.h"


struct z_opcode_t *z_opcode_match(struct z_token_t *token);

#endif
