#ifndef __BACKGROUND_H__
#define __BACKGROUND_H__

#include <stdbool.h>
#include <sys/types.h>

typedef struct {
    pid_t* pids;
    size_t n_pid;
    size_t n_running;
    char*  cmd;
} JobInfo;

void init_background();

void free_background();

/**
 * @prarm [in] An array of pids of the processes.
 * @param [in] n_proc The number of processes.
 * @param [in] cmd The command string.
 */
void add_background_job(pid_t* pids, size_t n_proc, const char* cmd);

void check_background_status(bool slience);

const JobInfo* read_job(const JobInfo* prev);

#endif
