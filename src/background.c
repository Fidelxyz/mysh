#define _POSIX_C_SOURCE 200809L

#include "background.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "io_helpers.h"

// ========== Job List ==========

static const size_t INIT_JOBS_CAPACITY = 16;

static JobInfo* jobs          = NULL;
static size_t   jobs_len      = 0;
static size_t   jobs_capacity = 0;

void init_background() {
    jobs          = malloc(INIT_JOBS_CAPACITY * sizeof(JobInfo));
    jobs_capacity = INIT_JOBS_CAPACITY;
}

void free_background() {
    check_background_status(true);

    for (size_t i = 0; i < jobs_len; i++) {
        // kill all non-terminated processes
        if (jobs[i].n_running == 0) continue;
        for (size_t j = 0; j < jobs[i].n_pid; j++) {
            if (jobs[i].pids[j] != -1) {
                kill(jobs[i].pids[j], SIGKILL);
                DEBUG_PRINT("DEBUG: Killed non-terminated process %d\n",
                            jobs[i].pids[j]);
            }
        }
        free(jobs[i].pids);
        free(jobs[i].cmd);
    }
    free(jobs);
}

static int push_job(pid_t* const pids, const size_t n_proc,
                    const char* const cmd) {
    assert(jobs_len <= jobs_capacity);

    if (jobs_len == jobs_capacity) {
        jobs_capacity *= 2;
        jobs           = realloc(jobs, jobs_capacity * sizeof(JobInfo));
    }

    pid_t* const owned_pids = malloc((n_proc + 1) * sizeof(pid_t));
    memcpy(owned_pids, pids, n_proc * sizeof(pid_t));
    owned_pids[n_proc] = -1;  // pids is terminated by -1

    jobs[jobs_len] = (JobInfo){.pids      = owned_pids,
                               .n_pid     = n_proc,
                               .n_running = n_proc,
                               .cmd       = strdup(cmd)};

    return ++jobs_len;  // return index + 1
}

/**
 * @brief Attempt to pop a job from the list.
 *
 * @param [in] pid The pid of the process.
 * @param [out] cmd The command string of the job.
 * @return The index of the job in the list (starts from 1) if all processes of
 * the job have finished, or -1 otherwise.
 */
static int pop_job(const pid_t pid, char* const cmd) {
    size_t i_job;
    for (i_job = 0; i_job < jobs_len; i_job++) {
        if (jobs[i_job].n_running == 0) continue;

        const JobInfo* const job = &jobs[i_job];

        size_t i_pid;
        for (i_pid = 0; i_pid < job->n_pid; i_pid++) {
            if (job->pids[i_pid] == pid) {  // found pid
                jobs[i_job].pids[i_pid] = -1;
                jobs[i_job].n_running--;
                break;
            }
        }
        if (i_pid < job->n_pid) break;
    }
    if (i_job == jobs_len) return -1;  // not found

    if (jobs[i_job].n_running > 0) return -1;  // not finished

    // write output
    strcpy(cmd, jobs[i_job].cmd);

    // free resources
    free(jobs[i_job].pids);
    free(jobs[i_job].cmd);
    jobs[i_job].pids = NULL;
    jobs[i_job].cmd  = NULL;

    // shrink the list
    while (jobs_len > 0 && jobs[jobs_len - 1].n_running == 0) {
        jobs_len--;
        assert(jobs[jobs_len].pids == NULL);
        assert(jobs[jobs_len].cmd == NULL);
    }

    if (jobs_len == 0 && jobs_capacity > INIT_JOBS_CAPACITY) {
        free_background();
        init_background();
    }

    return i_job + 1;
}

// ========== Public Interface ==========

void add_background_job(pid_t* const pids, const size_t n_proc,
                        const char* const cmd) {
    const int index = push_job(pids, n_proc, cmd);

    display_message("[%d]\t%d\n", index, pids[n_proc - 1]);
}

void check_background_status(bool slience) {
    pid_t pid;
    do {
        int status;
        pid = waitpid(-1, &status, WNOHANG);
        if (pid == -1) return;

        // if a job process has finished
        if (pid > 0 && (WIFEXITED(status) || WIFSIGNALED(status))) {
            char      cmd[MAX_STR_LEN];
            const int index = pop_job(pid, cmd);
            if (index == -1) continue;  // not a job process
            if (!slience) {
                display_message("[%d]+  Done\t%s\n", index, cmd);
            }
        }
    } while (pid);
}

const JobInfo* read_job(const JobInfo* const prev) {
    const JobInfo* curr = jobs;
    if (prev == NULL) {
        curr = jobs;
    } else {
        curr = prev + 1;
    }

    const JobInfo* const end = jobs + jobs_len;
    // get the next valid job
    while (curr < end && curr->n_running == 0) {
        curr++;
    }
    if (curr == end) return NULL;

    assert(curr->n_running > 0);
    assert(curr->pids != NULL);
    assert(curr->cmd != NULL);

    return curr;
}
