#ifndef EMITTER_H
#define EMITTER_H

#include <stdlib.h>

#include "structs.h"
#include "util.h"
#include "config.h"
#include "tokenizer.h"

#define Z_TAP_BLK_FLG_HDR 0x00
#define Z_TAP_BLK_FLG_DATA 0xff
#define Z_TAP_HDR_TYPE_CODE 0x03

uint8_t *z_emit(
  struct z_token_t **tokens,
  size_t tokcnt,
  size_t *emitsz,
  struct z_label_t *labels,
  struct z_def_t *defs,
  size_t bytepos);

uint8_t *z_tap_make(
  uint8_t *data, size_t datalen, const char *tapname, size_t *tapsz);

#endif
