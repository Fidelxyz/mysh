#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#include <unistd.h>

#include "types.h"

/**
 * @brief Type for builtin handling functions
 * @param [in] tokens Array of tokens
 * @return >=0 on success and -1 on error
 */
typedef RetVal (*builtin_fn)(size_t, char *const *);

typedef struct {
    const char *name;
    builtin_fn  fn;
    const bool  foreground;
} Builtin;

/**
 * @param [in] cmd string of the command
 * @return pointer of a struct Builtin storing the information of the builtin
 * function or NULL if cmd doesn't match a builtin
 */
const Builtin *check_builtin(const char *cmd);

#endif
