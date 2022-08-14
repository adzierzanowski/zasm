#include "opcodes.h"

#define fail(...) do {\
  z_fail(token, __VA_ARGS__); \
  exit(1);\
} while (0);

#ifdef DEBUG
#define match_fail(x) do {\
  z_fail(token, __FILE__ ":%d: No match for the '" x "' instruction.\n", __LINE__);\
  exit(1);\
} while (0);
#else
#define match_fail(x) do {\
  z_fail(token, "No match for the '" x "' instruction.\n");\
  exit(1);\
} while (0);
#endif

#define require_operand(op, ins) do {\
  if (!op) {\
    fail("'" ins "' instruction requires operand(s).\n");\
    exit(1);\
  }\
} while (0);

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

static bool z_check_ixiy(struct z_token_t *plus, struct z_token_t *offset) {
  return (
    z_typecmp(plus, Z_TOKTYPE_OPERATOR) &&
    z_streq(plus->value, "+") &&
    z_typecmp(offset, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER));
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
    default: fail("Unknown 8 bit register.\n");
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
  }
}

static void z_ld_reg8(
    struct z_token_t *token,
    struct z_token_t *op1,
    struct z_token_t *op2,
    struct z_opcode_t *opcode) {
  if (z_typecmp(op2, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER | Z_TOKTYPE_IDENTIFIER)) {
    if (op2->memref) {
      if (z_streq(op1->value, "a")) {
        z_opcode_set(opcode, 3, 0x3a, 0, 0);
      } else {
        match_fail("ld reg8, [imm]");
      }

    } else {
      if (z_streq(op1->value, "b")) {
        z_opcode_set(opcode, 2, 0x06, op2->numval);
      } else if (z_streq(op1->value, "c")) {
        z_opcode_set(opcode, 2, 0x0e, op2->numval);
      } else if (z_streq(op1->value, "d")) {
        z_opcode_set(opcode, 2, 0x16, op2->numval);
      } else if (z_streq(op1->value, "e")) {
        z_opcode_set(opcode, 2, 0x1e, op2->numval);
      } else if (z_streq(op1->value, "h")) {
        z_opcode_set(opcode, 2, 0x26, op2->numval);
      } else if (z_streq(op1->value, "l")) {
        z_opcode_set(opcode, 2, 0x2e, op2->numval);
      } else if (z_streq(op1->value, "a")) {
        z_opcode_set(opcode, 2, 0x3e, op2->numval);
      } else {
        match_fail("ld reg8, imm");
      }
    }

    if (z_typecmp(op2, Z_TOKTYPE_NUMBER)) {
      opcode->bytes[1] = op2->numval & 0xff;
      opcode->bytes[2] = op2->numval >> 8;

    } else if (z_typecmp(op2, Z_TOKTYPE_IDENTIFIER)) {
      token->label_offset = 1;
    }

  } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
    if (op2->memref) {
      if (z_streq(op1->value, "a")) {
        if (z_streq(op2->value, "bc")) {
          z_opcode_set(opcode, 1, 0x0a);
        } else if (z_streq(op2->value, "de")) {
          z_opcode_set(opcode, 1, 0x1a);
        } else if (z_streq(op2->value, "hl")) {
          z_opcode_set(opcode, 1, 0x7e);
        } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "ld a, [ix/iy + d]");
          struct z_token_t *op4 = op3->child;
          require_operand(op4, "ld a, [ix/iy + d]");

          if (z_check_ixiy(op3, op4)) {
            if (z_streq(op2->value, "ix")) {
              z_opcode_set(opcode, 3, 0xdd, 0x7e, op4->numval);
            } else if (z_streq(op2->value, "iy")) {
              z_opcode_set(opcode, 3, 0xfd, 0x7e, op4->numval);
            }

          } else {
            match_fail("ld a, [ix + d]");
          }

        } else {
          match_fail("ld a, [reg16]");
        }

      } else if (z_streq(op2->value, "hl")) {
        z_opcode_set(opcode, 1, 0x46 | z_reg8_bits(op1) << 3);

      } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
        struct z_token_t *op3 = op2->child;
        require_operand(op3, "ld reg8, [ix/iy + d]");
        struct z_token_t *op4 = op3->child;
        require_operand(op4, "ld reg8, [ix/iy + d]");

        if (z_check_ixiy(op3, op4)) {
          if (z_streq(op2->value, "ix")) {
            z_opcode_set(
              opcode, 3, 0xdd, 0x46 | (z_reg8_bits(op1) << 3) | op4->numval);
          } else if (z_streq(op2->value, "iy")) {
            z_opcode_set(
              opcode, 3, 0xfd, 0x46 | (z_reg8_bits(op1) << 3) | op4->numval);
          }

        } else {
          match_fail("ld reg8, [ix + d]");
        }

      } else {
        match_fail("ld reg8, [reg16]");
      }

    } else {
      match_fail("ld reg8, reg16");
    }

  } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
    opcode->size = 1;

    if (op2->memref) {
      match_fail("ld reg8, [reg8]");
    }

    if (z_strmatch(op1->value, "a", "b", "c", "d", "e", "h", "l", NULL) &&
        z_strmatch(op2->value, "a", "b", "c", "d", "e", "h", "l", NULL)) {
      z_opcode_set(
        opcode, 1, 0x40 | (z_reg8_bits(op1) << 3) | z_reg8_bits(op2));

    } else if (z_streq(op1->value, "i") && z_streq(op2->value, "a")) {
      z_opcode_set(opcode, 2, 0xed, 0x47);

    } else if (z_streq(op1->value, "r") && z_streq(op2->value, "a")) {
      z_opcode_set(opcode, 2, 0xed, 0x4f);

    } else if (z_streq(op1->value, "a") && z_streq(op2->value, "i")) {
      z_opcode_set(opcode, 2, 0xed, 0x57);

    } else if (z_streq(op1->value, "a") && z_streq(op2->value, "r")) {
      z_opcode_set(opcode, 2, 0xed, 0x5f);

    } else {
      match_fail("ld reg8, reg8");
    }

  } else {
    match_fail("ld reg8");
  }
}

