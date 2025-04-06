#define _POSIX_C_SOURCE 200809L

#include "io_helpers.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void handle_sigint() {}

ssize_t get_input(char *in_ptr) {
    // set signal handler
    struct sigaction old_sa;
    struct sigaction sa = {
        .sa_handler = handle_sigint,
        .sa_flags   = 0,
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, &old_sa);

    // Not a sanitizer issue since in_ptr is allocated as MAX_STR_LEN+1
    const ssize_t read_len = read(STDIN_FILENO, in_ptr, MAX_STR_LEN + 1);
    if (read_len == -1 && errno == EINTR) {
        putchar('\n');
    }

    // restore signal handler
    sigaction(SIGINT, &old_sa, NULL);

    // read error
    if (read_len == -1) {
        in_ptr[0] = '\0';
        return -1;
    }

    if (read_len > MAX_STR_LEN) {  // input too long
        display_error("ERROR: input line too long\n");

        int junk;
        do {
            junk = getchar();
        } while (junk != EOF && junk != '\n');

        in_ptr[0] = '\0';
        return -1;
    }

    in_ptr[read_len] = '\0';
    return read_len;
}

int parse_background(char *const str) {
    char str_cpy[MAX_STR_LEN];
    strcpy(str_cpy, str);

    size_t n_token = 0;
    char  *tokens[MAX_STR_LEN];

    char *token = strtok(str_cpy, DELIMITERS);
    while (token != NULL) {
        tokens[n_token++] = token;
        token             = strtok(NULL, DELIMITERS);
    }

    for (size_t i = 0; i < n_token; i++) {
        // if exists a token '&'
        if (tokens[i][0] == BACKGROUND_SYMBOL && tokens[i][1] == '\0') {
            // if '&' is not the last token
            if (i != n_token - 1) {
                display_error(
                    "ERROR: Syntax error near unexpected token after `&'\n");
                return -1;
            }

            // remove '&' from the command
            str[tokens[i] - str_cpy] = '\0';

            return 1;
        }
    }

    return 0;
}

size_t parse_pipe(char *const str, char **const cmds) {
    size_t n_cmd = 0;
    char  *cmd   = str;
    while (cmd != NULL) {
        cmds[n_cmd++] = cmd;

        cmd = strchr(cmd, PIPE_SYMBOL);
        if (cmd == NULL) break;
        *cmd++ = '\0';
    }
    cmds[n_cmd] = NULL;
    return n_cmd;
}

size_t tokenize_input(char *const str, char **const tokens) {
    size_t n_token = 0;
    char  *token   = strtok(str, DELIMITERS);
    while (token != NULL) {
        tokens[n_token++] = token;
        token             = strtok(NULL, DELIMITERS);
    }
    tokens[n_token] = NULL;
    return n_token;
}

size_t count_leading_empty_tokens(char *const *const tokens) {
    size_t count = 0;
    for (char *const *token = tokens; *token != NULL; token++) {
        if ((*token)[0] != '\0') break;  // reach the first non-empty token
        count++;
    }
    return count;
}
