#include "opcodes.h"

#define fail(...) do {\
  z_fail(token->fname, token->line, token->col, __VA_ARGS__); \
  exit(1);\
} while (0);

#define match_fail(x) do {\
  z_fail(token->fname, token->line, token->col, __FILE__ ":%d: No match for the '" x "' instruction.\n", __LINE__);\
  exit(1);\
} while (0);

#define require_operand(op, ins) do {\
  if (!op) {\
    fail("'" ins "' instruction requires operand(s).\n");\
    exit(1);\
  }\
} while (0);

static bool z_check_ixiy(struct z_token_t *plus, struct z_token_t *offset) {
  return (
    z_typecmp(plus, Z_TOKTYPE_OPERATOR) &&
    z_streq(plus->value, "+") &&
    z_typecmp(offset, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER));
}

static uint8_t z_reg8_bits(struct z_token_t *token) {
  switch (token->value[0]) {
    case 'a': return 0b111;
    case 'b': return 0b000;
    case 'c': return 0b001;
    case 'd': return 0b010;
    case 'e': return 0b011;
    case 'h': return 0b100;
    case 'l': return 0b101;
    default: fail("Unknown 8 bit register.\n");
  }
}

static uint8_t cond_bits(struct z_token_t *token) {
  if (z_streq(token->value, "nz")) {
    return 0b000;
  } else if (z_streq(token->value, "z")) {
    return 0b001;
  } else if (z_streq(token->value, "nc")) {
    return 0b010;
  } else if (z_streq(token->value, "c")) {
    return 0b011;
  } else if (z_streq(token->value, "po")) {
    return 0b100;
  } else if (z_streq(token->value, "pe")) {
    return 0b101;
  } else if (z_streq(token->value, "p")) {
    return 0b110;
  } else if (z_streq(token->value, "m")) {
    return 0b111;
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
      opcode->size = 3;

      if (z_streq(op1->value, "a")) {
        opcode->bytes[0] = 0x3a;
      } else {
        match_fail("ld reg8, [imm]");
      }

    } else {
      opcode->size = 2;

      if (z_streq(op1->value, "b")) {
        opcode->bytes[0] = 0x06;
      } else if (z_streq(op1->value, "c")) {
        opcode->bytes[0] = 0x0e;
      } else if (z_streq(op1->value, "d")) {
        opcode->bytes[0] = 0x16;
      } else if (z_streq(op1->value, "e")) {
        opcode->bytes[0] = 0x1e;
      } else if (z_streq(op1->value, "h")) {
        opcode->bytes[0] = 0x26;
      } else if (z_streq(op1->value, "l")) {
        opcode->bytes[0] = 0x2e;
      } else if (z_streq(op1->value, "a")) {
        opcode->bytes[0] = 0x3e;
      } else {
        match_fail("ld reg8, imm");
      }

      opcode->bytes[1] = op2->numval;
    }

    if (z_typecmp(op2, Z_TOKTYPE_NUMBER)) {
      opcode->bytes[1] = op2->numval & 0xff;
      opcode->bytes[2] = op2->numval >> 8;

    } else if (z_typecmp(op2, Z_TOKTYPE_IDENTIFIER)) {
      token->label_offset = 1;
    }

  } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
    opcode->size = 1;

    if (op2->memref) {
      if (z_streq(op1->value, "a")) {

        if (z_streq(op2->value, "bc")) {
          opcode->bytes[0] = 0x0a;
        } else if (z_streq(op2->value, "de")) {
          opcode->bytes[0] = 0x1a;
        } else if (z_streq(op2->value, "hl")) {
          opcode->bytes[0] = 0x7e;
        } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "ld a, [ix/iy + d]");
          struct z_token_t *op4 = op3->child;
          require_operand(op4, "ld a, [ix/iy + d]");

          if (z_check_ixiy(op3, op4)) {
            opcode->size = 3;
            if (z_streq(op2->value, "ix")) {
              opcode->bytes[0] = 0xdd;
            } else if (z_streq(op2->value, "iy")) {
              opcode->bytes[0] = 0xfd;
            }
            opcode->bytes[1] = 0x7e;
            opcode->bytes[2] = op4->numval;

          } else {
            match_fail("ld a, [ix + d]");
          }

        } else {
          match_fail("ld a, [reg16]");
        }


      } else if (z_streq(op2->value, "hl")) {
        opcode->bytes[0] = 0b01000110;
        uint8_t reg_bits = z_reg8_bits(op1);
        opcode->bytes[0] |= (reg_bits << 3);

      } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
        struct z_token_t *op3 = op2->child;
        require_operand(op3, "ld reg8, [ix/iy + d]");
        struct z_token_t *op4 = op3->child;
        require_operand(op4, "ld reg8, [ix/iy + d]");

        if (z_check_ixiy(op3, op4)) {
          opcode->size = 3;
          if (z_streq(op2->value, "ix")) {
            opcode->bytes[0] = 0xdd;
          } else if (z_streq(op2->value, "iy")) {
            opcode->bytes[0] = 0xfd;
          }
          opcode->bytes[1] = 0b01000110 | (z_reg8_bits(op1) << 3);
          opcode->bytes[2] = op4->numval;

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

    if (z_strmatch(op1->value, "a", "b", "c", "d", "e", "h", "l", NULL) &&
        z_strmatch(op2->value, "a", "b", "c", "d", "e", "h", "l", NULL)) {
      opcode->bytes[0] = 0b01000000;

      uint8_t op1_bits = z_reg8_bits(op1);
      uint8_t op2_bits = z_reg8_bits(op2);
      opcode->bytes[0] |= (op1_bits << 3) | op2_bits;

    } else if (z_streq(op1->value, "i") && z_streq(op2->value, "a")) {
      opcode->size = 2;
      opcode->bytes[0] = 0xed;
      opcode->bytes[1] = 0x47;

    } else if (z_streq(op1->value, "r") && z_streq(op2->value, "a")) {
      opcode->size = 2;
      opcode->bytes[0] = 0xed;
      opcode->bytes[1] = 0x4f;

    } else if (z_streq(op1->value, "a") && z_streq(op2->value, "i")) {
      opcode->size = 2;
      opcode->bytes[0] = 0xed;
      opcode->bytes[1] = 0x57;

    } else if (z_streq(op1->value, "a") && z_streq(op2->value, "r")) {
      opcode->size = 2;
      opcode->bytes[0] = 0xed;
      opcode->bytes[1] = 0x5f;

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
    opcode->size = 3;

    if (op2->memref) {
      if (z_streq(op1->value, "hl")) {
        opcode->bytes[0] = 0x2a;
      } else if (z_streq(op1->value, "bc")) {
        opcode->size = 4;
        opcode->bytes[0] = 0xed;
        opcode->bytes[1] = 0b01001011;

      } else if (z_streq(op1->value, "de")) {
        opcode->size = 4;
        opcode->bytes[0] = 0xed;
        opcode->bytes[1] = 0b01011011;

      } else if (z_streq(op1->value, "sp")) {
        opcode->size = 4;
        opcode->bytes[0] = 0xed;
        opcode->bytes[1] = 0b01111011;

      } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
        opcode->size = 4;
        if (z_streq(op1->value, "ix")) {
          opcode->bytes[0] = 0xdd;
        } else if (z_streq(op1->value, "iy")) {
          opcode->bytes[0] = 0xfd;
        }
        opcode->bytes[1] = 0x2a;

      } else {
        match_fail("ld reg16, [imm]");
      }

    } else {
      if (z_streq(op1->value, "bc")) {
        opcode->bytes[0] = 0x01;
      } else if (z_streq(op1->value, "de")) {
        opcode->bytes[0] = 0x11;
      } else if (z_streq(op1->value, "hl")) {
        opcode->bytes[0] = 0x21;
      } else if (z_streq(op1->value, "sp")) {
        opcode->bytes[0] = 0x31;
      } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
        opcode->size = 4;
        if (z_streq(op1->value, "ix")) {
          opcode->bytes[0] = 0xdd;
        } else if (z_streq(op1->value, "iy")) {
          opcode->bytes[0] = 0xfd;
        }

        opcode->bytes[1] = 0x21;

      } else {
        match_fail("ld reg16, imm");
      }
    }

  } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
    if (op2->memref) {
      match_fail("ld reg16, [reg16]");

    } else {
      if (z_streq(op1->value, "sp") && z_streq(op2->value, "hl")) {
        opcode->size = 1;
        opcode->bytes[0] = 0xf9;
      } else if (z_streq(op1->value, "sp") &&
          (z_streq(op2->value, "ix") || z_streq(op2->value, "iy"))) {
        opcode->size = 2;
        if (z_streq(op2->value, "ix")) {
          opcode->bytes[0] = 0xdd;
        } else if (z_streq(op2->value, "iy")) {
          opcode->bytes[0] = 0xfd;
        }
        opcode->bytes[1] = 0xf9;

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
    opcode->size = 1;

    if (z_streq(op2->value, "a")) {
      if (z_streq(op1->value, "bc")) {
        opcode->bytes[0] = 0x02;

      } else if (z_streq(op1->value, "de")) {
        opcode->bytes[0] = 0x12;

      } else if (z_streq(op1->value, "hl")) {
        opcode->bytes[0] = 0x77;

      } else {
        match_fail("ld [reg16], a");
      }

    } else if (z_streq(op1->value, "hl")) {
      opcode->bytes[0] = 0b1110000;
      uint8_t op2_bits = z_reg8_bits(op2);
      opcode->bytes[0] |= op2_bits;

    } else {
      match_fail("ld [reg16], reg8");
    }

  } else if (z_typecmp(op2, Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR)) {
    opcode->size = 2;

    if (z_streq(op1->value, "hl")) {
      opcode->bytes[0] = 0x36;

    } else {
      match_fail("ld [reg16], imm");
    }

    opcode->bytes[1] = op2->numval;

  } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "ld [ix/iy + d]");
    struct z_token_t *op3 = op2->child;
    require_operand(op3, "ld [ix/iy + d]");
    struct z_token_t *op4 = op3->child;
    require_operand(op4, "ld [ix/iy + d]");

    if (z_check_ixiy(op2, op3))  {
      if (z_typecmp(op4, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
        opcode->size = 4;
        if (z_streq(op1->value, "ix")) {
          opcode->bytes[0] = 0xdd;
        } else if (z_streq(op1->value, "iy")) {
          opcode->bytes[0] = 0xfd;
        }
        opcode->bytes[1] = 0x36;
        opcode->bytes[2] = op3->numval;
        opcode->bytes[3] = op4->numval;

      } else if (z_typecmp(op4, Z_TOKTYPE_REGISTER_8)) {
        opcode->size = 3;
        if (z_streq(op1->value, "ix")) {
          opcode->bytes[0] = 0xdd;
        } else if (z_streq(op1->value, "iy")) {
          opcode->bytes[0] = 0xfd;
        }
        opcode->bytes[1] = 0b01110000 | z_reg8_bits(op4);
        opcode->bytes[2] = op3->numval;

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
  opcode->size = 3;

  if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
    if (z_streq(op2->value, "a")) {
      opcode->bytes[0] = 0x32;

    } else {
      match_fail("ld [imm], reg8");
    }

  } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
    if (op2->memref) {
      match_fail("ld [imm], [reg16]");

    } else {
      if (z_streq(op2->value, "hl")) {
        opcode->bytes[0] = 0x22;

      } else if (z_streq(op2->value, "bc")) {
        opcode->size = 4;
        opcode->bytes[0] = 0xed;
        opcode->bytes[1] = 0b01000011;

      } else if (z_streq(op2->value, "de")) {
        opcode->size = 4;
        opcode->bytes[0] = 0xed;
        opcode->bytes[1] = 0x53;

      } else if (z_streq(op2->value, "sp")) {
        opcode->size = 4;
        opcode->bytes[0] = 0xed;
        opcode->bytes[1] = 0x73;

      } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
        opcode->size = 4;

        if (z_streq(op2->value, "ix")) {
          opcode->bytes[0] = 0xdd;
        } else if (z_streq(op2->value, "iy")) {
          opcode->bytes[0] = 0xfd;
        }

        opcode->bytes[1] = 0x22;

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

  if (z_streq(token->value, "adc")) {
    require_operand(op1, "adc");
    struct z_token_t *op2 = op1->child;

    // TODO: check if op1 == a
    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      require_operand(op2, "adc");

      if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
        opcode->size = 1;
        opcode->bytes[0] = 0b10001000 | z_reg8_bits(op2);

      } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
        if (op2->memref) {
          if (z_streq(op2->value, "hl")) {
            opcode->size = 1;
            opcode->bytes[0] = 0x8e;

          } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
            struct z_token_t *op3 = op2->child;
            require_operand(op3, "adc reg8, [ix/iy + d]");
            struct z_token_t *op4 = op3->child;
            require_operand(op4, "adc reg8, [ix/iy + d]");

            if (z_streq(op1->value, "a") && z_check_ixiy(op3, op4)) {
              opcode->size = 3;
              if (z_streq(op2->value, "ix")){
                opcode->bytes[0] = 0xdd;
              } else if (z_streq(op2->value, "iy")) {
                opcode->bytes[0] = 0xfd;
              }
              opcode->bytes[1] = 0x8e;
              opcode->bytes[2] = op4->numval;

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
        opcode->size = 1;
        opcode->bytes[0] = 0xce;
        opcode->bytes[1] = op2->numval;

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
            opcode->size = 3;
            if (z_streq(op1->value, "ix")) {
              opcode->bytes[0] = 0xdd;
            } else if (z_streq(op1->value, "iy")) {
              opcode->bytes[0] = 0xfd;
            }

            opcode->bytes[1] = 0x9e;
            opcode->bytes[2] = op3->numval;

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
              opcode->size = 2;
              opcode->bytes[0] = 0xed;

              if (z_streq(op2->value, "bc")) {
                opcode->bytes[1] = 0b01001010;
              } else if (z_streq(op2->value, "de")) {
                opcode->bytes[1] = 0b01011010;
              } else if (z_streq(op2->value, "hl")) {
                opcode->bytes[1] = 0b01101010;
              } else if (z_streq(op2->value, "sp")) {
                opcode->bytes[1] = 0b01111010;
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
    opcode->size = 1;

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (z_streq(op1->value, "hl")) {
        if (op1->memref) {
          match_fail("add");

        } else {
          if (op2->type == Z_TOKTYPE_REGISTER_16) {
            if (op2->memref) {
              match_fail("add");

            } else {
              switch (op2->value[0]) {
                case 'b':
                  opcode->bytes[0] = 0b00001001;
                  break;
                case 'd':
                  opcode->bytes[0] = 0b00011001;
                  break;
                case 'h':
                  opcode->bytes[0] = 0b00101001;
                  break;
                case 's':
                  opcode->bytes[0] = 0b00111001;
                  break;
                default:
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
        opcode->size = 1;

        if (z_streq(op1->value, "a")) {
          opcode->bytes[0] = 0b10000000 | z_reg8_bits(op2);

        } else {
          match_fail("add reg8, reg8");
        }

      } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
        if (op2->memref) {
          if (z_streq(op2->value, "hl")) {
            opcode->size = 1;
            opcode->bytes[0] = 0x86;

          } else if (z_streq(op1->value, "a") &&
              (z_streq(op2->value, "ix") || z_streq(op2->value, "iy"))) {
            struct z_token_t *op3 = op2->child;
            require_operand(op3, "add reg8, [ix/iy + d]");
            struct z_token_t *op4 = op3->child;
            require_operand(op4, "add reg8, [ix/iy + d]");

            if (z_check_ixiy(op3, op4)) {
              opcode->size = 2;
              if (z_streq(op2->value, "ix")) {
                opcode->bytes[0] = 0xdd;
              } else if (z_streq(op2->value, "iy")) {
                opcode->bytes[0] = 0xfd;
              }
              opcode->bytes[1] = 0x86;

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
        opcode->size = 2;
        opcode->bytes[0] = 0xc6;
        opcode->bytes[1] = op2->numval;

      } else {
        match_fail("add reg8");
      }

    } else {
      match_fail("add");
    }

  } else if (z_streq(token->value, "and")) {
    require_operand(op1, "and");
    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      opcode->size = 1;
      opcode->bytes[0] = 0b10100000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 1;
          opcode->bytes[0] = 0xa6;

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "and [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "and [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            opcode->size = 3;
            if (z_streq(op1->value, "ix")) {
              opcode->bytes[0] = 0xdd;
            } else if (z_streq(op1->value, "iy")) {
              opcode->bytes[0] = 0xfd;
            }
            opcode->bytes[1] = 0xa6;
            opcode->bytes[2] = op3->numval;

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
      opcode->size = 1;
      opcode->bytes[0] = 0xe6;
      opcode->bytes[1] = op1->numval;

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
              opcode->size = 2;
              opcode->bytes[0] = 0xcb;
              opcode->bytes[1] = 0b01000000 | (op1->numval << 3) | z_reg8_bits(op2);
            }

          } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
            if (op2->memref) {
              if (z_streq(op2->value, "hl")) {
                opcode->size = 2;
                opcode->bytes[0] = 0xcb;
                opcode->bytes[1] = 0b01000110 | (op1->numval << 3);

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
      opcode->size = 3;

      if (z_typecmp(op2, Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
        opcode->bytes[0] = 0b11000100 | (cond_bits(op1) << 3);

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
      opcode->size = 3;
      opcode->bytes[0] = 0xcd;

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
    opcode->size = 1;
    opcode->bytes[0] = 0x3f;

  } else if (z_streq(token->value, "cp")) {
    require_operand(op1, "cp");

    if (op1->type & (Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      opcode->size = 2;
      opcode->bytes[0] = 0xfe;

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      opcode->size = 1;
      opcode->bytes[0] = 0b10111000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 1;
          opcode->bytes[0] = 0xbe;

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "cp [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "cp [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            opcode->size = 3;
            if (z_streq(op1->value, "ix")) {
              opcode->bytes[0] = 0xdd;
            } else if (z_streq(op1->value, "iy")) {
              opcode->bytes[0] = 0xfd;
            }
            opcode->bytes[1] = 0xbe;
            opcode->bytes[2] = op3->numval;

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
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xa1;

  } else if (z_streq(token->value, "cpir")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xb1;

  } else if (z_streq(token->value, "cpd")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xa9;

  } else if (z_streq(token->value, "cpdr")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xb9;

  } else if (z_streq(token->value, "cpl")) {
    opcode->size = 1;
    opcode->bytes[0] = 0x2f;

  } else if (z_streq(token->value, "daa")) {
    opcode->size = 1;
    opcode->bytes[0] = 0x27;

  } else if (z_streq(token->value, "dec")) {

    if (op1->type == Z_TOKTYPE_REGISTER_8) {
      opcode->size = 1;

      switch (op1->value[0]) {
        case 'a':
          opcode->bytes[0] = 0b00111101;
          break;
        case 'b':
          opcode->bytes[0] = 0b00000101;
          break;
        case 'c':
          opcode->bytes[0] = 0b00001101;
          break;
        case 'd':
          opcode->bytes[0] = 0b00010101;
          break;
        case 'e':
          opcode->bytes[0] = 0b00011101;
          break;
        case 'h':
          opcode->bytes[0] = 0b00100101;
          break;
        case 'l':
          opcode->bytes[0] = 0b00101101;
          break;
        default:
          match_fail("dec");
          break;
      }

    } else if (op1->type == Z_TOKTYPE_REGISTER_16) {
      opcode->size = 1;

      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->bytes[0] = 0x35;

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "inc [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "inc [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            opcode->size = 3;
            if (z_streq(op1->value, "ix")) {
              opcode->bytes[0] = 0xdd;
            } else if (z_streq(op1->value, "iy")) {
              opcode->bytes[0] = 0xfd;
            }
            opcode->bytes[1] = 0x35;
            opcode->bytes[2] = op3->numval;

          } else {
            match_fail("inc [ix + d]");
          }


        } else {
          match_fail("dec [reg16]");
        }

      } else {
        if (z_streq(op1->value, "bc")) {
          opcode->bytes[0] = 0b00001011;
        } else if (z_streq(op1->value, "de")) {
          opcode->bytes[0] = 0b00011011;
        } else if (z_streq(op1->value, "hl")) {
          opcode->bytes[0] = 0b00101011;
        } else if (z_streq(op1->value, "sp")) {
          opcode->bytes[0] = 0b00111011;
        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          opcode->size = 2;
          if (z_streq(op1->value, "ix")) {
            opcode->bytes[0] = 0xdd;
          } else if (z_streq(op1->value, "iy")) {
            opcode->bytes[0] = 0xfd;
          }
          opcode->bytes[1] = 0x2b;
        } else {
          match_fail("dec reg16");
        }
      }

    } else {
      match_fail("dec");
    }

  } else if (z_streq(token->value, "di")) {
    opcode->size = 1;
    opcode->bytes[0] = 0xf3;

  } else if (z_streq(token->value, "djnz")) {
    require_operand(op1, "djnz");

    if (op1->type == Z_TOKTYPE_NUMBER) {
      opcode->size = 2;
      opcode->bytes[0] = 0x10;
      opcode->bytes[1] = op1->numval - 2;

    } else {
      match_fail("djnz");
    }

  } else if (z_streq(token->value, "ei")) {
    opcode->size = 1;
    opcode->bytes[0] = 0xfb;

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
                opcode->size = 1;
                opcode->bytes[0] = 0xe3;

              } else if (z_streq(op2->value, "ix") || z_streq(op2->value, "iy")) {
                opcode->size = 2;
                if (z_streq(op2->value, "ix")) {
                  opcode->bytes[0] = 0xdd;
                } else if (z_streq(op2->value, "iy")) {
                  opcode->bytes[0] = 0xfd;
                }
                opcode->bytes[1] = 0xe3;

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
              opcode->size = 1;
              opcode->bytes[0] = 0x08;

            } else if (z_streq(op1->value, "de") && z_streq(op2->value, "hl")) {
              opcode->size = 1;
              opcode->bytes[0] = 0xeb;

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
    opcode->size = 1;
    opcode->bytes[0] = 0xd9;

  } else if (z_streq(token->value, "halt")) {
    opcode->size = 1;
    opcode->bytes[0] = 0x76;

  } else if (z_streq(token->value, "im")) {
    require_operand(op1, "im");

    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      opcode->size = 2;

      opcode->bytes[0] = 0xed;
      switch (op1->numval) {
        case 0:
          opcode->bytes[1] = 0x46;
          break;
        case 1:
          opcode->bytes[1] = 0x56;
          break;
        case 2:
          opcode->bytes[1] = 0x5e;
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
              opcode->size = 2;
              opcode->bytes[0] = 0xdb;
              opcode->bytes[1] = op2->numval;

            } else {
              match_fail("in reg8, [imm]");
            }

          } else {
            match_fail("in reg8, imm");
          }

        } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
          if (op2->memref) {
            if (z_streq(op2->value, "c")) {
              opcode->size = 2;
              opcode->bytes[0] = 0xed;
              opcode->bytes[1] = 0b01000000 | (z_reg8_bits(op1) << 3);

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
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xaa;

  } else if (z_streq(token->value, "indr")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xba;

  } else if (z_streq(token->value, "ini")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xa2;

  } else if (z_streq(token->value, "inir")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xb2;

  } else if (z_streq(token->value, "inc")) {
    require_operand(op1, "inc");

    if (op1->type == Z_TOKTYPE_REGISTER_8) {
      opcode->size = 1;

      switch (op1->value[0]) {
        case 'a':
          opcode->bytes[0] = 0b00111100;
          break;
        case 'b':
          opcode->bytes[0] = 0b00000100;
          break;
        case 'c':
          opcode->bytes[0] = 0b00001100;
          break;
        case 'd':
          opcode->bytes[0] = 0b00010100;
          break;
        case 'e':
          opcode->bytes[0] = 0b00011100;
          break;
        case 'h':
          opcode->bytes[0] = 0b00100100;
          break;
        case 'l':
          opcode->bytes[0] = 0b00101100;
          break;
        default:
          match_fail("inc");
          break;
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      opcode->size = 1;

      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->bytes[0] = 0x34;

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "inc ix/iy");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "inc ix/iy");

          if (z_check_ixiy(op2, op3)) {
            opcode->size = 3;
            if (z_streq(op1->value, "ix")) {
              opcode->bytes[0] = 0xdd;
            } else if (z_streq(op1->value, "iy")) {
              opcode->bytes[0] = 0xfd;
            }

            opcode->bytes[1] = 0x34;
            opcode->bytes[2] = op3->numval;

          } else {
            match_fail("inc [ix/iy]");
          }

        } else {
          match_fail("inc [reg16]");
        }

      } else {
        if (z_streq(op1->value, "bc")) {
          opcode->bytes[0] = 0b00000011;
        } else if (z_streq(op1->value, "de")) {
          opcode->bytes[0] = 0b00010011;
        } else if (z_streq(op1->value, "hl")) {
          opcode->bytes[0] = 0b00100011;
        } else if (z_streq(op1->value, "sp")) {
          opcode->bytes[0] = 0x33;
        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          opcode->size = 2;
          if (z_streq(op1->value, "ix")) {
            opcode->bytes[0] = 0xdd;
          } else if (z_streq(op1->value, "iy")) {
            opcode->bytes[0] = 0xfd;
          }

          opcode->bytes[1] = 0x23;
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
      z_ld_reg8(token, op1, op2, opcode);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        z_ld_reg16_ref(token, op1, op2, opcode);

      } else {
        z_ld_reg16(token, op1, op2, opcode);
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR | Z_TOKTYPE_IDENTIFIER)) {
      if (op1->memref) {
        z_ld_imm_ref(token, op1, op2, opcode);

      } else {
        match_fail("ld [imm]")
      }

    } else {
      match_fail("ld");
    }

  } else if (z_streq(token->value, "ldi")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xa0;

  } else if (z_streq(token->value, "ldir")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xb0;

  } else if (z_streq(token->value, "ldd")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xa8;

  } else if (z_streq(token->value, "lddr")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xb8;

  } else if (z_streq(token->value, "jp")) {
    require_operand(op1, "jp");

    opcode->size = 3;

    if (z_typecmp(op1, Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
      opcode->bytes[0] = 0xc3;

      if (op1->type == Z_TOKTYPE_NUMBER) {
        opcode->bytes[1] = op1->numval & 0xff;
        opcode->bytes[2] = op1->numval >> 8;

      } else if (op1->type == Z_TOKTYPE_IDENTIFIER) {
        token->label_offset = 1;
      }

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 1;
          opcode->bytes[0] = 0xe9;

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          opcode->size = 2;
          if (z_streq(op1->value, "ix")) {
            opcode->bytes[0] = 0xdd;
          } else if (z_streq(op1->value, "iy")) {
            opcode->bytes[0] = 0xfd;
          }
          opcode->bytes[1] = 0xe9;

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
          opcode->bytes[0] = 0b11000010 | (cond_bits(op1) << 3);

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
    opcode->size = 2;

    if (op1->type == Z_TOKTYPE_NUMBER) {
      opcode->bytes[0] = 0x18;
      opcode->bytes[1] = op1->numval - 2;

    } else if (op1->type == Z_TOKTYPE_CONDITION) {
      struct z_token_t *op2 = op1->child;
      require_operand(op2, "jr");

      if (z_streq(op1->value, "nz")) {
        opcode->bytes[0] = 0x20;

      } else if (z_streq(op1->value, "z")) {
        opcode->bytes[0] = 0x28;

      } else if (z_streq(op1->value, "nc")) {
        opcode->bytes[0] = 0x30;

      } else if (z_streq(op1->value, "c")) {
        opcode->bytes[0] = 0x38;

      } else {
        match_fail("jr cond");
      }

      if (op2->type == Z_TOKTYPE_NUMBER) {
        opcode->bytes[1] = op2->numval - 2;

      } else {
        match_fail("jr cond");
      }

    } else {
      match_fail("jr");
    }

  } else if (z_streq(token->value, "neg")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0x44;


  } else if (z_streq(token->value, "nop")) {
    opcode->size = 1;
    opcode->bytes[0] = 0;

  } else if (z_streq(token->value, "or")) {
    require_operand(op1, "or");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      opcode->size = 1;
      opcode->bytes[0] = 0b10110000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 1;
          opcode->bytes[0] = 0xb6;

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "or [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "or [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            opcode->size = 3;
            if (z_streq(op1->value, "ix")) {
              opcode->bytes[0] = 0xdd;
            } else if (z_streq(op1->value, "iy")) {
              opcode->bytes[0] = 0xfd;
            }

            opcode->bytes[1] = 0xb6;
            opcode->bytes[2] = op3->numval;

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
        opcode->size = 2;
        opcode->bytes[0] = 0xf6;
        opcode->bytes[1] = op1->numval;
      }

    } else {
      match_fail("or");
    }

  } else if (z_streq(token->value, "otir")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xb3;

  } else if (z_streq(token->value, "otdr")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xbb;

  } else if (z_streq(token->value, "out")) {
    require_operand(op1, "out");
    if (z_typecmp(op1, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      if (op1->memref) {
        opcode->size = 2;

        struct z_token_t *op2 = op1->child;
        require_operand(op2, "out [imm]");

        if (z_typecmp(op2, Z_TOKTYPE_REGISTER_8)) {
          if (z_streq(op2->value, "a")) {
            opcode->bytes[0] = 0xd3;
            opcode->bytes[1] = op2->numval;

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
              opcode->size = 2;
              opcode->bytes[0] = 0xed;
              opcode->bytes[1] = 0b01000001 | (z_reg8_bits(op2) << 3);

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
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xab;

  } else if (z_streq(token->value, "outi")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xa3;

  } else if (z_streq(token->value, "pop")) {
    require_operand(op1, "pop");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        match_fail("pop [reg16]");

      } else {
        opcode->size = 1;

        if (z_streq(op1->value, "bc")) {
          opcode->bytes[0] = 0b11000001;
        } else if (z_streq(op1->value, "de")) {
          opcode->bytes[0] = 0b11010001;
        } else if (z_streq(op1->value, "hl")) {
          opcode->bytes[0] = 0b11100001;
        } else if (z_streq(op1->value, "af")) {
          opcode->bytes[0] = 0b11110001;
        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          opcode->size = 2;
          if (z_streq(op1->value, "ix")) {
            opcode->bytes[0] = 0xdd;
          } else if (z_streq(op1->value, "iy")) {
            opcode->bytes[0] = 0xfd;
          }
          opcode->bytes[1] = 0xe1;

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
          opcode->bytes[0] = 0b11000101;
        } else if (z_streq(op1->value, "de")) {
          opcode->bytes[0] = 0b11010101;
        } else if (z_streq(op1->value, "hl")) {
          opcode->bytes[0] = 0b11100101;
        } else if (z_streq(op1->value, "af")) {
          opcode->bytes[0] = 0b11110101;
        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          opcode->size = 2;
          if (z_streq(op1->value, "ix")) {
            opcode->bytes[0] = 0xdd;
          } else if (z_streq(op1->value, "iy")) {
            opcode->bytes[0] = 0xfd;
          }
          opcode->bytes[1] = 0xe5;

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
              opcode->size = 2;
              opcode->bytes[0] = 0xcb;
              opcode->bytes[1] = 0b10000000 | (op1->numval << 3) | z_reg8_bits(op2);
            }

          } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
            if (op2->memref) {
              if (z_streq(op2->value, "hl")) {
                opcode->size = 2;
                opcode->bytes[0] = 0xcb;
                opcode->bytes[1] = 0b10000110 | (op1->numval << 3);

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
    opcode->size = 1;

    if (!op1) {
      opcode->bytes[0] = 0xc9;

    } else if (z_typecmp(op1, Z_TOKTYPE_CONDITION)) {
      opcode->bytes[0] = 0b11000000 | (cond_bits(op1) << 3);

    } else {
      match_fail("ret cond");
    }

  } else if (z_streq(token->value, "reti")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0x4d;

  } else if (z_streq(token->value, "retn")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0x45;

  } else if (z_streq(token->value, "rl")) {
    require_operand(op1, "rl");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      opcode->size = 2;
      opcode->bytes[0] = 0xcb;
      opcode->bytes[1] = 0b00010000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 2;
          opcode->bytes[0] = 0xcb;
          opcode->bytes[1] = 0x16;

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
    opcode->size = 1;
    opcode->bytes[0] = 0x17;

  } else if (z_streq(token->value, "rlc")) {
    require_operand(op1, "rlc");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      opcode->size = 2;
      opcode->bytes[0] = 0xcb;
      opcode->bytes[1] = z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 2;
          opcode->bytes[0] = 0xcb;
          opcode->bytes[1] = 0x06;

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
    opcode->size = 1;
    opcode->bytes[0] = 0x07;

  } else if (z_streq(token->value, "rld")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0x6f;

  } else if (z_streq(token->value, "rr")) {
    require_operand(op1, "rr");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      opcode->size = 2;
      opcode->bytes[0] = 0xcb;
      opcode->bytes[1] = 0b00011000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 2;
          opcode->bytes[0] = 0xcb;
          opcode->bytes[1] = 0x1e;

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
    opcode->size = 1;
    opcode->bytes[0] = 0x1f;

  } else if (z_streq(token->value, "rrc")) {
    require_operand(op1, "rrc");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      opcode->size = 2;
      opcode->bytes[0] = 0xcb;
      opcode->bytes[1] = 0b00001000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 2;
          opcode->bytes[0] = 0xcb;
          opcode->bytes[1] = 0x0e;

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
    opcode->size = 1;
    opcode->bytes[0] = 0x0f;

  } else if (z_streq(token->value, "rrd")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0x67;

  } else if (z_streq(token->value, "rst")) {
    require_operand(op1, "rst");

    if (z_typecmp(op1, Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR)) {
      opcode->size = 1;
      switch (op1->numval) {
        case 0x00:
          opcode->bytes[0] = 0xc7;
          break;
        case 0x08:
          opcode->bytes[0] = 0xcf;
          break;
        case 0x10:
          opcode->bytes[0] = 0xd7;
          break;
        case 0x18:
          opcode->bytes[0] = 0xdf;
          break;
        case 0x20:
          opcode->bytes[0] = 0xe7;
          break;
        case 0x28:
          opcode->bytes[0] = 0xef;
          break;
        case 0x30:
          opcode->bytes[0] = 0xf7;
          break;
        case 0x38:
          opcode->bytes[0] = 0xff;
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
          opcode->size = 1;
          opcode->bytes[0] = 0b10011000 | z_reg8_bits(op2);

        } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
          if (op2->memref) {
            if (z_streq(op2->value, "hl")) {
              opcode->size = 1;
              opcode->bytes[0] = 0x9e;

            } else {
              match_fail("sbc a, [reg16]");
            }

          } else {
            match_fail("sbc a, reg16")

          }

        } else if (z_typecmp(op2, Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
          opcode->size = 2;
          opcode->bytes[0] = 0xde;
          opcode->bytes[1] = op2->numval;

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
              opcode->size = 2;
              opcode->bytes[0] = 0xed;

              if (z_streq(op2->value, "bc")) {
                opcode->bytes[1] = 0b01000010;
              } else if (z_streq(op2->value, "de")) {
                opcode->bytes[1] = 0b01010010;
              } else if (z_streq(op2->value, "hl")) {
                opcode->bytes[1] = 0b01100010;
              } else if (z_streq(op2->value, "sp")) {
                opcode->bytes[1] = 0b01110010;
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
              opcode->size = 2;
              opcode->bytes[0] = 0xcb;
              opcode->bytes[1] = 0b11000000 | (op1->numval << 3) | z_reg8_bits(op2);
            }

          } else if (z_typecmp(op2, Z_TOKTYPE_REGISTER_16)) {
            if (op2->memref) {
              if (z_streq(op2->value, "hl")) {
                opcode->size = 2;
                opcode->bytes[0] = 0xcb;
                opcode->bytes[1] = 0b11000110 | (op1->numval << 3);

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
    opcode->size = 1;
    opcode->bytes[0] = 0x37;

  } else if (z_streq(token->value, "sla")) {
    require_operand(op1, "sla");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      opcode->size = 2;
      opcode->bytes[0] = 0xcb;
      opcode->bytes[1] = 0b00100000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 2;
          opcode->bytes[0] = 0xcb;
          opcode->bytes[1] = 0x26;

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
      opcode->size = 2;
      opcode->bytes[0] = 0xcb;
      opcode->bytes[1] = 0b00101000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 2;
          opcode->bytes[0] = 0xcb;
          opcode->bytes[1] = 0x2e;

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
      opcode->size = 2;
      opcode->bytes[0] = 0xcb;
      opcode->bytes[1] = 0b00111000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 2;
          opcode->bytes[0] = 0xcb;
          opcode->bytes[1] = 0x3e;

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
      opcode->size = 1;
      opcode->bytes[0] = 0b10010000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 1;
          opcode->bytes[0] = 0x96;

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "sub [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "sub [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            opcode->size = 3;
            if (z_streq(op1->value, "ix")) {
              opcode->bytes[0] = 0xdd;
            } else if (z_streq(op1->value, "iy")){
              opcode->bytes[0] = 0xfd;
            }
            opcode->bytes[1] = 0x96;
            opcode->bytes[2] = op3->numval;

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
      opcode->size = 2;
      opcode->bytes[0] = 0xd6;
      opcode->bytes[1] = op1->numval;

    } else {
      match_fail("sub");
    }

  } else if (z_streq(token->value, "xor")) {
    require_operand(op1, "xor");

    if (z_typecmp(op1, Z_TOKTYPE_REGISTER_8)) {
      opcode->size = 1;
      opcode->bytes[0] = 0b10101000 | z_reg8_bits(op1);

    } else if (z_typecmp(op1, Z_TOKTYPE_REGISTER_16)) {
      if (op1->memref) {
        if (z_streq(op1->value, "hl")) {
          opcode->size = 1;
          opcode->bytes[0] = 0xae;

        } else if (z_streq(op1->value, "ix") || z_streq(op1->value, "iy")) {
          struct z_token_t *op2 = op1->child;
          require_operand(op2, "xor [ix/iy + d]");
          struct z_token_t *op3 = op2->child;
          require_operand(op3, "xor [ix/iy + d]");

          if (z_check_ixiy(op2, op3)) {
            opcode->size = 3;
            if (z_streq(op1->value, "ix")) {
              opcode->bytes[0] = 0xdd;
            } else if (z_streq(op1->value, "iy")) {
              opcode->bytes[0] = 0xfd;
            }
            opcode->bytes[1] = 0xae;
            opcode->bytes[2] = op3->numval;

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
      opcode->size = 2;
      opcode->bytes[0] = 0xee;
      opcode->bytes[1] = op1->numval;

    } else {
      match_fail("xor");
    }

  } else {
    fail("No match for instruction '%s'\n", token->value);
  }

  return opcode;
}
