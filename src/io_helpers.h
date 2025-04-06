#ifndef __IO_HELPERS_H__
#define __IO_HELPERS_H__

#include <stdio.h>
#include <unistd.h>

#define PROMPT "mysh$ "

#define MAX_STR_LEN 128

// Assumption: all input tokens are whitespace delimited
#define DELIMITERS        " \t\n"
#define PIPE_SYMBOL       '|'
#define BACKGROUND_SYMBOL '&'

#define VARIABLE_EXPANSION_SYMBOL "$"

// ========== OUTPUT MARCOS ==========

#define COLOR_RED  "\033[1;31m"
#define COLOR_NONE "\033[0m"

#ifndef NDEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, COLOR_RED fmt COLOR_NONE, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

#define display_message(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#define display_error(fmt, ...)   fprintf(stderr, fmt, ##__VA_ARGS__)

/**
 * @note Prereq: in_ptr points to a character buffer of size > MAX_STR_LEN
 * @return number of bytes read
 */
ssize_t get_input(char *in_ptr);

/**
 * @brief Check whether str is a valid background command, and return the parsed
 * command without background symbol.
 *
 * @param str [in, out] The string to parse.
 * @return 1 if str is a background command,
 *         0 if str is not a background command,
 *         -1 on error.
 *
 * @warning str is modified.
 */
int parse_background(char *str);

/**
 * @brief Parse str into commands separated by pipe.
 *
 * @param str [in, out] The string to parse.
 * @param cmds [out] An array receiving pointers to the commands terminated by
 * NULL.
 * @return the number of commands.
 *
 * @warning str is modified.
 * @warning cmds points to the memory in str, so str should live as long as
 * cmds.
 */
size_t parse_pipe(char *str, char **cmds);

/**
 * @brief Tokenize str into tokens.
 *
 * @param str [in, out] The string to tokenize.
 * @param tokens_view [out] An array receving pointers to the tokens terminated
 * by NULL.
 * @return the number of tokens.
 *
 * @warning str is modified.
 * @warning tokens points to the memory in str, so str should live as long as
 * tokens.
 */
size_t tokenize_input(char *str, char **tokens);

/**
 * @brief Return the number of leading empty tokens.
 * @param token_count [in] to the number of tokens
 * @return the number of leading empty tokens.
 */
size_t count_leading_empty_tokens(char *const *tokens);

#endif
