#include "opcodes.h"


#ifdef DEBUG
#define fail(...) do {\
  z_fail(token, __VA_ARGS__); \
} while (0);
#define match_fail(x) do {\
  z_fail(token, __FILE__ ":%d: No match for the '" x "' instruction.\n", __LINE__);\
} while (0);
#define require_operand(op, ins) do {\
  if (!op) {\
    fail("'" ins "' instruction requires operand(s).\n");\
  }\
} while (0);
#else
#define fail(...) do {\
  z_fail(token, __VA_ARGS__); \
  exit(1);\
} while (0);
#define match_fail(x) do {\
  z_fail(token, "No match for the '" x "' instruction.\n");\
  exit(1);\
} while (0);
#define require_operand(op, ins) do {\
  if (!op) {\
    fail("'" ins "' instruction requires operand(s).\n");\
    exit(1);\
  }\
} while (0);
#endif

#define TOKVAL(token, val) z_streq(token->value, (char *) val)

void z_opcode_set(struct z_opcode_t *opcode, size_t size, ...) {
  opcode->size = size;

  va_list args;
  va_start(args, size);

  for (int i = 0; i < size; i++) {
    uint8_t arg = va_arg(args, int);
    opcode->bytes[i] = arg;
  }

  va_end(args);
}

static uint8_t z_reg8_bits(struct z_token_t *token) {
  switch (token->value[0]) {
    case 'a': return 0x07;
    case 'b': return 0x00;
    case 'c': return 0x01;
    case 'd': return 0x02;
    case 'e': return 0x03;
    case 'h': return 0x04;
    case 'l': return 0x05;
    default:
      fail("Unknown 8 bit register.\n");
      return 0;
  }
}

static uint8_t z_reg16_bits(struct z_token_t *token) {
  if (TOKVAL(token, "bc")) {
    return 0;
  } else if (TOKVAL(token, "de")) {
    return 1;
  } else if (z_strmatch(token->value, "hl", "ix", "iy", NULL)) {
    return 2;
  } else if (TOKVAL(token, "sp") || TOKVAL(token, "af")) {
    return 3;
  } else {
    fail("Unknown 16-bit pair.\n");
    return 0;
  }
}

static uint8_t cond_bits(struct z_token_t *token) {
  if (z_streq(token->value, "nz")) {
    return 0x00;
  } else if (z_streq(token->value, "z")) {
    return 0x01;
  } else if (z_streq(token->value, "nc")) {
    return 0x02;
  } else if (z_streq(token->value, "c")) {
    return 0x03;
  } else if (z_streq(token->value, "po")) {
    return 0x04;
  } else if (z_streq(token->value, "pe")) {
    return 0x05;
  } else if (z_streq(token->value, "p")) {
    return 0x06;
  } else if (z_streq(token->value, "m")) {
    return 0x07;
  } else {
    fail("Unknown condition.\n");
    return 0;
  }
}

static uint8_t z_bit_bits(struct z_token_t *token) {
  if (token->numval >= 0 && token->numval < 8) {
    return token->numval;

  } else {
    fail("Bad bit number: %d.\n", token->numval);
    return 0;
  }
}

static bool z_is_abcdehl(struct z_token_t *token, bool memref) {
  return (
    z_typecmp(token, Z_TOKTYPE_REGISTER_8) &&
    z_strmatch(token->value, "a", "b", "c", "d", "e", "h", "l", NULL) &&
    token->memref == memref
  );
}

static bool z_is_hl(struct z_token_t *token) {
  return (z_typecmp(token, Z_TOKTYPE_REGISTER_16) && TOKVAL(token, "hl"));
}

static bool z_is_reg8(struct z_token_t *token, const char *value, bool memref) {
  return z_typecmp(token, Z_TOKTYPE_REGISTER_8) &&
    TOKVAL(token, value) &&
    token->memref == memref;
}

static bool z_is_reg16(struct z_token_t *token, const char *value, bool memref) {
  bool valcondition = true;

  if (value != NULL) {
    valcondition = (TOKVAL(token, value));
  }

  return z_typecmp(token, Z_TOKTYPE_REGISTER_16) && valcondition && token->memref == memref;
}

static bool z_is_num(struct z_token_t *token, bool memref) {
  return z_typecmp(token, Z_TOKTYPE_NUMERIC) && token->memref == memref;
}

void z_validate_operands(struct z_token_t *token, int min_opcnt, int max_opcnt, ...) {
  if (token->children_count < min_opcnt || token->children_count > max_opcnt) {
    z_fail(
      token,
      "%s instruction takes from %d to %d operands but encountered %d.\n",
      token->value,
      min_opcnt,
      max_opcnt,
      token->children_count);
    #ifndef DEBUG
    exit(1);
    #endif
  }
}


void z_set_offsets(
    struct z_token_t *token, int label_offset,struct z_token_t *numop) {
  token->label_offset = label_offset;
  token->numop = numop;
}

struct z_opcode_t *z_opcode_match(struct z_token_t *token) {
  struct z_opcode_t *opcode = malloc(sizeof (struct z_opcode_t));

  opcode->size = 0;

