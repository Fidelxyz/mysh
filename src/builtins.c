#include "builtins.h"

#include <string.h>

#include "builtins/cd.h"

static const Builtin BUILTINS[] = {
    {"cd", bn_cd, true},  // foreground
};
static const size_t BUILTINS_COUNT = sizeof(BUILTINS) / sizeof(Builtin);

const Builtin* check_builtin(const char* const cmd) {
    for (size_t i = 0; i < BUILTINS_COUNT; i++) {
        if (strcmp(cmd, BUILTINS[i].name) == 0) {
            return &BUILTINS[i];
        }
    }
    return NULL;
}
