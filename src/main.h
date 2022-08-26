#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdbool.h>

#include "argparser.h"
#include "config.h"
#include "emitter.h"
#include "tokenizer.h"

void z_print_tokens(struct z_token_t **tokens, size_t tokcnt);

#endif
