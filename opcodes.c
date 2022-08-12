#include "opcodes.h"

#define fail(...) do {\
  z_fail(token->fname, token->line, token->col, __VA_ARGS__); \
  exit(1);\
} while (0);

#define match_fail(x) do {\
  z_fail(token->fname, token->line, token->col, "No match for the '" x "' instruction.\n");\
  exit(1);\
} while (0);

struct z_opcode_t *z_opcode_match(struct z_token_t *token) {
  struct z_opcode_t *opcode = malloc(sizeof (struct z_opcode_t));

  if (z_streq(token->value, "cp")) {
    struct z_token_t *operand1 = token->child;
    if (!operand1) {
      //z_fail(token->fname, token->line, token->col, "cp instruction requires an operand.\n");
      fail("cp instruction requires an operand\n");
      exit(1);
    }


    if (operand1->type & (Z_TOKTYPE_CHAR | Z_TOKTYPE_NUMBER)) {
      opcode->size = 2;
      opcode->bytes[0] = 0xfe;

    } else {
      match_fail("cp");
    }

  } else if (z_streq(token->value, "ex")) {
    struct z_token_t *operand1 = token->child;

    if (!operand1) {
      fail("ex instruction requires operands.\n");
    }

    if (operand1->type & Z_TOKTYPE_REGISTER_16) {
      struct z_token_t *operand2 = operand1->child;

      if (!operand2) {
        fail("ex instruction requires operands.\n");
      }

      if (operand2->type & Z_TOKTYPE_REGISTER_16) {
        if (z_streq(operand1->value, "af") && z_streq(operand2->value, "af")) {
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
    struct z_token_t *operand1 = token->child;

    if (!operand1) {
      fail("inc instruction requires an operand.\n");
    }

    if (operand1->type & Z_TOKTYPE_REGISTER_16) {
      opcode->size = 1;

      switch (operand1->value[0]) {
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
    struct z_token_t *operand1 = token->child;

    if (!operand1) {
      fail("ld instruction requires two operands.\n");
    }

    struct z_token_t *operand2 = operand1->child;

    if (operand1->type == Z_TOKTYPE_REGISTER_16) {
      if (operand2->type & (Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
        opcode->size = 3;

        if (z_streq(operand1->value, "sp")) {
          opcode->bytes[0] = 0b00110001;

        } else if (z_streq(operand1->value, "bc")) {
          opcode->bytes[0] = 0b00000001;

        } else if (z_streq(operand1->value, "de")) {
          opcode->bytes[0] = 0b00010001;

        } else if (z_streq(operand1->value, "hl")) {
          opcode->bytes[0] = 0b00100001;

        } else {
          match_fail("ld");
        }

        if (operand2->type == Z_TOKTYPE_NUMBER) {
          opcode->bytes[1] = operand2->numval >> 8;
          opcode->bytes[2] = operand2->numval & 0xff;

        } else if (operand2->type == Z_TOKTYPE_IDENTIFIER) {
          token->label_offset = 1;

        }
      }

    } else if (operand1->type == Z_TOKTYPE_REGISTER_8) {
      if (operand2->type & (Z_TOKTYPE_NUMBER | Z_TOKTYPE_CHAR)) {
        opcode->size = 2;

        switch (operand1->value[0]) {
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

        if (operand2->type == Z_TOKTYPE_NUMBER) {
          opcode->bytes[1] = operand2->numval;

        } else if (operand2->type == Z_TOKTYPE_CHAR) {
          opcode->bytes[1] = operand2->value[0];
        }

      } else if (operand2->type & Z_TOKTYPE_REGISTER_16) {
        if (operand2->memref) {
          if (z_streq(operand2->value, "hl")) {
            opcode->size = 1;

            switch (operand1->value[0]) {
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
            }

          } else {
            fail("ld not implemented yet.\n");
          }

        } else {
          fail("ld not implemented yet.\n");
        }


      } else if (operand2->type & (Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
        if (z_streq(operand1->value, "a") && operand2->memref) {
          opcode->size = 3;
          opcode->bytes[0] = 0x3a;

          if (operand2->type == Z_TOKTYPE_IDENTIFIER) {
            token->label_offset = 1;

          } else if (operand2->type == Z_TOKTYPE_NUMBER) {
            opcode->bytes[1] = operand2->numval & 0xff;
            opcode->bytes[2] = operand2->numval >> 8;
          }

        } else {
          fail("ld not implemented yet.\n");

        }

      } else {
        fail("ld not implemented yet.\n");
      }

    }

  } else if (z_streq(token->value, "jp")) {
    struct z_token_t *operand1 = token->child;
    opcode->size = 3;

    if (!operand1) {
      fail("jp instruction requires operands.\n");
    }

    if (operand1->type & (Z_TOKTYPE_IDENTIFIER | Z_TOKTYPE_NUMBER)) {
      opcode->bytes[0] = 0xc3;

      if (operand1->type == Z_TOKTYPE_NUMBER) {
        opcode->bytes[1] = operand1->numval & 0xff;
        opcode->bytes[2] = operand1->numval >> 8;

      } else if (operand1->type == Z_TOKTYPE_IDENTIFIER) {
        token->label_offset = 1;

      }

    } else if (operand1->type & Z_TOKTYPE_CONDITION) {
      if (z_streq(operand1->value, "nz")) {
        opcode->bytes[0] = 0xc2;
        struct z_token_t *operand2 = operand1->child;
        if (!operand2) {
          fail("jp cond requires operands.\n");
        }

        if (operand2->type == Z_TOKTYPE_IDENTIFIER) {
          token->label_offset = 1;

        } else if (operand2->type == Z_TOKTYPE_NUMBER) {
          opcode->bytes[0] = operand2->numval & 0xff;
          opcode->bytes[1] = operand2->numval >> 8;


        } else {
          match_fail("jp cond");
        }


      } else {
        fail("jp condition not implemented yet.\n")
      }

    }

  } else if (z_streq(token->value, "otir")) {
    opcode->size = 2;
    opcode->bytes[0] = 0xed;
    opcode->bytes[1] = 0xb3;

  } else if (z_streq(token->value, "ret")) {
    struct z_token_t *operand1 = token->child;
    if (!operand1) {
      opcode->size = 1;
      opcode->bytes[0] = 0xc9;

    } else {
      match_fail("ret cond");

    }


  } else {
    fail("No match for instruction '%s'\n", token->value);
  }

  return opcode;
}
