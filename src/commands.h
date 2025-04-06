#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <stdbool.h>
#include <stddef.h>

#include "builtins.h"

void exec_builtin(builtin_fn fn, size_t argc, char* const* const argv,
                  bool new_proc);

void exec_executable(char* const* argv, bool new_proc);

#endif
