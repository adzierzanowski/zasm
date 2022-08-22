#ifndef ARGPARSER_H
#define ARGPARSER_H

#ifdef __cplusplus
  extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

struct option_t
{
  const char *short_name;
  const char *long_name;
  const char *help;
  bool required;
  bool takes_arg;

  bool passed;
  const char *value;
};

struct option_init_t
{
  const char *short_name;
  const char *long_name;
  const char *help;
  bool required;
  bool takes_arg;
};


struct argparser_t
{
  int count;
  const char *prog_name;
  struct option_t **options;
  int positional_count;
  char **positional;
};

struct argparser_t *argparser_new(const char *prog_name);
int argparser_index_of(struct argparser_t *parser, const char *optname);
void argparser_add(
  struct argparser_t *parser,
  const char *short_name, 
  const char *long_name, 
  const char *help, 
  bool required, 
  bool takes_arg
);
void argparser_from_struct(
  struct argparser_t *parser, struct option_init_t *init);
int argparser_parse(struct argparser_t *parser, int argc, char *argv[]);
bool argparser_passed(struct argparser_t *parser, const char *optname);
const char *argparser_get(struct  argparser_t *parser, const char *optname);
void argparser_free(struct argparser_t *parser);
void argparser_usage(struct argparser_t *parser);
void argparser_validate(struct argparser_t *parser);
void argparser_dump(struct argparser_t *parser);
void argparser_option_dump(struct option_t *opt);

#ifdef __cplusplus
}
#endif

#endif
