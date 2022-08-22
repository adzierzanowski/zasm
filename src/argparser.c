#include "argparser.h"


int argparser_index_of(struct argparser_t *parser, const char *optname)
{
  for (int i = 0; i < parser->count; i++)
  {
    if (strcmp(optname, parser->options[i]->short_name) == 0
      || strcmp(optname, parser->options[i]->long_name) == 0)
      return i;
  }

  return -1;
}

struct argparser_t *argparser_new(const char *prog_name)
{
  struct argparser_t *parser = malloc(sizeof(struct argparser_t));
  parser->prog_name = prog_name;
  parser->count = 0;
  parser->options = NULL;
  parser->positional_count = 0;
  parser->positional = NULL;
  return parser;
}

void argparser_add(
  struct argparser_t *parser,
  const char *short_name, 
  const char *long_name, 
  const char *help, 
  bool required, 
  bool takes_arg
) {
  int i = parser->count;
  parser->count++;
  parser->options = realloc(parser->options,
    sizeof(struct option_t *) * parser->count);

  struct option_t *opt = malloc(sizeof(struct option_t));

  opt->short_name = short_name;
  opt->long_name = long_name;
  opt->help = help;
  opt->passed = false;
  opt->required = required;
  opt->takes_arg = takes_arg;

  parser->options[i] = opt;
}

void argparser_from_struct(
  struct argparser_t *parser, struct option_init_t *init)
{
  argparser_add(
    parser,
    init->short_name,
    init->long_name,
    init->help,
    init->required,
    init->takes_arg);
}

static void argparser_take_arg(
    struct option_t *opt,
    int *i, 
    int argc,
    char *argv[])
{
  int idx = *i;
  if (idx+1 >= argc || argv[idx+1][0] == '-')
  {
    fprintf(stderr, "Option %s/%s requires an argument.\n",
      opt->short_name, opt->long_name);
    exit(1);
  }
  opt->value = argv[idx+1];
  (*i)++;
}

int argparser_parse(struct argparser_t *parser, int argc, char *argv[])
{
  int passed_count = 0;

  if (parser->prog_name == NULL)
    parser->prog_name = argv[0];

  // Double dash == accept only positional arguments from now on 
  bool double_dash = false;

  // Positional arguments
  int pos_cnt = 0;
  int *pos_idxs = NULL;

  for (int i = 0; i < argc; i++)
  {
    if (double_dash)
      goto L_parse_positional;

    const char *arg = (const char *) argv[i];

    if (strcmp("--", arg) == 0)
    {
      double_dash = true;
      continue;
    }

    int argi = argparser_index_of(parser, arg);

    if (argi > -1)
    {
      struct option_t *opt = parser->options[argi];
      if (! opt->passed)
      {
        opt->passed = true;
        passed_count++;
      }
      if (opt->takes_arg)
        argparser_take_arg(opt, &i, argc, argv);
    }
    else if (strlen(arg) > 2 && arg[0] == '-' && arg[1] != '-')
    {
      for (int k = 1; k < (int) strlen(arg); k++)
      {
        char c_arg[3];
        sprintf(c_arg, "-%c", arg[k]);
        argi = argparser_index_of(parser, (const char *) c_arg);

        if (argi > -1)
        {
          struct option_t *opt = parser->options[argi];
          if (! opt->passed)
          {
            opt->passed = true;
            passed_count++;
          }
          if (opt->takes_arg)
          {
            if (k == (int) strlen(arg) - 1)
              argparser_take_arg(opt, &i, argc, argv);
            else
            {
              fprintf(stderr, "Option %s/%s requires an argument.\n",
                opt->short_name, opt->long_name);
              exit(1);
            }
          }
        }
      }
    }
    else
    {
      L_parse_positional:
        pos_idxs = realloc(pos_idxs, sizeof(int) * ++pos_cnt);
        pos_idxs[pos_cnt - 1] = i;
    }
  }

  parser->positional_count = pos_cnt;
  parser->positional = realloc(parser->positional,
    sizeof(const char *) * pos_cnt);

  for (int i = 0; i < pos_cnt; i++)
    parser->positional[i] = argv[pos_idxs[i]];

  if (pos_idxs != NULL)
    free(pos_idxs);
  
  return passed_count;
}

void argparser_validate(struct argparser_t *parser)
{
  for (int i = 0; i < parser->count; i++)
  {
    struct option_t *opt = parser->options[i];

    if (opt->required && ! opt->passed)
    {
      fprintf(stderr,
        "Option %s/%s is required.\n",
        opt->short_name, opt->long_name);
      exit(1);
    }
  }
}

bool argparser_passed(struct argparser_t *parser, const char *optname)
{
  int i = argparser_index_of(parser, optname);
  if (i < 0)
    return false;
  
  return parser->options[i]->passed;
}

const char *argparser_get(struct argparser_t *parser, const char *optname)
{
  int argi = argparser_index_of(parser, optname);
  if (argi < 0)
    return NULL;

  return parser->options[argi]->value;
}

void argparser_free(struct argparser_t *parser) 
{
  if (parser != NULL)
  {
    for (int i = 0; i < parser->count; i++)
      free(parser->options[i]);

    if (parser->options != NULL)
      free(parser->options);
    
    if (parser->positional != NULL)
      free(parser->positional);

    free(parser);
  }
}

void argparser_usage(struct argparser_t *parser)
{
  printf("usage: %s ", parser->prog_name);
  for (int i = 0; i < parser->count; i++)
  {
    struct option_t *opt = parser->options[i];

    char metavar[0xff] = {'\0'}; 
    if (opt->takes_arg)
    {
      metavar[0] = ' ';
      const char *name = opt->long_name;
      int j = 1;
      for (int i = 0; i < (int) strlen(name); i++)
      {
        if (isalnum(name[i]))
          metavar[j++] = toupper(name[i]);
      }
    }

    printf("%s%s%s%s ",
      opt->required? "" : "[",
      opt->short_name,
      metavar,
      opt->required? "" : "]");
  }
  printf("\n\n");

  for (int i = 0; i < parser->count; i++)
  {
    struct option_t *opt = parser->options[i];
    printf("%4s, %-20s %-40s\n",
        opt->short_name, opt->long_name, opt->help);
  }

  printf("\n");
  exit(0);
}

void argparser_dump(struct argparser_t *parser)
{
  printf("Parser:\n");
  printf("  prog_name: %s\n", parser->prog_name);
  printf("  count: %d\n", parser->count);
  printf("  positional_count: %d\n\n", parser->positional_count);

  for (int i = 0; i < parser->count; i++)
  {
    struct option_t *opt = parser->options[i];
    argparser_option_dump(opt);
  }

  printf("Positional: ");
  for (int i = 0; i < parser->positional_count; i++)
  {
    printf("%s%s ",
      parser->positional[i],
      i == parser->positional_count - 1 ? "" : ",");
  }
  printf("\n\n");
}

void argparser_option_dump(struct option_t *opt)
{
  printf("%s, %s\n", opt->short_name, opt->long_name);
  printf("%10s: %s\n", "Help", opt->help);
  printf("%10s: %s\n", "Required", opt->required ? "true" : "false");
  printf("%10s: %s\n", "Takes arg", opt->takes_arg ? "true" : "false");
  printf("%10s: %s\n", "Passed", opt->passed ? "true" : "false");
  if (opt->takes_arg)
    printf("%10s: %s\n",
      "Value", opt->value == NULL ? "(NULL)" : opt->value);
  printf("\n");
}