static void z_ld_reg16(
    struct z_token_t *token,
    struct z_token_t *op1,
    struct z_token_t *op2,
    struct z_opcode_t *opcode) {

  if (z_typecmp(op2, Z_TOKTYPE_NUMBER | Z_TOKTYPE_IDENTIFIER)) {
    if (op2->memref) {
      if (z_streq(op1->value, "hl")) {
        z_opcode_set(opcode, 3, 0x2a, 0, 0);

      } else if (z_streq(op1->value, "bc")) {
        z_opcode_set(opcode, 4, 0xed, 0x4b, 0, 0);

      } else if (z_streq(op1->value, "de")) {
        z_opcode_set(opcode, 4, 0xed, 0x5b, 0, 0);

      } else if (z_streq(op1->value, "sp")) {
        z_opcode_set(opcode, 4, 0xed, 0x6b, 0, 0);

      } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
        if (z_streq(op1->value, "ix")) {
          z_opcode_set(opcode, 4, 0xdd, 0x2a, 0, 0);
        } else if (z_streq(op1->value, "iy")) {
          z_opcode_set(opcode, 4, 0xfd, 0x2a, 0, 0);
        }

      } else {
        match_fail("ld reg16, [imm]");
      }

    } else {
      if (z_streq(op1->value, "bc")) {
        z_opcode_set(opcode, 3, 0x01, 0, 0);
      } else if (z_streq(op1->value, "de")) {
        z_opcode_set(opcode, 3, 0x11, 0, 0);
      } else if (z_streq(op1->value, "hl")) {
        z_opcode_set(opcode, 3, 0x21, 0, 0);
      } else if (z_streq(op1->value, "sp")) {
        z_opcode_set(opcode, 3, 0x31, 0, 0);
      } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
        if (z_streq(op1->value, "ix")) {
          z_opcode_set(opcode, 4, 0xdd, 0x21, 0, 0);
        } else if (z_streq(op1->value, "iy")) {
          z_opcode_set(opcode, 4, 0xfd, 0x21, 0, 0);
        }

      } else {
        match_fail("ld reg16, imm");
      }
    }

  } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
    if (op2->memref) {
      match_fail("ld reg16, [reg16]");

    } else {
      if (z_streq(op1->value, "sp") && z_streq(op2->value, "hl")) {
        z_opcode_set(opcode, 1, 0xf9);

      } else if (z_streq(op1->value, "sp") &&
          (z_streq(op2->value, "ix") || z_streq(op2->value, "iy"))) {
        if (z_streq(op2->value, "ix")) {
          z_opcode_set(opcode, 2, 0xdd, 0xf9);
        } else if (z_streq(op2->value, "iy")) {
          z_opcode_set(opcode, 2, 0xfd, 0xf9);
        }

      } else  {
        match_fail("ld reg16, reg16");
      }
    }

  } else {
    match_fail("ld reg16");
  }

  if (z_typecmp(op2, Z_TOKTYPE_NUMBER)) {
    opcode->bytes[opcode->size - 2] = op2->numval & 0xff;
    opcode->bytes[opcode->size - 1] = op2->numval >> 8;

  } else if (z_typecmp(op2, Z_TOKTYPE_IDENTIFIER)) {
    token->label_offset = opcode->size - 2;
  }
}

static void z_ld_reg16_ref(
    struct z_token_t *token,
    struct z_token_t *op1,
    struct z_token_t *op2,
    struct z_opcode_t *opcode) {

  if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
    if (z_streq(op2->value, "a")) {
      if (z_streq(op1->value, "bc")) {
        z_opcode_set(opcode, 1, 0x02);

      } else if (z_streq(op1->value, "de")) {
        z_opcode_set(opcode, 1, 0x12);

      } else if (z_streq(op1->value, "hl")) {
        z_opcode_set(opcode, 1, 0x77);

      } else {
        match_fail("ld [reg16], a");
      }

    } else if (z_streq(op1->value, "hl")) {
      z_opcode_set(opcode, 1, 0x70 | z_reg8_bits(op2));

    } else {
      match_fail("ld [reg16], reg8");
    }

  } else if (z_typecmp(op2, Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR)) {
    if (z_streq(op1->value, "hl")) {
      z_opcode_set(opcode, 2, 0x36, op2->numval);

    } else {
      match_fail("ld [reg16], imm");
    }

  } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "ld [ix/iy + d]");
    struct z_token_t *op3 = op2->child;
    require_operand(op3, "ld [ix/iy + d]");
    struct z_token_t *op4 = op3->child;
    require_operand(op4, "ld [ix/iy + d]");

    if (z_check_ixiy(op2, op3))  {
      if (z_typecmp(op4, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
        if (z_streq(op1->value, "ix")) {
          z_opcode_set(opcode, 4, 0xdd, 0x36, op3->numval, op4->numval);
        } else if (z_streq(op1->value, "iy")) {
          z_opcode_set(opcode, 4, 0xfd, 0x36, op3->numval, op4->numval);
        }

      } else if (z_typecmp(op4, Z_TOKTYPE_REGISTER_8)) {
        if (z_streq(op1->value, "ix")) {
          z_opcode_set(opcode, 3, 0xdd, 0x70 | z_reg8_bits(op4));
        } else if (z_streq(op1->value, "iy")) {
          z_opcode_set(opcode, 3, 0xfd, 0x70 | z_reg8_bits(op4));
        }

      } else {
        match_fail("ld [ix/iy + d]");
      }

    } else if (z_typecmp(op4, Z_TOKTYPE_REGISTER_8)) {
      match_fail("ld [ix/iy + d], reg8")

    } else {
      match_fail("ld [ix/iy + d]");
    }

  } else {
    match_fail("ld [reg16]");
  }
}

static void z_ld_imm_ref(
    struct z_token_t *token,
    struct z_token_t *op1,
    struct z_token_t *op2,
    struct z_opcode_t *opcode) {
  if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
    if (z_streq(op2->value, "a")) {
      z_opcode_set(opcode, 3, 0x32, 0, 0);

    } else {
      match_fail("ld [imm], reg8");
    }

  } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
    if (op2->memref) {
      match_fail("ld [imm], [reg16]");

    } else {
      if (z_streq(op2->value, "hl")) {
        z_opcode_set(opcode, 3, 0x22, 0, 0);

      } else if (z_streq(op2->value, "bc")) {
        z_opcode_set(opcode, 4, 0xed, 0x43, 0, 0);

      } else if (z_streq(op2->value, "de")) {
        z_opcode_set(opcode, 4, 0xed, 0x53, 0, 0);

      } else if (z_streq(op2->value, "sp")) {
        z_opcode_set(opcode, 4, 0xed, 0x73, 0, 0);

      } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
        if (z_streq(op2->value, "ix")) {
          z_opcode_set(opcode, 4, 0xdd, 0x22, 0, 0);
        } else if (z_streq(op2->value, "iy")) {
          z_opcode_set(opcode, 4, 0xfd, 0x22, 0, 0);
        }

      } else {
        match_fail("ld [imm], reg16");
      }
    }

  } else {
    match_fail("ld [imm]");
  }

  if (z_typecmp(op1, Z_TOKTYPE_NUMBER)) {
    opcode->bytes[opcode->size - 2] = op1->numval & 0xff;
    opcode->bytes[opcode->size - 1] = op1->numval >> 8;

  } else if (z_typecmp(op1, Z_TOKTYPE_IDENTIFIER)) {
    token->label_offset = opcode->size = 2;
  }
}

