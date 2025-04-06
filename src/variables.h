#ifndef __VARIABLES_H__
#define __VARIABLES_H__

#include <stdbool.h>
#include <stddef.h>

void init_variables();

void free_variables();

void expand_variables(char *buf, char **tokens, size_t token_count);

/*
 * @brief Check if the token is an assignment.
 * If it is, split the token into key and value.
 *
 * @return true if the token is an assignment, false otherwise.
 *
 * @note Prereq: size of key >= size of token,
 *               size of rvalue >= size of token.
 */
bool exec_assignment(const char *token);

#endif