  if (TOKVAL(token, "ld")) {
    z_validate_operands(token, 2, 2);

    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    // GROUP: 8-bit load group
    // LD r, r'
    if (z_is_abcdehl(op1, false) && z_is_abcdehl(op2, false)) {
      z_opcode_set(opcode, 1, 0x40 | (z_reg8_bits(op1) << 3) | z_reg8_bits(op2));

    // LD r, n
    } else if (z_is_abcdehl(op1, false) && !op1->memref && z_is_num(op2, false)) {
      z_opcode_set(opcode, 2, 0x06 | (z_reg8_bits(op1) << 3), 0);
      z_set_offsets(token, 1, op2);

    // LD r, [HL]
    } else if (z_is_abcdehl(op1, false) && z_is_hl(op2) && op2->memref) {
      z_opcode_set(opcode, 1, 0x46 | (z_reg8_bits(op1) << 3));

    // LD r, [IX + d]
    } else if (z_is_abcdehl(op1, false) && z_is_reg16(op2, "ix", true) && !op1->memref) {
      z_opcode_set(opcode, 3, 0xdd, 0x46 | (z_reg8_bits(op1) << 3), 0);
      z_set_offsets(token, 2, op2->children[1]);

    // LD r, [IY + d]
    } else if (z_is_abcdehl(op1, false) && z_is_reg16(op2, "iy", true) && !op1->memref) {
      z_opcode_set(opcode, 3, 0xfd, 0x46 | (z_reg8_bits(op1) << 3), 0);
      z_set_offsets(token, 2, op2->children[1]);

    // LD [HL], r
    } else if (z_is_hl(op1) && z_is_abcdehl(op2, false) && op1->memref && !op2->memref) {
      z_opcode_set(opcode, 1, 0x70 | (z_reg8_bits(op2)));

    // LD [IX + d], r
    } else if (z_is_reg16(op1, "ix", true) && z_is_abcdehl(op2, false)) {
      z_opcode_set(opcode, 3, 0xdd, 0x70 | z_reg8_bits(op2), 0);
      z_set_offsets(token, 2, op1->children[1]);

    // LD [IY + d], r
    } else if (z_is_reg16(op1, "iy", true) && z_is_abcdehl(op2, false)) {
      z_opcode_set(opcode, 3, 0xfd, 0x70 | z_reg8_bits(op2), 0);
      z_set_offsets(token, 2, op1->children[1]);

    // LD [HL], n
    } else if (z_is_reg16(op1, "hl", true) && z_typecmp(op2, Z_TOKTYPE_NUMERIC)) {
      z_opcode_set(opcode, 2, 0x36, 0);
      z_set_offsets(token, 1, op2);

    // LD [IX + d], n
    } else if (z_is_reg16(op1, "ix", true) && z_is_num(op2, false)) {
      z_opcode_set(opcode, 4, 0xdd, 0x36, op1->children[1]->numval);
      z_set_offsets(token, 3, op2);

    // LD [IY + d], n
    } else if (z_is_reg16(op1, "iy", true) && z_is_num(op2, false)) {
      z_opcode_set(opcode, 4, 0xfd, 0x36, op1->children[1]->numval, 0);
      z_set_offsets(token, 3, op2);

    // LD A, [BC]
    } else if (z_is_reg8(op1, "a", false) && z_is_reg16(op2, "bc", true)) {
      z_opcode_set(opcode, 1, 0x0a);

    // LD A, [DE]
    } else if (z_is_reg8(op1, "a", false) && z_is_reg16(op2, "de", true)) {
      z_opcode_set(opcode, 1, 0x1a);

    // LD A, [nn]
    } else if (z_is_reg8(op1, "a", false) && z_is_num(op2, true)) {
      z_opcode_set(opcode, 3, 0x3a, 0, 0);
      z_set_offsets(token, 1, op2);

    // LD [BC], A
    } else if (z_is_reg16(op1, "bc", true) && z_is_reg8(op2, "a", false)) {
      z_opcode_set(opcode, 1, 0x02);

    // LD [DE], A
    } else if (z_is_reg16(op1, "de", true) && z_is_reg8(op2, "a", false)) {
      z_opcode_set(opcode, 1, 0x12);

    // LD [nn], A
    } else if (z_is_num(op1, true) && z_is_reg8(op2, "a", false)) {
      z_opcode_set(opcode, 3, 0x32, 0, 0);
      z_set_offsets(token, 1, op1);

    // LD A, I
    } else if (z_is_reg8(op1, "a", false) && z_is_reg8(op2, "i", false)) {
      z_opcode_set(opcode, 2, 0xed, 0x57);

    // LD A, R
    } else if (z_is_reg8(op1, "a", false) && z_is_reg8(op2, "r", false)) {
      z_opcode_set(opcode, 2, 0xed, 0x5f);

    // LD I, A
    } else if (z_is_reg8(op1, "i", false) && z_is_reg8(op2, "a", false)) {
      z_opcode_set(opcode, 2, 0xed, 0x47);

    // LD R, A
    } else if (z_is_reg8(op1, "r", false) && z_is_reg8(op2, "a", false)) {
      z_opcode_set(opcode, 2, 0xed, 0x4f);

    // GROUP: 16-bit load group
    // NOTICE: the order of the conditions is not exactly one-to-one with the
    // Z80 manual because z_is_reg16(..., NULL, ...) catches ALL the 16-bit
    // registers so IX must be caught first!

    // LD IX, nn
    } else if (z_is_reg16(op1, "ix", false) && z_is_num(op2, false)) {
      z_opcode_set(opcode, 4, 0xdd, 0x21, 0, 0);
      z_set_offsets(token, 2, op2);

    // LD IY, nn
    } else if (z_is_reg16(op1, "iy", false) && z_is_num(op2, false)) {
      z_opcode_set(opcode, 4, 0xfd, 0x21, 0, 0);
      z_set_offsets(token, 2, op2);

    // LD dd, nn
    } else if (z_is_reg16(op1, NULL, false) && z_is_num(op2, false)) {
      z_opcode_set(opcode, 3, (0x01 | z_reg16_bits(op1) << 4), 0, 0);
      z_set_offsets(token, 1, op2);

    // LD HL, [nn]
    } else if (z_is_reg16(op1, "hl", false) && z_is_num(op2, true)) {
      z_opcode_set(opcode, 3, 0x2a, 0, 0);
      z_set_offsets(token, 1, op2);

    // LD IX, [nn]
    } else if (z_is_reg16(op1, "ix", false) && z_is_num(op2, true)) {
      z_opcode_set(opcode, 4, 0xdd, 0x2a, 0, 0);
      z_set_offsets(token, 1, op2);

    // LD IY, [nn]
    } else if (z_is_reg16(op1, "iy", false) && z_is_num(op2, true)) {
      z_opcode_set(opcode, 4, 0xfd, 0x2a, 0, 0);
      z_set_offsets(token, 2, op2);

    // LD dd, [nn]
    } else if (z_is_reg16(op1, NULL, false) && z_is_num(op2, true)) {
      z_opcode_set(opcode, 4, 0xed, (0x4b | z_reg16_bits(op1) << 4), 0, 0);
      z_set_offsets(token, 2, op2);

    // LD [nn], HL
    } else if (z_is_num(op1, true) && z_is_reg16(op2, "hl", false)) {
      z_opcode_set(opcode, 3, 0x22, 0, 0);
      z_set_offsets(token, 1, op1);

    // LD [nn], IX
    } else if (z_is_num(op1, true) && z_is_reg16(op2, "ix", false)) {
      z_opcode_set(opcode, 4, 0xdd, 0x22, 0, 0);
      z_set_offsets(token, 2, op1);

    // LD [nn], IY
    } else if (z_is_num(op1, true) && z_is_reg16(op2, "iy", false)) {
      z_opcode_set(opcode, 4, 0xfd, 0x22, 0, 0);
      z_set_offsets(token, 2, op1);

    // LD [nn], dd
    } else if (z_is_num(op1, true) && z_is_reg16(op2, NULL, false)) {
      z_opcode_set(opcode, 4, 0xed, 0x43 | (z_reg16_bits(op2) << 4), 0, 0);
      z_set_offsets(token, 2, op1);

    // LD SP, HL
    } else if (z_is_reg16(op1, "sp", false) && z_is_reg16(op2, "hl", false)) {
      z_opcode_set(opcode, 1, 0xf9);

    // LD SP, IX
    } else if (z_is_reg16(op1, "sp", false) && z_is_reg16(op2, "ix", false)) {
      z_opcode_set(opcode, 2, 0xf9);

    // LD SP, IY
    } else if (z_is_reg16(op1, "sp", false) && z_is_reg16(op2, "iy", false)) {
      z_opcode_set(opcode, 2, 0xfd, 0xf9);

    } else {
      match_fail("ld");
    }

  } else if (TOKVAL(token, "push")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // PUSH IX
    if (z_is_reg16(op1, "ix", false)) {
      z_opcode_set(opcode, 2, 0xdd, 0xe5);

    // PUSH IY
    } else if (z_is_reg16(op1, "iy", false)) {
      z_opcode_set(opcode, 2, 0xfd, 0xe5);

    // PUSH qq
    } else if (z_is_reg16(op1, NULL, false)) {
      z_opcode_set(opcode, 1, 0xc5 | (z_reg16_bits(op1) << 4));
    } else {
      match_fail("push");
    }

  } else if (TOKVAL(token, "pop")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // POP IX
    if (z_is_reg16(op1, "ix", false)) {
      z_opcode_set(opcode, 2, 0xdd, 0xe1);

    // POP IY
    } else if (z_is_reg16(op1, "iy", false)) {
      z_opcode_set(opcode, 2, 0xfd, 0xe1);

    // POP qq
    } else if (z_is_reg16(op1, NULL, false)) {
      z_opcode_set(opcode, 1, 0xc1 | (z_reg16_bits(op1) << 4));

    } else {
      match_fail("pop");
    }

  // GROUP: Exchange, block transfer and search

  } else if (TOKVAL(token, "ex")) {
    z_validate_operands(token, 2, 2);

    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    // EX DE, HL
    if (z_is_reg16(op1, "de", false) && z_is_reg16(op2, "hl", false)) {
      z_opcode_set(opcode, 1, 0xeb);

    // EX AF, AF'
    } else if (z_is_reg16(op1, "af", false) && z_is_reg16(op2, "af", false)) {
      z_opcode_set(opcode, 1, 0x08);

    // EX [SP], HL
    } else if (z_is_reg16(op1, "sp", true) && z_is_reg16(op2, "hl", false)) {
      z_opcode_set(opcode, 1, 0xe3);

    // EX [SP], IX
    } else if (z_is_reg16(op1, "sp", true) && z_is_reg16(op2, "ix", false)) {
      z_opcode_set(opcode, 2, 0xdd, 0xe3);

    // EX [SP], IY
    } else if (z_is_reg16(op1, "sp", true) && z_is_reg16(op2, "iy", false)) {
      z_opcode_set(opcode, 2, 0xfd, 0xe3);

    } else {
      match_fail("ex");
    }

  // EXX
  } else if (TOKVAL(token, "exx")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0xd9);

