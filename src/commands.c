#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "commands.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "io_helpers.h"

static pid_t exec_pid = 0;

static void send_sigint() {
    if (exec_pid > 0) {
        kill(exec_pid, SIGINT);
    }
}

void exec_builtin(const builtin_fn fn, const size_t argc,
                  char* const* const argv, const bool new_proc) {
    DEBUG_PRINT("DEBUG: Executing builtin: %s\n", argv[0]);

    if (!new_proc) {
        // set process name
        char old_name[16];
        pthread_getname_np(pthread_self(), old_name, sizeof(old_name));
        pthread_setname_np(pthread_self(), argv[0]);

        const RetVal retval = fn(argc, argv);
        if (FAILED(retval)) {
            display_error("ERROR: Builtin failed: %s\n", argv[0]);
        }

        // restore process name
        pthread_setname_np(pthread_self(), old_name);

    } else {
        exec_pid = fork();
        if (exec_pid == -1) {
            display_error("ERROR: Fork failed\n");
            return;
        }

        if (exec_pid) {
            // parent process

            // send SIGINT to child process
            struct sigaction old_sa;
            struct sigaction sa = {
                .sa_handler = send_sigint,
                .sa_flags   = 0,
            };
            sigemptyset(&sa.sa_mask);
            sigaction(SIGINT, &sa, &old_sa);

            // wait for execution
            int status;
            do {
                waitpid(exec_pid, &status, 0);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            // restore SIGINT handler
            sigaction(SIGINT, &old_sa, NULL);

        } else {
            // execution process

            // set process name
            pthread_setname_np(pthread_self(), argv[0]);

            // restore child process' SIGINT handler to default, so that the
            // child process can receive SIGINT from the main process
            struct sigaction sa = {
                .sa_handler = SIG_DFL,
                .sa_flags   = 0,
            };
            sigemptyset(&sa.sa_mask);
            sigaction(SIGINT, &sa, NULL);

            const RetVal retval = fn(argc, argv);
            if (FAILED(retval)) {
                display_error("ERROR: Builtin failed: %s\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }
}

void exec_executable(char* const* const argv, const bool new_proc) {
    DEBUG_PRINT("DEBUG: Try executing executable: %s\n", argv[0]);

    if (!new_proc) {
        if (execvp(argv[0], argv) == -1) {
            display_error("ERROR: Unknown command: %s\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        assert(false);

    } else {
        exec_pid = fork();
        if (exec_pid == -1) {
            display_error("ERROR: Fork failed\n");
            return;
        }

        if (exec_pid) {
            // parent process

            // send SIGINT to child process
            struct sigaction old_sa;
            struct sigaction sa = {
                .sa_handler = send_sigint,
                .sa_flags   = 0,
            };
            sigemptyset(&sa.sa_mask);
            sigaction(SIGINT, &sa, &old_sa);

            // wait for execution
            int status;
            do {
                waitpid(exec_pid, &status, 0);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            // restore SIGINT handler
            sigaction(SIGINT, &old_sa, NULL);

        } else {
            // execution process

            if (execvp(argv[0], argv) == -1) {
                display_error("ERROR: Unknown command: %s\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            assert(false);
        }
    }
}
