#include "opcodes.h"

#define fail(...) do {\
  z_fail(token->fname, token->line, token->col, __VA_ARGS__); \
  exit(1);\
} while (0);

#define match_fail(x) do {\
  z_fail(token->fname, token->line, token->col, "No match for the '" x "' instruction.\n");\
  exit(1);\
} while (0);

#define require_operand(op, ins) do {\
  if (!op) {\
    fail("'" ins "' instruction requires operand(s).\n");\
    exit(1);\
  }\
} while (0);



struct z_opcode_t *z_opcode_match(struct z_token_t *token) {
  struct z_opcode_t *opcode = malloc(sizeof (struct z_opcode_t));
  struct z_token_t *op1 = token->child;

  if (z_streq(token->value, "add")) {
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "add");
    opcode->size = 1;

    if (op1->type == Z_TOKTYPE_REGISTER_16) {
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
              }
            }

          } else {
            match_fail("add");
          }
        }

      } else {
        match_fail("add");

      }
    }


  } else if (z_streq(token->value, "cp")) {
    require_operand(op1, "cp");

    if (op1->type & (Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      opcode->size = 2;
      opcode->bytes[0] = 0xfe;

    } else {
      match_fail("cp");
    }

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

      switch (op1->value[0]) {
        case 'b':
          opcode->bytes[0] = 0b00001011;
          break;
        case 'd':
          opcode->bytes[0] = 0b00011011;
          break;
        case 'h':
          opcode->bytes[0] = 0b00101011;
          break;
        case 's':
          opcode->bytes[0] = 0b00111011;
          break;
        default:
          match_fail("dec");
          break;
      }

    } else {
      match_fail("dec");
    }

  } else if (z_streq(token->value, "djnz")) {
    require_operand(op1, "djnz");

    if (op1->type == Z_TOKTYPE_NUMBER) {
      opcode->size = 2;
      opcode->bytes[0] = 0x10;
      opcode->bytes[1] = op1->numval - 2;

    } else {
      match_fail("djnz");
    }


  } else if (z_streq(token->value, "ex")) {
    require_operand(op1, "ex");

    if (op1->type & Z_TOKTYPE_REGISTER_16) {
      struct z_token_t *op2 = op1->child;
      require_operand(op2, "ex");

      if (op2->type & Z_TOKTYPE_REGISTER_16) {
        if (z_streq(op1->value, "af") && z_streq(op2->value, "af")) {
          opcode->size = 1;
          opcode->bytes[0] = 0x08;
        } else {
          match_fail("ex");
        }

      } else {
        match_fail("ex");
      }

    } else {
      match_fail("ex");
    }

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
          match_fail("dec");
          break;
      }

    } else if (op1->type & Z_TOKTYPE_REGISTER_16) {
      opcode->size = 1;

      switch (op1->value[0]) {
        case 'b':
          opcode->bytes[0] = 0b00000011;
          break;
        case 'd':
          opcode->bytes[0] = 0b00010011;
          break;
        case 'h':
          opcode->bytes[0] = 0b00100011;
          break;
        case 's':
          opcode->bytes[0] = 0b00110011;
          break;
      }

    } else {
      match_fail("inc");
    }

  } else if (z_streq(token->value, "ld")) {
    require_operand(op1, "ld");
    struct z_token_t *op2 = op1->child;
    require_operand(op2, "ld");

    if (op1->type == Z_TOKTYPE_REGISTER_16) {
      if (op1->memref) {

        if (op2->type == Z_TOKTYPE_REGISTER_8) {
          opcode->size = 1;
          if (op2->memref) match_fail("ld");

          if (z_streq(op2->value, "a")) {
            if (z_streq(op1->value, "bc")) {
              opcode->bytes[0] = 0x02;

            } else if (z_streq(op1->value, "de")) {
              opcode->bytes[0] = 0x12;

            } else {
              match_fail("ld");
            }

          } else {
            match_fail("ld");
          }


        } else {
          match_fail("ld");
        }

      } else  {
        if (op2->type & (Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
          opcode->size = 3;

          if (z_streq(op1->value, "sp")) {
            opcode->bytes[0] = 0b00110001;

          } else if (z_streq(op1->value, "bc")) {
            opcode->bytes[0] = 0b00000001;

          } else if (z_streq(op1->value, "de")) {
            opcode->bytes[0] = 0b00010001;

          } else if (z_streq(op1->value, "hl")) {
            opcode->bytes[0] = 0b00100001;

          } else {
            match_fail("ld");
          }

          if (op2->type == Z_TOKTYPE_NUMBER) {
            opcode->bytes[1] = op2->numval >> 8;
            opcode->bytes[2] = op2->numval & 0xff;

          } else if (op2->type == Z_TOKTYPE_IDENTIFIER) {
            token->label_offset = 1;

          }
        }
      }


    } else if (op1->type == Z_TOKTYPE_REGISTER_8) {
      if (op2->type & (Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR)) {
        opcode->size = 2;

        switch (op1->value[0]) {
          case 'a':
            opcode->bytes[0] = 0b00111110;
            break;
          case 'b':
            opcode->bytes[0] = 0b00000110;
            break;
          case 'c':
            opcode->bytes[0] = 0b00001110;
            break;
          case 'd':
            opcode->bytes[0] = 0b00010110;
            break;
          case 'e':
            opcode->bytes[0] = 0b00011110;
            break;
          case 'h':
            opcode->bytes[0] = 0b00100110;
            break;
          case 'l':
            opcode->bytes[0] = 0b00101110;
            break;
          default:
            match_fail("ld");
            break;
        }

        if (op2->type == Z_TOKTYPE_NUMBER) {
          opcode->bytes[1] = op2->numval;

        } else if (op2->type == Z_TOKTYPE_CHAR) {
          opcode->bytes[1] = op2->value[0];
        }

      } else if (op2->type & Z_TOKTYPE_REGISTER_16) {
        if (op2->memref) {
          if (z_streq(op2->value, "hl")) {
            opcode->size = 1;

            switch (op1->value[0]) {
              case 'a':
                opcode->bytes[0] = 0b01111110;
                break;
              case 'b':
                opcode->bytes[0] = 0b01000110;
                break;
              case 'c':
                opcode->bytes[0] = 0b01001110;
                break;
              case 'd':
                opcode->bytes[0] = 0b01010110;
                break;
              case 'e':
                opcode->bytes[0] = 0b01011110;
                break;
              case 'h':
                opcode->bytes[0] = 0b01100110;
                break;
              case 'l':
                opcode->bytes[0] = 0b01101110;
                break;
              default:
                match_fail("ld");
                break;
            }

          } else if (z_streq(op2->value, "bc")) {
            if (z_streq(op1->value, "a")) {
              opcode->size = 1;
              opcode->bytes[0] = 0x0a;
            } else {
              match_fail("ld");
            }

          } else if (z_streq(op2->value, "de")) {
            if (z_streq(op1->value, "a")) {
              opcode->size = 1;
              opcode->bytes[0] = 0x1a;
            } else {
              match_fail("ld");
            }

          } else {
            match_fail("ld");
          }

        } else {
          match_fail("ld");
        }


      } else if (op2->type & (Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
        if (z_streq(op1->value, "a") && op2->memref) {
          opcode->size = 3;
          opcode->bytes[0] = 0x3a;

          if (op2->type == Z_TOKTYPE_IDENTIFIER) {
            token->label_offset = 1;

          } else if (op2->type == Z_TOKTYPE_NUMBER) {
            opcode->bytes[1] = op2->numval & 0xff;
            opcode->bytes[2] = op2->numval >> 8;
          }

        } else {
          fail("ld not implemented yet.\n");

        }

      } else {
        fail("ld not implemented yet.\n");
      }

    }

  } else if (z_streq(token->value, "jp")) {
    require_operand(op1, "jp");

    opcode->size = 3;

    if (op1->type & (Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
      opcode->bytes[0] = 0xc3;

      if (op1->type == Z_TOKTYPE_NUMBER) {
        opcode->bytes[1] = op1->numval & 0xff;
        opcode->bytes[2] = op1->numval >> 8;

      } else if (op1->type == Z_TOKTYPE_IDENTIFIER) {
        token->label_offset = 1;

      }

    } else if (op1->type & Z_TOKTYPE_CONDITION) {
      if (z_streq(op1->value, "nz")) {
        opcode->bytes[0] = 0xc2;
        struct z_token_t *op2 = op1->child;
        if (!op2) {
          fail("jp cond requires operands.\n");
        }

        if (op2->type == Z_TOKTYPE_IDENTIFIER) {
          token->label_offset = 1;

        } else if (op2->type == Z_TOKTYPE_NUMBER) {
          opcode->bytes[0] = op2->numval & 0xff;
          opcode->bytes[1] = op2->numval >> 8;


        } else {
          match_fail("jp cond");
        }


      } else {
        fail("jp condition not implemented yet.\n")
      }

    }

  } else if (z_streq(token->value, "jr")) {
    require_operand(op1, "jr");

    if (op1->type == Z_TOKTYPE_NUMBER) {
      opcode->size = 2;
      opcode->bytes[0] = 0x18;
      opcode->bytes[1] = op1->numval;

    } else if (op1->type == Z_TOKTYPE_CONDITION) {
      match_fail("jr cond");

    } else {
      match_fail("jr");
    }

  } else if (z_streq(token->value, "nop")) {
    opcode->size = 1;
    opcode->bytes[0] = 0;

  } else if (z_streq(token->value, "otir")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xb3;

  } else if (z_streq(token->value, "ret")) {
    struct z_token_t *op1 = token->child;
    if (!op1) {
      opcode->size = 1;
      opcode->bytes[0] = 0xc9;

    } else {
      match_fail("ret cond");

    }

  } else if (z_streq(token->value, "rla")) {
    opcode->size = 1;
    opcode->bytes[0] = 0x17;

  } else if (z_streq(token->value, "rlca")) {
    opcode->size = 1;
    opcode->bytes[0] = 0x07;

  } else if (z_streq(token->value, "rra")) {
    opcode->size = 1;
    opcode->bytes[0] = 0x1f;

  } else if (z_streq(token->value, "rrca")) {
    opcode->size = 1;
    opcode->bytes[0] = 0x0f;


  } else {
    fail("No match for instruction '%s'\n", token->value);
  }

  return opcode;
}
