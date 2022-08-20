#ifndef EMITTER_H
#define EMITTER_H

#include <stdlib.h>

#include "structs.h"
#include "util.h"
#include "config.h"
#include "tokenizer.h"

uint8_t *z_emit(
  struct z_token_t **tokens,
  size_t tokcnt,
  size_t *emitsz,
  struct z_label_t *labels,
  size_t bytepos);

#endif
