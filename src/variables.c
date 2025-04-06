#define _POSIX_C_SOURCE 200809L

#include "variables.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "io_helpers.h"
#include "utils/minmax.h"

typedef struct {
    char *key;
    char *value;
} Variable;

static const size_t INIT_VARS_CAPACITY = 16;

static Variable *vars          = NULL;
static size_t    vars_len      = 0;
static size_t    vars_capacity = 0;

void init_variables() {
    vars          = malloc(INIT_VARS_CAPACITY * sizeof(Variable));
    vars_capacity = INIT_VARS_CAPACITY;
}

void free_variables() {
    for (size_t i = 0; i < vars_len; i++) {
        free(vars[i].key);
        free(vars[i].value);
    }
    free(vars);
}

static void add_variable(const char *key, const char *value) {
    assert(vars_len <= vars_capacity);

    if (vars_len == vars_capacity) {
        vars_capacity *= 2;
        vars           = realloc(vars, vars_capacity * sizeof(Variable));
    }

    vars[vars_len].key   = strdup(key);
    vars[vars_len].value = strdup(value);
    vars_len++;
}

static Variable *find_variable(const char *const key) {
    for (size_t i = 0; i < vars_len; i++) {
        if (strcmp(key, vars[i].key) == 0) {
            return &vars[i];
        }
    }
    return NULL;
}

void set_variable(const char *const key, const char *const value) {
    Variable *var = find_variable(key);
    if (var == NULL) {
        add_variable(key, value);
    } else {
        free(var->value);
        var->value = strdup(value);
    }
}

/*
 * @note Prereq: len(buf) >= MAX_STR_LEN + 1
 */
void expand_variables(char *const buf, char **tokens,
                      const size_t token_count) {
    // Make a copy of tokens
    char **src_tokens = malloc(token_count * sizeof(char *));
    for (size_t i = 0; i < token_count; i++) {
        src_tokens[i] = strdup(tokens[i]);
    }

    size_t buf_len = 0;

    for (size_t i = 0; i < token_count; i++) {
        tokens[i] = buf + buf_len;

        const char *const token             = src_tokens[i];
        const char       *token_begin       = token;
        bool              is_token_variable = false;

        // Process buffer in batch of variables and non-variables
        for (const char *ch = token;; ch++) {
            if (*ch != '$' && *ch != ' ' && *ch != '\n' && *ch != '\0')
                continue;

            // process when meet end of token

            size_t token_len = ch - token_begin;

            if (is_token_variable && token_len > 1) {
                // is variable and not a single '$'

                // skip '$'
                token_begin++;
                token_len--;

                // copy variable key
                char *key = malloc(token_len + 1);
                memcpy(key, token_begin, token_len);
                key[token_len] = '\0';

                // load variable value
                Variable *var = find_variable(key);
                if (var != NULL) {  // found variable
                    const char *value = var->value;

                    // concatenate variable value to result
                    size_t res_len =
                        min(strlen(value), (size_t)(MAX_STR_LEN - buf_len));
                    memcpy(buf + buf_len, value, res_len);
                    buf_len += res_len;
                }
                free(key);

            } else {
                // is not variable
                size_t res_len =
                    min(token_len, (size_t)(MAX_STR_LEN - buf_len));
                memcpy(buf + buf_len, token_begin, res_len);
                buf_len += res_len;
            }

            // End of input
            if (*ch == '\0') break;

            is_token_variable = *ch == '$';
            token_begin       = ch;

            assert(buf_len <= MAX_STR_LEN);
        }
        buf[buf_len++] = '\0';
    }

    for (size_t i = 0; i < token_count; i++) free(src_tokens[i]);
    free(src_tokens);

    DEBUG_PRINT("DEBUG: Expanded tokens: [");
    for (size_t i = 0; i < token_count; i++) {
        DEBUG_PRINT("%s", tokens[i]);
        if (i < token_count - 1) {
            DEBUG_PRINT(", ");
        }
    }
    DEBUG_PRINT("] (len: %zu)\n", buf_len);
}

bool exec_assignment(const char *const token) {
    const char *eq = strchr(token, '=');
    if (eq == NULL) return false;

    const size_t key_len = eq - token;
    if (key_len == 0) return false;
    const size_t value_len = strlen(eq + 1);

    char *key   = malloc(key_len + 1);
    char *value = malloc(value_len + 1);

    memcpy(key, token, key_len);
    key[key_len] = '\0';

    memcpy(value, eq + 1, value_len);
    value[value_len] = '\0';

    DEBUG_PRINT("DEBUG: Executing assignment: %s\n", token);
    set_variable(key, value);

    free(key);
    free(value);

    return true;
}
