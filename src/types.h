#ifndef __BUILTINS_RETVAL_H__
#define __BUILTINS_RETVAL_H__

#include <stdbool.h>
#include <stddef.h>

// ============
// RETURN VALUE
// ============

typedef int RetVal;

#define RETVAL_SUCCESS 0
#define RETVAL_FAILURE -1

#define SUCCEEDED(retval) ((retval) >= 0)
#define FAILED(retval)    ((retval) < 0)

#endif
