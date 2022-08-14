#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

struct z_config_t {
  bool verbose;
  bool very_verbose;
};

struct z_config_t z_config;

#endif
