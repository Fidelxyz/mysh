#define _DEFAULT_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "background.h"
#include "builtins.h"
#include "commands.h"
#include "io_helpers.h"
#include "variables.h"

static pid_t executing_pgid = -1;

void sigint_executing_processes() {
    if (executing_pgid == -1) return;

    if (killpg(executing_pgid, SIGINT) == -1) {
        display_error("ERROR: Failed to send SIGINT to executing processes\n");
    }
}

void init() {
    setpgid(0, 0);

    // Ignore SIGINT
    struct sigaction sa = {
        .sa_handler = SIG_IGN,
        .sa_flags   = 0,
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    // Set stdout & stderr to line-buffered
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    init_variables();
    init_background();
}

void cleanup() {
    free_variables();
    free_background();
}

void exec(const size_t argc, char *const *const argv, const bool background) {
    assert(argv[argc] == NULL);  // argv should be NULL-terminated

    // Check for assignment
    if (argc == 1) {
        if (exec_assignment(argv[0])) {
            return;
        }
    }

    // Check for builtins
    if (argc >= 1) {
        const Builtin *builtin = check_builtin(argv[0]);
        if (builtin != NULL) {
            // execute builtin in new process if both:
            // 1. it is not a builtin which should run in foreground
            // 2. we are not already running in background
            const bool new_proc = !builtin->foreground && !background;

            exec_builtin(builtin->fn, argc, argv, new_proc);
            return;
        }
    }

    // Check for executable
    exec_executable(argv, !background);
}

/**
 * @return 0 on continue, -1 on exit
 */
int execute_command(char *const cmd, const bool background) {
    // Tokenize
    char  *tokens[MAX_STR_LEN];
    size_t n_token = tokenize_input(cmd, tokens);

    // Expand variables
    expand_variables(cmd, tokens, n_token);

    char *const *tokens_view = tokens;

    // Drop leading empty tokens
    size_t n_drop = count_leading_empty_tokens(tokens_view);
    assert(n_drop <= n_token);
    tokens_view += n_drop;
    n_token     -= n_drop;

    // Skip empty line
    if (n_token == 0) return 0;

    // Exit
    if (strncmp("exit", tokens[0], 5) == 0) return -1;

    exec(n_token, tokens_view, background);

    return 0;
}

int main() {
    DEBUG_PRINT("DEBUG: Main process pid = %d\n", getpid());

    init();

    while (true) {
        display_message(PROMPT);
        fflush(stdout);

        // ========== Input ==========

        char          input_buf[MAX_STR_LEN + 1];
        const ssize_t read_len = get_input(input_buf);
        if (read_len == -1) continue;

        // Exit by EOF <C-d>
        if (read_len == 0) break;

        // ========== Background ==========

        check_background_status(false);

        const int bg = parse_background(input_buf);
        if (bg == -1) continue;  // error

        DEBUG_PRINT("DEBUG: Background: %d\n", bg);

        char job_cmd[MAX_STR_LEN + 1];
        if (bg) {
            strncpy(job_cmd, input_buf, MAX_STR_LEN + 1);
        } else {
            job_cmd[0] = '\0';
        }

        // ========== Parse Pipe ==========

        char  *cmds[MAX_STR_LEN];
        size_t n_command = parse_pipe(input_buf, cmds);

        // validate parsed pipe
        if (n_command > 1) {
            for (size_t i = 0; i < n_command; i++) {
                char *const cmd = cmds[i];

                // if exists an empty command
                if (cmd[strspn(cmd, DELIMITERS)] == '\0') {
                    display_error(
                        "ERROR: Syntax error near unexpected token `|'\n");
                    n_command = 0;  // <== invalid command flag set here
                    break;
                }
            }
            // invalid command
            if (n_command == 0) continue;
        }

        // Allowcate new string for each command
        for (size_t i = 0; i < n_command; i++) {
            char *cmd = cmds[i];
            cmds[i]   = malloc(MAX_STR_LEN + 1);
            strcpy(cmds[i], cmd);
        }

        DEBUG_PRINT("DEBUG: Command count: %zu\n", n_command);
        for (size_t i = 0; i < n_command; i++) {
            DEBUG_PRINT("DEBUG: Command %zu: %s\n", i, cmds[i]);
        }

        // ========== Execute ==========

        bool exit = false;

        if (n_command == 1) {
            // single command
            if (bg) {
                // run in background
                pid_t pid = fork();
                if (pid == -1) {
                    display_error("ERROR: Fork failed\n");
                    continue;
                }

                if (pid) {
                    // parent process
                    DEBUG_PRINT("DEBUG: New process started, pid: %d\n", pid);

                    add_background_job(&pid, 1, job_cmd);

                } else {
                    // child process
                    close(STDIN_FILENO);
                    execute_command(cmds[0], true);
                    exit = true;
                }

            } else {
                // run in foreground
                if (execute_command(cmds[0], false) == -1) {
                    exit = true;
                }
            }

        } else {
            // pipe
            int pipe_fd_in[2]  = {-1, -1};
            int pipe_fd_out[2] = {-1, -1};
            pipe_fd_in[0]      = dup(STDIN_FILENO);

            int stored_stdin  = dup(STDIN_FILENO);
            int stored_stdout = dup(STDOUT_FILENO);

            pid_t *pids = malloc(n_command * sizeof(*pids));
            pid_t  pid  = 0;

            for (size_t i = 0; i < n_command; i++) {
                DEBUG_PRINT("DEBUG: Executing command %zu\n", i);

                // only pipe_fd_in[0] should be open
                assert(fcntl(pipe_fd_in[0], F_GETFD) != -1 || errno != EBADF);
                assert(fcntl(pipe_fd_in[1], F_GETFD) == -1 && errno == EBADF);
                assert(fcntl(pipe_fd_out[0], F_GETFD) == -1 && errno == EBADF);
                assert(fcntl(pipe_fd_out[1], F_GETFD) == -1 && errno == EBADF);

                if (i != n_command - 1) {
                    // if not last command, create new pipe
                    if (pipe(pipe_fd_out) == -1) {
                        display_error("ERROR: Pipe failed\n");
                        break;
                    }
                    DEBUG_PRINT("DEBUG: Pipe created: %d %d\n", pipe_fd_out[0],
                                pipe_fd_out[1]);
                } else {
                    // if last command, restore stdout
                    pipe_fd_out[1] = stored_stdout;
                    stored_stdout  = -1;
                }

                // fork
                pid = fork();
                if (pid == -1) {
                    display_error("ERROR: Fork failed\n");
                    break;
                }

                pids[i] = pid;

                // execute command
                if (pid) {
                    // parent process
                    DEBUG_PRINT("DEBUG: New process started, pid: %d\n", pid);

                    close(pipe_fd_in[0]);
                    close(pipe_fd_out[1]);

                } else {
                    // execution process

                    // set pgid to pid of the first command
                    // If this is the first command, pids[0] == 0;
                    // otherwise, pids[0] == pid of the first command.
                    setpgid(0, pids[0]);

                    // sub-process does not need to read from the output
                    // pipe
                    close(pipe_fd_out[0]);

                    // redirect input
                    if (bg && i == 0) {
                        // close stdin on the first command in background
                        close(STDIN_FILENO);
                    } else {
                        dup2(pipe_fd_in[0], STDIN_FILENO);
                    }
                    close(pipe_fd_in[0]);

                    // redirect output
                    dup2(pipe_fd_out[1], STDOUT_FILENO);
                    close(pipe_fd_out[1]);

                    if (i != n_command - 1) {
                        // Set stdout to full-buffered for piped commands
                        setvbuf(stdout, NULL, _IOFBF, 0);
                    } else {
                        // Reset stdout to line-buffered for the last command
                        setvbuf(stdout, NULL, _IOLBF, 0);
                    }

                    execute_command(cmds[i], true);

                    fflush(stdout);

                    exit = true;
                }

                pipe_fd_in[0]  = pipe_fd_out[0];
                pipe_fd_out[0] = -1;

                if (exit) break;
            }  // for commands

            if (pid) {  // in main process
                if (bg) {
                    // use pid of the last command
                    add_background_job(pids, n_command, job_cmd);

                } else {
                    // handle SIGINT
                    executing_pgid = pids[0];
                    struct sigaction old_sa;
                    struct sigaction sa = {
                        .sa_handler = sigint_executing_processes,
                        .sa_flags   = 0,
                    };
                    sigemptyset(&sa.sa_mask);
                    sigaction(SIGINT, &sa, &old_sa);

                    // wait for all sub-process
                    while (wait(NULL) > 0);

                    // restore SIGINT handler
                    executing_pgid = -1;
                    sigaction(SIGINT, &old_sa, NULL);
                }
            }

            free(pids);

            // all pipes should be closed
            assert(fcntl(pipe_fd_in[0], F_GETFD) == -1 && errno == EBADF);
            assert(fcntl(pipe_fd_in[1], F_GETFD) == -1 && errno == EBADF);
            assert(fcntl(pipe_fd_out[0], F_GETFD) == -1 && errno == EBADF);
            assert(fcntl(pipe_fd_out[1], F_GETFD) == -1 && errno == EBADF);

            // restore stdin
            dup2(stored_stdin, STDIN_FILENO);
            close(stored_stdin);

            if (pid) {
                assert(fcntl(stored_stdin, F_GETFD) == -1 && errno == EBADF);
                assert(fcntl(stored_stdout, F_GETFD) == -1 && errno == EBADF);
            }

        }  // if n_command == 1

        for (size_t i = 0; i < n_command; i++) free(cmds[i]);

        if (exit) break;
    }

    cleanup();

    return 0;
}