struct z_opcode_t *z_opcode_match(struct z_token_t *token) {
  struct z_opcode_t *opcode = malloc(sizeof (struct z_opcode_t));
  opcode->size = 0;
  struct z_token_t *op1 = token->child;

  if (z_strmatch(token->value,
    "nop", "rlca", "rrca", "rla", "rra", "daa", "cpl", "scf", "ccf", "halt",
    "exx", "di", "ei", "neg", "retn", "reti", "rrd", "rld", "ldi", "cpi", "ini",
    "outi", "ldd", "cpd", "ind", "outd", "ldir", "cpir", "inir", "otir", "lddr",
    "cpdr", "indr", "otdr", NULL)) {
      if (op1) {
        z_fail(token,
          "'%s' instruction doesn't have operands.\n", token->value);
        exit(1);
      }
  }

  if (z_streq(token->value, "adc")) {
    require_operand(op1, "adc");
    struct z_token_t *op2 = op1->child;

    // TODO: check if op1 == a
    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      require_operand(op2, "adc");

      if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
        z_opcode_set(opcode, 1, 0b10001000 | z_reg8_bits(op2));

      } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
        if (op2->memref) {
          if (z_streq(op2->value, "hl")) {
            z_opcode_set(opcode, 1, 0x8e);

          } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
            struct z_token_t *op3 = op2->child;
            require_operand(op3, "adc reg8, [ix/iy + d]");
            struct z_token_t *op4 = op3->child;
            require_operand(op4, "adc reg8, [ix/iy + d]");

            if (z_streq(op1->value, "a") && z_check_ixiy(op3, op4)) {
              if (z_streq(op2->value, "ix")){
                z_opcode_set(opcode, 3, 0xdd, 0x8e, op4->numval);
              } else if (z_streq(op2->value, "iy")) {
                z_opcode_set(opcode, 3, 0xfd, 0x8e, op4->numval);
              }

            } else {
              match_fail("adc reg8, [ix/iy + d]");
            }

          } else {
            match_fail("adc reg8, [reg16]");
          }

        } else {
          match_fail("adc reg8, reg16");
        }

      } else if (z_typecmp(op2, Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR)) {
        z_opcode_set(opcode, 1, 0xce, op2->numval);

      } else {
        match_fail("adc reg8");
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      require_operand(op2, "adc");

      if (op1->memref) {
        if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "adc [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op2, "adc [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 3, 0xdd, 0x9e, op3->numval);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 3, 0xfd, 0x9e, op3->numval);
            }

          } else {
            match_fail("adc [ix/iy + d]")
          }

        } else {
          match_fail("adc [reg16]")
        }

      } else {
        if (z_streq(op1->value, "hl")) {
          if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
            if (op2->memref) {
              match_fail("adc reg16, [reg16]");

            } else {
              if (z_streq(op2->value, "bc")) {
                z_opcode_set(opcode, 2, 0xed, 0x4a);
              } else if (z_streq(op2->value, "de")) {
                z_opcode_set(opcode, 2, 0xed, 0x5a);
              } else if (z_streq(op2->value, "hl")) {
                z_opcode_set(opcode, 2, 0xed, 0x6a);
              } else if (z_streq(op2->value, "sp")) {
                z_opcode_set(opcode, 2, 0xed, 0x7a);
              } else {
                match_fail("adc reg16, reg16");
              }
            }

          } else {
            match_fail("adc reg16");
          }

        } else {
          match_fail("adc reg16");
        }
      }


    } else {
      match_fail("adc");
    }

  } else if (z_streq(token->value, "add")) {
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "add");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (z_streq(op1->value, "hl")) {
        if (op1->memref) {
          match_fail("add");

        } else {
          if (op2->type == Z_TOKTYPE_REGISTER_16) {
            if (op2->memref) {
              match_fail("add");

            } else {
              if (z_streq(op2->value, "bc")) {
                z_opcode_set(opcode, 1, 0x09);
              } else if (z_streq(op2->value, "de")) {
                z_opcode_set(opcode, 1, 0x19);
              } else if (z_streq(op2->value, "hl")) {
                z_opcode_set(opcode, 1, 0x29);
              } else if (z_streq(op2->value, "sp")) {
                z_opcode_set(opcode, 1, 0x39);
              } else {
                match_fail("add reg16, reg16");
              }
            }

          } else {
            match_fail("add reg16");
          }
        }

      } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
        struct z_token_t *op2 = op1->child;
        require_operand(op2, "add ix/iy");

        if (op1->memref) {
          match_fail("");

        } else {
          if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
            if (op2->memref) {
              match_fail("add ix/iy, [reg16]");

            } else {
              opcode->size = 2;
              if (z_streq(op1->value, "ix")) {
                opcode->bytes[0] = 0xdd;

              } else if (z_streq(op1->value, "iy")) {
                opcode->bytes[0] = 0xfd;
              }

              if (z_streq(op2->value, "bc")) {
                opcode->bytes[1] = 0b00001001;
              } else if (z_streq(op2->value, "de")) {
                opcode->bytes[1] = 0b00011001;
              } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
                opcode->bytes[1] = 0b00101001;
              } else if (z_streq(op2->value, "sp")) {
                opcode->bytes[1] = 0b00111001;

              } else {
                match_fail("add ix/iy, reg16");
              }
            }

          } else {
            match_fail("add ix/iy");
          }
        }

      } else {
        match_fail("add");
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {

        if (z_streq(op1->value, "a")) {
          z_opcode_set(opcode, 1, 0x80 | z_reg8_bits(op2));

        } else {
          match_fail("add reg8, reg8");
        }

      } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
        if (op2->memref) {
          if (z_streq(op2->value, "hl")) {
            z_opcode_set(opcode, 1, 0x86);

          } else if (z_streq(op1->value, "a") &&
              (z_streq(op2->value, "ix") || z_streq(op2->value, "iy"))) {
            struct z_token_t *op3 = op2->child;
            require_operand(op3, "add reg8, [ix/iy + d]");
            struct z_token_t *op4 = op3->child;
            require_operand(op4, "add reg8, [ix/iy + d]");

            if (z_check_ixiy(op3, op4)) {
              if (z_streq(op2->value, "ix")) {
                z_opcode_set(opcode, 2, 0xdd, 0x86);
              } else if (z_streq(op2->value, "iy")) {
                z_opcode_set(opcode, 2, 0xfd, 0x86);
              }

            } else {
              printf("op4->value %s\n", op3->value);
              match_fail("add")
            }

          } else {
            match_fail("add reg8, [reg16]");
          }

        } else {
          match_fail("add reg8, reg16");
        }

      } else if (z_typecmp(op2, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
        z_opcode_set(opcode, 2, 0xc6, op2->numval);

      } else {
        match_fail("add reg8");
      }

    } else {
      match_fail("add");
    }

  } else if (z_streq(token->value, "and")) {
    require_operand(op1, "and");
    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 1, 0xa0 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0xa6);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "and [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "and [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 3, 0xdd, 0xa6, op3->numval);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 3, 0xfd, 0xa6, op3->numval);
            }

          } else {
            match_fail("and [ix/iy + d]");
          }

        } else {
          match_fail("and [reg16]");
        }

      } else {
        match_fail("and reg16");
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      z_opcode_set(opcode, 1, 0xe6, op1->numval);

    } else {
      match_fail("and");
    }

  } else if (z_streq(token->value, "bit")) {
    require_operand(op1, "bit");
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "bit");

    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      if (op1->memref) {
        match_fail("bit [imm]");
      } else {
        if (op1->numval > 7) {
          match_fail("bit imm");
        } else {
          if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
            if (op2->memref) {
              match_fail("bit imm, [reg8]");

            } else {
              z_opcode_set(
                opcode, 2, 0xcb, 0x40 | (op1->numval << 3) | z_reg8_bits(op2));
            }

          } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
            if (op2->memref) {
              if (z_streq(op2->value, "hl")) {
                z_opcode_set(opcode, 2, 0xcb, 0x46 | (op1->numval << 3));

              } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
                struct z_token_t *op3 = op2->child;
                require_operand(op3, "bit n, [ix/iy + d]");
                struct z_token_t *op4 = op3->child;
                require_operand(op4, "bit n, [ix/iy + d]");

                if (z_check_ixiy(op3, op4)) {
                  if (z_streq(op2->value, "ix")) {
                    z_opcode_set(opcode,
                      4, 0xdd, 0xcb, op4->numval, 0x46 | (op1->numval << 3));
                  } else if (z_streq(op2->value, "iy")) {
                    z_opcode_set(opcode,
                      4, 0xfd, 0xcb, op4->numval, 0x46 | (op1->numval << 3));
                  }

                } else {
                  match_fail("bit n, [ix/iy + d]");
                }

              } else {
                match_fail("bit imm, [reg16]");
              }

            } else {
              match_fail("bit imm, reg16");
            }

          } else {
            match_fail("bit imm");
          }
        }
      }
    }

  } else if (z_streq(token->value, "call")) {
    require_operand(op1, "call");

    if (z_typecmp(op1, Z_TOKTYPE_CONDITION)) {
      struct z_token_t *op2 = op1->child;
      require_operand(op2, "call cond");

      if (z_typecmp(op2, Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
        z_opcode_set(opcode, 3, 0xc4 | (cond_bits(op1) << 3), 0, 0);

      } else {
        match_fail("call cond, imm");
      }

      if (z_typecmp(op2, Z_TOKTYPE_IDENTIFIER)) {
        token->label_offset = 1;

      } else if (z_typecmp(op2, Z_TOKTYPE_NUMBER)) {
        opcode->bytes[1] = op2->numval & 0xff;
        opcode->bytes[2] = op2->numval >> 8;
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
      z_opcode_set(opcode, 3, 0xcd, 0, 0);

      if (z_typecmp(op1, Z_TOKTYPE_IDENTIFIER)) {
        token->label_offset = 1;
      } else {
        opcode->bytes[1] = op1->numval & 0xff;
        opcode->bytes[2] = op1->numval >> 8;
     }

    } else {
      match_fail("call");
    }

  } else if (z_streq(token->value, "ccf")) {
    z_opcode_set(opcode, 1, 0x3f);

  } else if (z_streq(token->value, "cp")) {
    require_operand(op1, "cp");

    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      z_opcode_set(opcode, 2, 0xfe, op1->numval);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 1, 0xb8 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0xbe);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "cp [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "cp [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 3, 0xdd, 0xbe, op3->numval);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 3, 0xfd, 0xbe, op3->numval);
            }

          } else {
            match_fail("cp [ix/iy + d]");
          }

        } else {
          match_fail("cp [reg16]");
        }

      } else {
        match_fail("cp [reg16]");
      }

    } else {
      match_fail("cp");
    }

  } else if (z_streq(token->value, "cpi")) {
    z_opcode_set(opcode, 2, 0xed, 0xa1);

  } else if (z_streq(token->value, "cpir")) {
    z_opcode_set(opcode, 2, 0xed, 0xb1);

  } else if (z_streq(token->value, "cpd")) {
    z_opcode_set(opcode, 2, 0xed, 0xa9);

  } else if (z_streq(token->value, "cpdr")) {
    z_opcode_set(opcode, 2, 0xed, 0xb9);

  } else if (z_streq(token->value, "cpl")) {
    z_opcode_set(opcode, 1, 0x2f);

  } else if (z_streq(token->value, "daa")) {
    z_opcode_set(opcode, 1, 0x27);

  } else if (z_streq(token->value, "dec")) {
    if (op1->type == Z_TOKTYPE_REGISTER_8) {
      z_opcode_set(opcode, 1, 0x05 | (z_reg8_bits(op1) << 3));

    } else if (op1->type == Z_TOKTYPE_REGISTER_16) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0x35);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "inc [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "inc [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 3, 0xdd, 0x35, op3->numval);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 3, 0xfd, 0x35, op3->numval);
            }

          } else {
            match_fail("inc [ix + d]");
          }

        } else {
          match_fail("dec [reg16]");
        }

      } else {
        if (z_streq(op1->value, "bc")) {
          z_opcode_set(opcode, 1, 0x0b);
        } else if (z_streq(op1->value, "de")) {
          z_opcode_set(opcode, 1, 0x1b);
        } else if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0x2b);
        } else if (z_streq(op1->value, "sp")) {
          z_opcode_set(opcode, 1, 0x3b);
        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          if (z_streq(op1->value, "ix")) {
            z_opcode_set(opcode, 2, 0xdd, 0x2b);
          } else if (z_streq(op1->value, "iy")) {
            z_opcode_set(opcode, 2, 0xfd, 0x2b);
          }
        } else {
          match_fail("dec reg16");
        }
      }

    } else {
      match_fail("dec");
    }

  } else if (z_streq(token->value, "di")) {
    z_opcode_set(opcode, 1, 0xf3);

  } else if (z_streq(token->value, "djnz")) {
    require_operand(op1, "djnz");

    if (op1->type == Z_TOKTYPE_NUMBER) {
      z_opcode_set(opcode, 2, 0x10, op1->numval - 2);

    } else {
      match_fail("djnz");
    }

  } else if (z_streq(token->value, "ei")) {
    z_opcode_set(opcode, 1, 0xfb);

  } else if (z_streq(token->value, "ex")) {
    require_operand(op1, "ex");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      struct z_token_t *op2 = op1->child;
      require_operand(op2, "ex");

      if (op1->memref) {
        if (z_streq(op1->value, "sp")) {
          if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
            if (op2->memref) {
              match_fail("ex [reg16], [reg16]");

            } else {
              if (z_streq(op2->value, "hl")) {
                z_opcode_set(opcode, 1, 0xe3);

              } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
                if (z_streq(op2->value, "ix")) {
                  z_opcode_set(opcode, 2, 0xdd, 0xe3);
                } else if (z_streq(op2->value, "iy")) {
                  z_opcode_set(opcode, 2, 0xfd, 0xe3);
                }

              } else {
                match_fail("ex [reg16], reg16");
              }
            }

          } else {
            match_fail("ex [reg16]");
          }

        } else {
          match_fail("ex [reg16]");
        }

      } else {
        if (op2->type & Z_TOKTYPE_REGISTER_16) {
          if (op2->memref) {
            match_fail("ex reg16, [reg16]");

          } else {
            if (z_streq(op1->value, "af") && z_streq(op2->value, "af")) {
              z_opcode_set(opcode, 1, 0x08);

            } else if (z_streq(op1->value, "de") && z_streq(op2->value, "hl")) {
              z_opcode_set(opcode, 1, 0xeb);

            } else {
              match_fail("ex reg16, reg16");
            }
          }

        } else {
          match_fail("ex reg16");
        }
      }

    } else {
      match_fail("ex");
    }

  } else if (z_streq(token->value, "exx")) {
    z_opcode_set(opcode, 1, 0xd9);

  } else if (z_streq(token->value, "halt")) {
    z_opcode_set(opcode, 1, 0x76);

  } else if (z_streq(token->value, "im")) {
    require_operand(op1, "im");

    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {

      switch (op1->numval) {
        case 0:
          z_opcode_set(opcode, 2, 0xed, 0x46);
          break;
        case 1:
          z_opcode_set(opcode, 2, 0xed, 0x56);
          break;
        case 2:
          z_opcode_set(opcode, 2, 0xed, 0x5e);
          break;
        default:
          match_fail("im imm");
      }

    } else {
      match_fail("im");
    }

  } else if (z_streq(token->value, "in")) {
    require_operand(op1, "in");
    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      if (op1->memref) {
        match_fail("in [reg8]");

      } else {
        struct z_token_t *op2 = op1->child;
        require_operand(op2, "in reg8");

        if (z_typecmp(op2, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
          if (op2->memref) {
            if (z_streq(op1->value, "a")) {
              z_opcode_set(opcode, 2, 0xdb, op2->numval);

            } else {
              match_fail("in reg8, [imm]");
            }

          } else {
            match_fail("in reg8, imm");
          }

        } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
          if (op2->memref) {
            if (z_streq(op2->value, "c")) {
              z_opcode_set(opcode, 2, 0xed, 0x40 | (z_reg8_bits(op1) << 3));

            } else {
              match_fail("in reg8, [reg8]");
            }

          } else {
            match_fail("in reg8, reg8");
          }

        } else {
          match_fail("in reg8");
        }
      }

    } else {
      match_fail("in");
    }

  } else if (z_streq(token->value, "ind")) {
    z_opcode_set(opcode, 2, 0xed, 0xaa);

  } else if (z_streq(token->value, "indr")) {
    z_opcode_set(opcode, 2, 0xed, 0xba);

  } else if (z_streq(token->value, "ini")) {
    z_opcode_set(opcode, 2, 0xed, 0xa2);

  } else if (z_streq(token->value, "inir")) {
    z_opcode_set(opcode, 2, 0xed, 0xb2);

  } else if (z_streq(token->value, "inc")) {
    require_operand(op1, "inc");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 1, 0x04 | (z_reg8_bits(op1) << 3));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0x34);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "inc ix/iy");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "inc ix/iy");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 3, 0xdd, 0x34, op3->numval);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 3, 0xfd, 0x34, op3->numval);
            }

          } else {
            match_fail("inc [ix/iy]");
          }

        } else {
          match_fail("inc [reg16]");
        }

      } else {
        if (z_streq(op1->value, "bc")) {
          z_opcode_set(opcode, 1, 0x03);
        } else if (z_streq(op1->value, "de")) {
          z_opcode_set(opcode, 1, 0x13);
        } else if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0x23);
        } else if (z_streq(op1->value, "sp")) {
          z_opcode_set(opcode, 1, 0x33);
        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          if (z_streq(op1->value, "ix")) {
            z_opcode_set(opcode, 2, 0xdd, 0x23);
          } else if (z_streq(op1->value, "iy")) {
            z_opcode_set(opcode, 2, 0xfd, 0x23);
          }

        } else {
          match_fail("inc reg16");
        }
      }

    } else {
      match_fail("inc");
    }

  } else if (z_streq(token->value, "ld")) {
    require_operand(op1, "ld");
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "ld");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      if (op1->memref) {
        match_fail("ld [reg8]");
      } else {
        z_ld_reg8(token, op1, op2, opcode);
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        z_ld_reg16_ref(token, op1, op2, opcode);

      } else {
        z_ld_reg16(token, op1, op2, opcode);
      }

    } else if (z_typecmp(
        op1, Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR | Z_TOKTYPE_IDENTIFIER)) {
      if (op1->memref) {
        z_ld_imm_ref(token, op1, op2, opcode);

      } else {
        match_fail("ld [imm]")
      }

    } else {
      match_fail("ld");
    }

  } else if (z_streq(token->value, "ldi")) {
    z_opcode_set(opcode, 2, 0xed, 0xa0);

  } else if (z_streq(token->value, "ldir")) {
    z_opcode_set(opcode, 2, 0xed, 0xb0);

  } else if (z_streq(token->value, "ldd")) {
    z_opcode_set(opcode, 2, 0xed, 0xa8);

  } else if (z_streq(token->value, "lddr")) {
    z_opcode_set(opcode, 2, 0xed, 0xb8);

  } else if (z_streq(token->value, "jp")) {
    require_operand(op1, "jp");

    if (z_typecmp(op1, Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
      z_opcode_set(opcode, 3, 0xc3, 0, 0);

      if (op1->type == Z_TOKTYPE_NUMBER) {
        opcode->bytes[1] = op1->numval & 0xff;
        opcode->bytes[2] = op1->numval >> 8;

      } else if (op1->type == Z_TOKTYPE_IDENTIFIER) {
        token->label_offset = 1;
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0xe9);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          if (z_streq(op1->value, "ix")) {
            z_opcode_set(opcode, 2, 0xdd, 0xe9);
          } else if (z_streq(op1->value, "iy")) {
            z_opcode_set(opcode, 2, 0xfd, 0xe9);
          }

        } else {
          match_fail("jp [reg16]");
        }

      } else {
        match_fail("jp reg16");
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_CONDITION)) {
      struct z_token_t *op2 = op1->child;
      require_operand(op2, "jp");

      if (z_typecmp(op2, Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
        if (op2->memref) {
          match_fail("jp cond, [imm]");

        } else {
          z_opcode_set(opcode, 3, 0xc2 | (cond_bits(op1) << 3));

          if (z_typecmp(op2, Z_TOKTYPE_IDENTIFIER)) {
            token->label_offset = 1;

          } else if (z_typecmp(op2, Z_TOKTYPE_NUMBER)) {
            opcode->bytes[1] = op2->numval & 0xff;
            opcode->bytes[2] = op2->numval >> 8;
          }
        }

      } else {
        match_fail("jp cond");
      }

    } else {
      match_fail("jp");
    }

  } else if (z_streq(token->value, "jr")) {
    require_operand(op1, "jr");

    if (z_typecmp(op1, Z_TOKTYPE_NUMBER)) {
      z_opcode_set(opcode, 2, 0x18, op1->numval - 2);

    } else if (z_typecmp(op1, Z_TOKTYPE_CONDITION)) {
      struct z_token_t *op2 = op1->child;
      require_operand(op2, "jr");

      if (z_typecmp(op2, Z_TOKTYPE_NUMBER)) {
        if (z_streq(op1->value, "nz")) {
          z_opcode_set(opcode, 2, 0x20, op2->numval - 2);

        } else if (z_streq(op1->value, "z")) {
          z_opcode_set(opcode, 2, 0x28, op2->numval - 2);

        } else if (z_streq(op1->value, "nc")) {
          z_opcode_set(opcode, 2, 0x30, op2->numval - 2);

        } else if (z_streq(op1->value, "c")) {
          z_opcode_set(opcode, 2, 0x38, op2->numval - 2);
        }

      } else {
        match_fail("jr cond");
      }

    } else {
      match_fail("jr");
    }

  } else if (z_streq(token->value, "neg")) {
    z_opcode_set(opcode, 2, 0xed, 0x44);

  } else if (z_streq(token->value, "nop")) {
    z_opcode_set(opcode, 1, 0x00);

  } else if (z_streq(token->value, "or")) {
    require_operand(op1, "or");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 1, 0xb0 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0xb6);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "or [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "or [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 3, 0xdd, 0xb6, op3->numval);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 3, 0xfd, 0xb6, op3->numval);
            }

          } else {
            match_fail("or [ix + d]");
          }

        } else {
          match_fail("or [reg16]");
        }

      } else {
        match_fail("or reg16");
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      if (op1->memref) {
        match_fail("or [imm]");

      } else {
        z_opcode_set(opcode, 2, 0xf6, op1->numval);
      }

    } else {
      match_fail("or");
    }

  } else if (z_streq(token->value, "otir")) {
    z_opcode_set(opcode, 2, 0xed, 0xb3);

  } else if (z_streq(token->value, "otdr")) {
    z_opcode_set(opcode, 2, 0xed, 0xbb);

  } else if (z_streq(token->value, "out")) {
    require_operand(op1, "out");
    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      if (op1->memref) {
        struct z_token_t *op2 = op1->child;
        require_operand(op2, "out [imm]");

        if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
          if (z_streq(op2->value, "a")) {
            z_opcode_set(opcode, 2, 0xd3, op2->numval);

          } else {
            match_fail("out [imm], reg8");
          }

        } else  {
          match_fail("out [imm]");
        }

      } else {
        match_fail("out imm");
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      struct z_token_t *op2 = op1->child;
      require_operand(op2, "out");

      if (op1->memref) {
        if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
          if (op2->memref) {
            match_fail("out [reg8], [reg8]")

          } else {
            if (z_streq(op1->value, "c")) {
              z_opcode_set(opcode, 2, 0xed, 0x41 | (z_reg8_bits(op2) << 3));

            } else {
              match_fail("out [reg8], reg8")
            }
          }

        } else {
          match_fail("out [reg8]")
        }

      } else {
        match_fail("out reg8");
      }

    } else {
      match_fail("out");
    }

  } else if (z_streq(token->value, "outd")) {
    z_opcode_set(opcode, 2, 0xed, 0xab);

  } else if (z_streq(token->value, "outi")) {
    z_opcode_set(opcode, 2, 0xed, 0xa3);

  } else if (z_streq(token->value, "pop")) {
    require_operand(op1, "pop");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        match_fail("pop [reg16]");

      } else {
        if (z_streq(op1->value, "bc")) {
          z_opcode_set(opcode, 1, 0xc1);
        } else if (z_streq(op1->value, "de")) {
          z_opcode_set(opcode, 1, 0xd1);
        } else if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0xe1);
        } else if (z_streq(op1->value, "af")) {
          z_opcode_set(opcode, 1, 0xf1);
        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          if (z_streq(op1->value, "ix")) {
            z_opcode_set(opcode, 2, 0xdd, 0xe1);
          } else if (z_streq(op1->value, "iy")) {
            z_opcode_set(opcode, 2, 0xfd, 0xe1);
          }

        } else  {
          match_fail("pop reg16");
        }
      }

    } else {
      match_fail("pop");
    }

  } else if (z_streq(token->value, "push")) {
    require_operand(op1, "push");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        match_fail("push [reg16]");

      } else {
        opcode->size = 1;
        if (z_streq(op1->value, "bc")) {
          z_opcode_set(opcode, 1, 0xc5);
        } else if (z_streq(op1->value, "de")) {
          z_opcode_set(opcode, 1, 0xd5);
        } else if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0xe5);
        } else if (z_streq(op1->value, "af")) {
          z_opcode_set(opcode, 1, 0xf5);
        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          if (z_streq(op1->value, "ix")) {
            z_opcode_set(opcode, 2, 0xdd, 0xe5);
          } else if (z_streq(op1->value, "iy")) {
            z_opcode_set(opcode, 2, 0xfd, 0xe5);
          }

        } else {
          match_fail("push reg16");
        }
      }

    } else {
      match_fail("push");
    }

  } else if (z_streq(token->value, "res")) {
    require_operand(op1, "res");
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "res");

    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      if (op1->memref) {
        match_fail("res [imm]");
      } else {
        if (op1->numval > 7) {
          match_fail("res imm");
        } else {
          if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
            if (op2->memref) {
              match_fail("res imm, [reg8]");

            } else {
              z_opcode_set(
                opcode, 2, 0xcb, 0x80 | (op1->numval << 3) | z_reg8_bits(op2));
            }

          } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
            if (op2->memref) {
              if (z_streq(op2->value, "hl")) {
                z_opcode_set(opcode, 2, 0xcb, 0x86 | (op1->numval << 3));

              } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
                struct z_token_t *op3 = op2->child;
                require_operand(op3, "res n, [ix/iy + d]");
                struct z_token_t *op4 = op3->child;
                require_operand(op4, "res n, [ix/iy + d]");

                if (z_check_ixiy(op3, op4)) {
                  if (z_streq(op2->value, "ix")) {
                    z_opcode_set(opcode,
                      4, 0xdd, 0xcb, op4->numval, 0x86 | (op1->numval << 3));
                  } else if (z_streq(op2->value, "iy")) {
                    z_opcode_set(opcode,
                      4, 0xfd, 0xcb, op4->numval, 0x86 | (op1->numval << 3));
                  }

                } else {
                  match_fail("res n, [ix/iy + d]");
                }

              } else {
                match_fail("res imm, [reg16]");
              }

            } else {
              match_fail("res imm, reg16");
            }

          } else {
            match_fail("res imm");
          }
        }
      }
    }

  } else if (z_streq(token->value, "ret")) {
    if (!op1) {
      z_opcode_set(opcode, 1, 0xc9);

    } else if (z_typecmp(op1, Z_TOKTYPE_CONDITION)) {
      z_opcode_set(opcode, 1, 0xc0 | (cond_bits(op1) << 3));

    } else {
      match_fail("ret cond");
    }

  } else if (z_streq(token->value, "reti")) {
    z_opcode_set(opcode, 2, 0xed, 0x4d);

  } else if (z_streq(token->value, "retn")) {
    z_opcode_set(opcode, 2, 0xed, 0x45);

  } else if (z_streq(token->value, "rl")) {
    require_operand(op1, "rl");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 2, 0xcb, 0x10 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 2, 0xcb, 0x16);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "rl [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "rl [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 4, 0xdd, 0xcb, op3->numval, 0x16);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 4, 0xfd, 0xcb, op3->numval, 0x16);
            }

          } else {
            match_fail("rl [ix/iy + d]");
          }

        } else {
          match_fail("rl [reg16]");
        }

      } else {
        match_fail("rl reg16");
      }

    } else {
      match_fail("rl");
    }

  } else if (z_streq(token->value, "rla")) {
    z_opcode_set(opcode, 1, 0x17);

  } else if (z_streq(token->value, "rlc")) {
    require_operand(op1, "rlc");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 2, 0xcb, z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 2, 0xcb, 0x06);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "rlc [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "rlc [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 4, 0xdd, 0xcb, op3->numval, 0x06);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 4, 0xfd, 0xcb, op3->numval, 0x06);
            }
          }

        } else {
          match_fail("rlc [reg16]");
        }

      } else {
        match_fail("rlc reg16");

      }

    } else {
      match_fail("rlc");
    }

  } else if (z_streq(token->value, "rlca")) {
    z_opcode_set(opcode, 1, 0x07);

  } else if (z_streq(token->value, "rld")) {
    z_opcode_set(opcode, 2, 0xed, 0x6f);

  } else if (z_streq(token->value, "rr")) {
    require_operand(op1, "rr");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 2, 0xcb, 0x18 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 2, 0xcb, 0x1e);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "rr [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "rr [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 4, 0xdd, 0xcb, op3->numval, 0x1e);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 4, 0xfd, 0xcb, op3->numval, 0x1e);
            }

          } else {
            match_fail("rr [ix/iy + d]");
          }

        } else {
          match_fail("rr [reg16]");
        }

      } else {
        match_fail("rr reg16");
      }

    } else {
      match_fail("rr");
    }

  } else if (z_streq(token->value, "rra")) {
    z_opcode_set(opcode, 1, 0x1f);

  } else if (z_streq(token->value, "rrc")) {
    require_operand(op1, "rrc");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 2, 0xcb, 0x08 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 2, 0xcb, 0x0e);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "rrc [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "rrc [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 4, 0xdd, 0xcb, op3->numval, 0x0e);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 4, 0xfd, 0xcb, op3->numval, 0x0e);
            }

          } else {
            match_fail("rrc [ix/iy + d]");
          }

        } else {
          match_fail("rrc [reg16]");
        }

      } else {
        match_fail("rrc reg16");
      }

    } else {
      match_fail("rrc");
    }

  } else if (z_streq(token->value, "rrca")) {
    z_opcode_set(opcode, 1, 0x0f);

  } else if (z_streq(token->value, "rrd")) {
    z_opcode_set(opcode, 2, 0xed, 0x67);

  } else if (z_streq(token->value, "rst")) {
    require_operand(op1, "rst");

    if (z_typecmp(op1, Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR)) {
      switch (op1->numval) {
        case 0x00:
          z_opcode_set(opcode, 1, 0xc7);
          break;
        case 0x08:
          z_opcode_set(opcode, 1, 0xcf);
          break;
        case 0x10:
          z_opcode_set(opcode, 1, 0xd7);
          break;
        case 0x18:
          z_opcode_set(opcode, 1, 0xdf);
          break;
        case 0x20:
          z_opcode_set(opcode, 1, 0xe7);
          break;
        case 0x28:
          z_opcode_set(opcode, 1, 0xef);
          break;
          break;
        case 0x30:
          z_opcode_set(opcode, 1, 0xf7);
          break;
        case 0x38:
          z_opcode_set(opcode, 1, 0xff);
          break;
        default:
          match_fail("rst imm");
          break;
      }

    } else {
      match_fail("rst");
    }

  } else if (z_streq(token->value, "sbc")) {
    require_operand(op1, "sbc");
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "sbc");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      if (z_streq(op1->value, "a")) {
        if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
          z_opcode_set(opcode, 1, 0x98 | z_reg8_bits(op2));

        } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
          if (op2->memref) {
            if (z_streq(op2->value, "hl")) {
              z_opcode_set(opcode, 1, 0x9e);

            } else {
              match_fail("sbc a, [reg16]");
            }

          } else {
            match_fail("sbc a, reg16")
          }

        } else if (z_typecmp(op2, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
          z_opcode_set(opcode, 2, 0xde, op2->numval);

        } else {
          match_fail("sbc a");
        }

      } else {
        match_fail("sbc reg8");
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      struct z_token_t *op2 = op1->child;
      require_operand(op2, "sbc");

      if (op1->memref) {
        match_fail("sbc [reg16]");

      } else {
        if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
          if (op2->memref) {
            match_fail("sbc reg16, [reg16]");
          } else {
            if (z_streq(op1->value, "hl")) {
              if (z_streq(op2->value, "bc")) {
                z_opcode_set(opcode, 2, 0xed, 0x42);
              } else if (z_streq(op2->value, "de")) {
                z_opcode_set(opcode, 2, 0xed, 0x52);
              } else if (z_streq(op2->value, "hl")) {
                z_opcode_set(opcode, 2, 0xed, 0x62);
              } else if (z_streq(op2->value, "sp")) {
                z_opcode_set(opcode, 2, 0xed, 0x72);
              } else {
                match_fail("sbc hl, reg16");
              }

            } else {
              match_fail("sbc reg16, reg16");
            }
          }
        }
      }

    } else {
      match_fail("sbc");
    }

  } else if (z_streq(token->value, "set")) {
    require_operand(op1, "set");
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "set");

    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      if (op1->memref) {
        match_fail("set [imm]");
      } else {
        if (op1->numval > 7) {
          match_fail("set imm");
        } else {
          if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
            if (op2->memref) {
              match_fail("set imm, [reg8]");

            } else {
              z_opcode_set(
                opcode, 2, 0xcb, 0xc0 | (op1->numval << 3) | z_reg8_bits(op2));
            }

          } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
            if (op2->memref) {
              if (z_streq(op2->value, "hl")) {
                z_opcode_set(opcode, 2, 0xcb, 0xc6 | (op1->numval << 3));

              } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
                struct z_token_t *op3 = op2->child;
                require_operand(op3, "set n, [ix/iy + d]");
                struct z_token_t *op4 = op3->child;
                require_operand(op4, "set n, [ix/iy + d]");

                if (z_check_ixiy(op3, op4)) {
                  if (z_streq(op2->value, "ix")) {
                    z_opcode_set(opcode,
                      4, 0xdd, 0xcb, op4->numval, 0xc6 | (op1->numval << 3));
                  } else if (z_streq(op2->value, "iy")) {
                    z_opcode_set(opcode,
                      4, 0xfd, 0xcb, op4->numval, 0xc6 | (op1->numval << 3));
                  }

                } else {
                  match_fail("set n, [ix/iy + d]");
                }

              } else {
                match_fail("set imm, [reg16]");
              }

            } else {
              match_fail("set imm, reg16");
            }

          } else {
            match_fail("set imm");
          }
        }
      }
    }

  } else if (z_streq(token->value, "scf")) {
    z_opcode_set(opcode, 1, 0x37);

  } else if (z_streq(token->value, "sla")) {
    require_operand(op1, "sla");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 2, 0xcb, 0x20 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 2, 0xcb, 0x26);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "sla [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "sla [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 4, 0xdd, 0xcb, op3->numval, 0x26);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 4, 0xfd, 0xcb, op3->numval, 0x26);
            }

          } else {
            match_fail("sla [ix/iy + d]");
          }

        } else {
          match_fail("sla [reg16]");
        }

      } else {
        match_fail("sla reg16");
      }

    } else {
      match_fail("sla");
    }

  } else if (z_streq(token->value, "sra")) {
    require_operand(op1, "sra");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 2, 0xcb, 0x28 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 2, 0xcb, 0x2e);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "sra [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "sra [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 4, 0xdd, 0xcb, op3->numval, 0x2e);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 4, 0xfd, 0xcb, op3->numval, 0x2e);
            }

          } else {
            match_fail("sra [ix/iy + d]");
          }

        } else {
          match_fail("sra [reg16]");
        }

      } else {
        match_fail("sra reg16");
      }

    } else {
      match_fail("sra");
    }

  } else if (z_streq(token->value, "srl")) {
    require_operand(op1, "srl");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 2, 0xcb, 0x38 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 2, 0xcb, 0x3e);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "srl [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "srl [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 4, 0xdd, 0xcb, op3->numval, 0x3e);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 4, 0xfd, 0xcb, op3->numval, 0x3e);
            }

          } else {
            match_fail("srl [ix/iy + d]");
          }

        } else {
          match_fail("srl [reg16]");
        }

      } else {
        match_fail("srl reg16");
      }

    } else {
      match_fail("srl");
    }

  } else if (z_streq(token->value, "sub")) {
    require_operand(op1, "sub");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 1, 0x90 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0x96);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "sub [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "sub [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 3, 0xdd, 0x96, op3->numval);
            } else if (z_streq(op1->value, "iy")){
              z_opcode_set(opcode, 3, 0xfd, 0x96, op3->numval);
            }

          } else {
            match_fail("sub [ix + d]");
          }


        } else {
          match_fail("sub [reg16]");
        }

      } else {
        match_fail("sub");
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      z_opcode_set(opcode, 2, 0xd6, op1->numval);

    } else {
      match_fail("sub");
    }

  } else if (z_streq(token->value, "xor")) {
    require_operand(op1, "xor");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      z_opcode_set(opcode, 1, 0xa8 | z_reg8_bits(op1));

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          z_opcode_set(opcode, 1, 0xae);

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "xor [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "xor [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            if (z_streq(op1->value, "ix")) {
              z_opcode_set(opcode, 3, 0xdd, 0xae, op3->numval);
            } else if (z_streq(op1->value, "iy")) {
              z_opcode_set(opcode, 3, 0xfd, 0xae, op3->numval);
            }

          } else {
            match_fail("xor [ix/iy + d]");
          }

        } else {
          match_fail("xor [reg16]");
        }

      } else {
        match_fail("xor reg16");
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      z_opcode_set(opcode, 2, 0xee, op1->numval);

    } else {
      match_fail("xor");
    }

  } else {
    fail("No match for instruction '%s'\n", token->value);
  }

  return opcode;
}