  // LDI
  } else if (TOKVAL(token, "ldi")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xa0);

  // LDIR
  } else if (TOKVAL(token, "ldir")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xb0);

  // LDD
  } else if (TOKVAL(token, "ldd")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xa8);

  // LDDR
  } else if (TOKVAL(token, "lddr")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xb8);

  // CPI
  } else if (TOKVAL(token, "cpi")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xa1);

  // CPIR
  } else if (TOKVAL(token, "cpir")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xb1);

  // CPD
  } else if (TOKVAL(token, "cpd")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xa9);

  // CPDR
  } else if (TOKVAL(token, "cpdr")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xb9);

  // GROUP: 8-bit arithmetic
  // GROUP: 16-bit arithmetic

  } else if (TOKVAL(token, "add")) {
    z_validate_operands(token, 2, 2);

    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    if (z_is_reg8(op1, "a", false)) {
      // ADD A, r
      if (z_is_abcdehl(op2, false)) {
        z_opcode_set(opcode, 1, 0x80 | z_reg8_bits(op2));

      // ADD A, n
      } else if (z_is_num(op2, false)) {
        z_opcode_set(opcode, 2, 0xc6, 0);
        z_set_offsets(token, 1, op2);

      // ADD A, [HL]
      } else if (z_is_reg16(op2, "hl", true)) {
        z_opcode_set(opcode, 1, 0x86);

      // ADD A, [IX + d]
      } else if (z_is_reg16(op2, "ix", true)) {
        z_opcode_set(opcode, 3, 0xdd, 0x86, 0);
        z_set_offsets(token, 2, op2->children[1]);

      // ADD A, [IY + d]
      } else if (z_is_reg16(op2, "iy", true)) {
        z_opcode_set(opcode, 3, 0xfd, 0x86, 0);
        z_set_offsets(token, 2, op2->children[1]);

      } else {
        match_fail("add");
      }

    // ADD HL, ss
    } else if (z_is_reg16(op1, "hl", false) && z_is_reg16(op2, NULL, false)) {
      z_opcode_set(opcode, 1, 0x09 | (z_reg16_bits(op2) << 4));

    // ADD IX, pp
    } else if (z_is_reg16(op1, "ix", false) && z_is_reg16(op2, NULL, false)) {
      z_opcode_set(opcode, 2, 0xdd, 0x09 | (z_reg16_bits(op2) << 4));

    // ADD IY, rr
    } else if (z_is_reg16(op1, "iy", false) && z_is_reg16(op2, NULL, false)) {
      z_opcode_set(opcode, 2, 0xfd, 0x09 | (z_reg16_bits(op2) << 4));

    } else {
      match_fail("add");
    }

  } else if (TOKVAL(token, "adc")) {
    z_validate_operands(token, 2, 2);

    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    // ADC A, r
    if (z_is_reg8(op1, "a", false)) {
      if (z_is_abcdehl(op2, false)) {
        z_opcode_set(opcode, 1, 0x88 | z_reg8_bits(op2));

      // ADC A, n
      } else if (z_is_num(op2, false)) {
        z_opcode_set(opcode, 2, 0xce, 0);
        z_set_offsets(token, 1, op2);

      // ADC A, [HL]
      } else if (z_is_reg16(op2, "hl", true)) {
        z_opcode_set(opcode, 1, 0x8e);

      // ADC A, [IX + d]
      } else if (z_is_reg16(op2, "ix", true)) {
        z_opcode_set(opcode, 3, 0xdd, 0x8e, 0);
        z_set_offsets(token, 2, op2->children[1]);

      // ADC A, [IY + d]
      } else if (z_is_reg16(op2, "iy", true)) {
        z_opcode_set(opcode, 3, 0xfd, 0x8e, 0);
        z_set_offsets(token, 2, op2->children[1]);

      } else {
        match_fail("adc");
      }

    // ADC HL, ss
    } else if (z_is_reg16(op1, "hl", false) && z_is_reg16(op2, NULL, false)) {
      z_opcode_set(opcode, 2, 0xed, 0x4a | z_reg16_bits(op2));

    } else {
      match_fail("adc");
    }

  } else if (TOKVAL(token, "sub")) {
    z_validate_operands(token, 1, 1);

    struct z_token_t *op1 = z_get_child(token, 0);

    // SUB r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 1, 0x90 | z_reg8_bits(op1));

    // SUB n
    } else if (z_is_num(op1, false)) {
      z_opcode_set(opcode, 2, 0xd6, 0);
      z_set_offsets(token, 1, op1);

    // SUB [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 1, 0x96);

    // SUB [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 3, 0xdd, 0x96, 0);
      z_set_offsets(token, 2, op1);

    // SUB [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 3, 0xfd, 0x96, 0);
      z_set_offsets(token, 2, op1);

    } else {
      match_fail("sub");
    }

  } else if (TOKVAL(token, "sbc")) {
    z_validate_operands(token, 2, 2);
    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    // SBC A, r
    if (z_is_reg8(op1, "a", false)) {
      if (z_is_abcdehl(op2, false)) {
        z_opcode_set(opcode, 1, 0x98 | z_reg8_bits(op2));

      // ADC A, n
      } else if (z_is_num(op2, false)) {
        z_opcode_set(opcode, 2, 0xde, 0);
        z_set_offsets(token, 1, op2);

      // ADC A, [HL]
      } else if (z_is_reg16(op2, "hl", true)) {
        z_opcode_set(opcode, 1, 0x9e);

      // ADC A, [IX + d]
      } else if (z_is_reg16(op2, "ix", true)) {
        z_opcode_set(opcode, 3, 0xdd, 0x9e, 0);
        z_set_offsets(token, 2, op2);

      // ADC A, [IY + d]
      } else if (z_is_reg16(op2, "iy", true)) {
        z_opcode_set(opcode, 3, 0xfd, 0x9e, 0);
        z_set_offsets(token, 2, op2);

      } else {
        match_fail("sbc");
      }

    // SBC HL, ss
    } else if (z_is_reg16(op1, "hl", false) && z_is_reg16(op2, NULL, false)) {
      z_opcode_set(opcode, 2, 0xed, 0x42 | (z_reg16_bits(op2) << 4));

    } else {
      match_fail("sbc");
    }

  } else if (TOKVAL(token, "and")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // AND r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 1, 0xa0 | z_reg8_bits(op1));

    // AND n
    } else if (z_is_num(op1, false)) {
      z_opcode_set(opcode, 2, 0xe6, 0);
      z_set_offsets(token, 1, op1);

    // AND [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 1, 0xa6);

    // AND [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 3, 0xdd, 0xa6, 0);
      z_set_offsets(token, 2, op1->children[1]);

    // AND [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 3, 0xfd, 0xa6, 0);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("and");
    }

  } else if (TOKVAL(token, "or")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // OR r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 1, 0xb0 | z_reg8_bits(op1));

    // OR n
    } else if (z_is_num(op1, false)) {
      z_opcode_set(opcode, 2, 0xf6, 0);
      z_set_offsets(token, 1, op1);

    // OR [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 1, 0xb6);

    // OR [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 3, 0xdd, 0xb6, 0);
      z_set_offsets(token, 2, op1->children[1]);

    // OR [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 3, 0xfd, 0xb6, 0);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("or");
    }

  } else if (TOKVAL(token, "xor")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // XOR r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 1, 0xa8 | z_reg8_bits(op1));

    // XOR n
    } else if (z_is_num(op1, false)) {
      z_opcode_set(opcode, 2, 0xee, 0);
      z_set_offsets(token, 1, op1);

    // XOR [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 1, 0xae);

    // XOR [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 3, 0xdd, 0xae, 0);
      z_set_offsets(token, 2, op1->children[1]);

    // XOR [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 3, 0xfd, 0xae, 0);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("xor");
    }

  } else if (TOKVAL(token, "cp")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // CP r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 1, 0xb8 | z_reg8_bits(op1));

    // CP n
    } else if (z_is_num(op1, false)) {
      z_opcode_set(opcode, 2, 0xfe, 0);
      z_set_offsets(token, 1, op1);

    // CP [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 1, 0xbe);

    // CP [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 3, 0xdd, 0xbe, 0);
      z_set_offsets(token, 2, op1->children[1]);

    // CP [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 3, 0xfd, 0xbe, 0);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("cp");
    }

  } else if (TOKVAL(token, "inc")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // INC r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 1, 0x04 | (z_reg8_bits(op1) << 3));

    // INC [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 1, 0x34);

    // INC [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 3, 0xdd, 0x34, 0);
      z_set_offsets(token, 2, op1->children[1]);

    // INC [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 3, 0xfd, 0x34, 0);
      z_set_offsets(token, 2, op1->children[1]);

    // INC IX
    } else if (z_is_reg16(op1, "ix", false)) {
      z_opcode_set(opcode, 2, 0xdd, 0x23);

    // INC IY
    } else if (z_is_reg16(op1, "iy", false)) {
      z_opcode_set(opcode, 2, 0xdd, 0x23);

    // INC ss
    } else if (z_is_reg16(op1, NULL, false)) {
      z_opcode_set(opcode, 1, 0x03 | (z_reg16_bits(op1) << 4));

    } else {
      match_fail("inc");
    }

  } else if (TOKVAL(token, "dec")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // DEC r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 1, 0x05 | (z_reg8_bits(op1) << 3));

    // DEC [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 1, 0x35);

    // DEC [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 3, 0xdd, 0x35, 0);
      z_set_offsets(token, 2, op1->children[1]);

    // DEC [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 3, 0xfd, 0x35, 0);
      z_set_offsets(token, 2, op1->children[1]);

    // DEC IX
    } else if (z_is_reg16(op1, "ix", false)) {
      z_opcode_set(opcode, 2, 0xdd, 0x2b);

    // DEC IY
    } else if (z_is_reg16(op1, "iy", false)) {
      z_opcode_set(opcode, 2, 0xfd, 0x2b);

    // DEC ss
    } else if (z_is_reg16(op1, NULL, false)) {
      z_opcode_set(opcode, 1, 0x0b | (z_reg16_bits(op1) << 4));

    } else {
      match_fail("dec");
    }

  // GROUP: general-purpose arithmetic and cpu control

  // DAA
  } else if (TOKVAL(token, "daa")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x27);

  // CPL
  } else if (TOKVAL(token, "cpl")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x2f);

  // NEG
  } else if (TOKVAL(token, "neg")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0x44);

  // CCF
  } else if (TOKVAL(token, "ccf")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x3f);

  // SCF
  } else if (TOKVAL(token, "scf")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x37);

  // NOP
  } else if (TOKVAL(token, "nop")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x00);

  // HALT
  } else if (TOKVAL(token, "halt")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x76);

  // DI
  } else if (TOKVAL(token, "di")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0xf3);

  // EI
  } else if (TOKVAL(token, "ei")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0xfb);

  } else if (TOKVAL(token, "im")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      if (op1->numval == 0) {
        z_opcode_set(opcode, 2, 0xed, 0x46);

      } else if (op1->numval == 1) {
        z_opcode_set(opcode, 2, 0xed, 0x56);

      } else if (op1->numval == 2) {
        z_opcode_set(opcode, 2, 0xed, 0x5e);

      } else {
        match_fail("im");
      }

    } else {
      match_fail("im");
    }


  // GROUP: rotate and shift

  // RLCA
  } else if (TOKVAL(token, "rlca")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x07);

  // RLA
  } else if (TOKVAL(token, "rla")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x17);

  // RRCA
  } else if (TOKVAL(token, "rrca")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x0f);

  // RRA
  } else if (TOKVAL(token, "rra")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 1, 0x1f);

  } else if (TOKVAL(token, "rlc")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // RLC r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 2, 0xcb, z_reg8_bits(op1));

    // RLC [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 2, 0xcb, 0x06);

    // RLC [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0x06);
      z_set_offsets(token, 2, op1->children[1]);

    // RLC [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0x06);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("rlc");
    }

  } else if (TOKVAL(token, "rl")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // RL r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 2, 0xcb, 0x10 | z_reg8_bits(op1));

    // RL [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 2, 0xcb, 0x16);

    // RL [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0x16);
      z_set_offsets(token, 2, op1->children[1]);

    // RL [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0x16);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("rl");
    }

  } else if (TOKVAL(token, "rrc")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // RRC r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 2, 0xcb, 0x08 | z_reg8_bits(op1));

    // RRC [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 2, 0xcb, 0x0e);

    // RRC [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0x0e);
      z_set_offsets(token, 2, op1->children[1]);

    // RRC [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0x0e);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("rrc");
    }

  } else if (TOKVAL(token, "rr")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // RR r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 2, 0xcb, 0x18 | z_reg8_bits(op1));

    // RR [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 2, 0xcb, 0x1e);

    // RR [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0x1e);
      z_set_offsets(token, 2, op1->children[1]);

    // RR [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0x1e);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("rr");
    }

  } else if (TOKVAL(token, "sla")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // SLA r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 2, 0xcb, 0x20 | z_reg8_bits(op1));

    // SLA [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 2, 0xcb, 0x16);

    // SLA [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0x26);
      z_set_offsets(token, 2, op1->children[1]);

    // SLA [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0x26);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("sla");
    }

  } else if (TOKVAL(token, "sra")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // SRA r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 2, 0xcb, 0x28 | z_reg8_bits(op1));

    // SRA [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 2, 0xcb, 0x2e);

    // SRA [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0x2e);
      z_set_offsets(token, 2, op1->children[1]);

    // SRA [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0x2e);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("sra");
    }

  } else if (TOKVAL(token, "srl")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    // SRL r
    if (z_is_abcdehl(op1, false)) {
      z_opcode_set(opcode, 2, 0xcb, 0x38 | z_reg8_bits(op1));

    // SRL [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 2, 0xcb, 0x3e);

    // SRL [IX + d]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0x3e);
      z_set_offsets(token, 2, op1->children[1]);

    // SRL [IY + d]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0x3e);
      z_set_offsets(token, 2, op1->children[1]);

    } else {
      match_fail("srl");
    }

  // RLD
  } else if (TOKVAL(token, "rld")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0x6f);

  // RLD
  } else if (TOKVAL(token, "rrd")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0x67);

  // GROUP: bit set, reset and test

  } else if (TOKVAL(token, "bit")) {
    z_validate_operands(token, 2, 2);
    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    if (z_typecmp(op1, Z_TOKTYPE_NUMBER)) {
      // BIT b, r
      if (z_is_abcdehl(op2, false)) {
        z_opcode_set(opcode, 2, 0xcb, 0x40 | (z_bit_bits(op1) << 3) | z_reg8_bits(op2));

      // BIT b, [HL]
      } else if (z_is_reg16(op2, "hl", true)) {
        z_opcode_set(opcode, 2, 0xcb, 0x46 | (z_bit_bits(op1) << 3));

      // BIT b, [IX + d]
      } else if (z_is_reg16(op2, "ix", true)) {
        z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0x46 | (z_bit_bits(op1) << 3));
        z_set_offsets(token, 2, op2->children[1]);

      // BIT b, [IY + d]
      } else if (z_is_reg16(op2, "iy", true)) {
        z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0x46 | (z_bit_bits(op1) << 3));
        z_set_offsets(token, 2, op2->children[1]);

      } else {
        match_fail("bit");
      }

    } else {
      match_fail("bit");
    }

  } else if (TOKVAL(token, "set")) {
    z_validate_operands(token, 2, 2);
    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    if (z_typecmp(op1, Z_TOKTYPE_NUMBER)) {
      // SET b, r
      if (z_is_abcdehl(op2, false)) {
        z_opcode_set(opcode, 2, 0xcb, 0xc0 | (z_bit_bits(op1) << 3) | z_reg8_bits(op2));

      // SET b, [HL]
      } else if (z_is_reg16(op2, "hl", true)) {
        z_opcode_set(opcode, 2, 0xcb, 0xc6 | (z_bit_bits(op1) << 3));

      // SET b, [IX + d]
      } else if (z_is_reg16(op2, "ix", true)) {
        z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0xc6 | (z_bit_bits(op1) << 3));
        z_set_offsets(token, 2, op2->children[1]);

      // SET b, [IY + d]
      } else if (z_is_reg16(op2, "iy", true)) {
        z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0xc6 | (z_bit_bits(op1) << 3));
        z_set_offsets(token, 2, op2->children[1]);

      } else {
        match_fail("set");
      }

    } else {
      match_fail("set");
    }

  } else if (TOKVAL(token, "res")) {
    z_validate_operands(token, 2, 2);
    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    if (z_typecmp(op1, Z_TOKTYPE_NUMBER)) {
      // RES b, r
      if (z_is_abcdehl(op2, false)) {
        z_opcode_set(opcode, 2, 0xcb, 0x80 | (z_bit_bits(op1) << 3) | z_reg8_bits(op2));

      // RES b, [HL]
      } else if (z_is_reg16(op2, "hl", true)) {
        z_opcode_set(opcode, 2, 0xcb, 0x86 | (z_bit_bits(op1) << 3));

      // RES b, [IX + d]
      } else if (z_is_reg16(op2, "ix", true)) {
        z_opcode_set(opcode, 4, 0xdd, 0xcb, 0, 0x86 | (z_bit_bits(op1) << 3));
        z_set_offsets(token, 2, op2->children[1]);

      // RES b, [IY + d]
      } else if (z_is_reg16(op2, "iy", true)) {
        z_opcode_set(opcode, 4, 0xfd, 0xcb, 0, 0x86 | (z_bit_bits(op1) << 3));
        z_set_offsets(token, 2, op2->children[1]);

      } else {
        match_fail("res");
      }

    } else {
      match_fail("res");
    }

  // GROUP: jump

  } else if (TOKVAL(token, "jp")) {
    z_validate_operands(token, 1, 2);
    struct z_token_t *op1 = z_get_child(token, 0);

    // JP nn
    if (z_is_num(op1, false)) {
      z_opcode_set(opcode, 3, 0xc3, 0, 0);
      z_set_offsets(token, 1, op1);

    // JP [HL]
    } else if (z_is_reg16(op1, "hl", true)) {
      z_opcode_set(opcode, 1, 0xe9);

    // JP [IX]
    } else if (z_is_reg16(op1, "ix", true)) {
      z_opcode_set(opcode, 2, 0xdd, 0xe9);

    // JP [IY]
    } else if (z_is_reg16(op1, "iy", true)) {
      z_opcode_set(opcode, 2, 0xfd, 0xe9);

    // JP cc, nn
    } else if (z_typecmp(op1, Z_TOKTYPE_CONDITION)) {
      struct z_token_t *op2 = z_get_child(token, 1);

      if (z_is_num(op2, false)) {
        z_opcode_set(opcode, 3, 0xc2 | (cond_bits(op1) << 3), 0, 0);
        z_set_offsets(token, 1, op2);

      } else {
        match_fail("jp");
      }

    } else {
      match_fail("jp");
    }


  } else if (TOKVAL(token, "jr")) {
    z_validate_operands(token, 1, 2);
    struct z_token_t *op1 = z_get_child(token, 0);

    // JR e
    if (z_is_num(op1, false)) {
      z_opcode_set(opcode, 2, 0x18, 0);
      z_set_offsets(token, 1, op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_CONDITION)) {
      struct z_token_t *op2 = z_get_child(token, 1);
      if (z_is_num(op2, false)) {
        // JR C, e
        if (TOKVAL(op1, "c")) {
          z_opcode_set(opcode, 2, 0x38, 0);

        // JR NC, e
        } else if (TOKVAL(op1, "nc")) {
          z_opcode_set(opcode, 2, 0x30, 0);

        // JR Z, e
        } else if (TOKVAL(op1, "z")) {
          z_opcode_set(opcode, 2, 0x28, 0);

        // JR NZ, e
        } else if (TOKVAL(op1, "nz")) {
          z_opcode_set(opcode, 2, 0x20, 0);

        } else {
          match_fail("jr");
        }

        z_set_offsets(token, 1, op2);

      } else {
        match_fail("jr");
      }

    } else {
      match_fail("jr");
    }

  } else if (TOKVAL(token, "djnz")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    if (z_is_num(op1, false)) {
      z_opcode_set(opcode, 2, 0x10, 0);
      z_set_offsets(token, 1, op1);

    } else {
      match_fail("djnz");
    }

  // GROUP: call and return

  } else if (TOKVAL(token, "call")) {
    z_validate_operands(token, 1, 2);
    struct z_token_t *op1 = z_get_child(token, 0);

    // CALL nn
    if (z_is_num(op1, false)) {
      z_opcode_set(opcode, 3, 0xcd, 0, 0);
      z_set_offsets(token, 1, op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_CONDITION)) {
      struct z_token_t *op2 = z_get_child(token, 1);

      // CALL cc, nn
      if (z_is_num(op2, false)) {
        z_opcode_set(opcode, 3, 0xc4 | (cond_bits(op1) << 3), 0, 0);
        z_set_offsets(token, 1, op2);

      } else {
        match_fail("call");
      }

    } else {
      match_fail("call");
    }

  } else if (TOKVAL(token, "ret")) {
    z_validate_operands(token, 0, 1);

    // RET
    if (token->children_count == 0) {
      z_opcode_set(opcode, 1, 0xc9);

    } else {
      struct z_token_t *op1 = z_get_child(token, 0);

      // RET cc
      if (z_typecmp(op1, Z_TOKTYPE_CONDITION)) {
        z_opcode_set(opcode, 1, 0xc0 | (cond_bits(op1) << 3));

      } else {
        match_fail("ret");
      }
    }

  // RETI
  } else if (TOKVAL(token, "reti")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0x4d);

  // RETN
  } else if (TOKVAL(token, "retn")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0x45);

  } else if (TOKVAL(token, "rst")) {
    z_validate_operands(token, 1, 1);
    struct z_token_t *op1 = z_get_child(token, 0);

    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {

      // RST p
      if (z_indexof("\x08\x10\x18\x20\x28\x30\x38", (char)op1->numval) > -1 || op1->numval == 0) {
        z_opcode_set(opcode, 1, 0xc7 | ((op1->numval / 8) << 3));

      } else {
        match_fail("rst");
      }

    } else {
      match_fail("rst");
    }

  // GROUP: input and output

  } else if (TOKVAL(token, "in")) {
    z_validate_operands(token, 2, 2);
    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    // IN A, [n]
    if (z_is_reg8(op1, "a", false) && z_typecmp(op2, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER) && op2->memref) {
      z_opcode_set(opcode, 2, 0xdb, op2->numval);

    // IN r, [C]
    } else if (z_is_abcdehl(op1, false) && z_is_reg8(op2, "c", true)) {
      z_opcode_set(opcode, 2, 0xed, 0x40 | (z_reg8_bits(op1) << 3));

    } else {
      match_fail("in");
    }

  // INI
  } else if (TOKVAL(token, "ini")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xa2);

  // INIR
  } else if (TOKVAL(token, "inir")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xb2);

  // IND
  } else if (TOKVAL(token, "ind")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xaa);

  // INDR
  } else if (TOKVAL(token, "indr")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xba);

  } else if (TOKVAL(token, "out")) {
    z_validate_operands(token, 2, 2);
    struct z_token_t *op1 = z_get_child(token, 0);
    struct z_token_t *op2 = z_get_child(token, 1);

    // OUT [n], A
    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER) && op1->memref && z_is_reg8(op2, "a", false)) {
      z_opcode_set(opcode, 2, 0xd3, op1->numval);

    // IN [C], r
    } else if (z_is_reg8(op1, "c", true) && z_is_abcdehl(op2, false)) {
      z_opcode_set(opcode, 2, 0xed, 0x41 | (z_reg8_bits(op1) << 3));

    } else {
      match_fail("out");
    }

  // OUTI
  } else if (TOKVAL(token, "outi")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xa3);

  // OTIR
  } else if (TOKVAL(token, "otir")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xb3);

  // OUTD
  } else if (TOKVAL(token, "outd")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xab);

  // OTDR
  } else if (TOKVAL(token, "otdr")) {
    z_validate_operands(token, 0, 0);
    z_opcode_set(opcode, 2, 0xed, 0xbb);

  } else {
    z_fail(token, "No match for the instruction '%s'.\n", token->value);
    #ifndef DEBUG
    exit(1);
    #endif
  }

  return opcode;
}
