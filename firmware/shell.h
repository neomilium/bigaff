#ifndef __SHELL_H__
#  define __SHELL_H__

#include <avr/pgmspace.h>
#include <stdbool.h>

typedef struct {
  const char *text;
  const char *description;
  void (*function)(const char*);
  bool debug;
} shell_command_t;

void    shell_loop (void);

#endif // __SHELL_H__
