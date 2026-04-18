/**
 * @file job_control.h
 * @brief Job control definitions for the 42sh shell.
 * @author pulgamecanica
 *
 * A **job** is one pipeline launched by the shell (foreground or background).
 * Each job has its own process group (`pgid`) so the controlling terminal can
 * deliver signals to the whole pipeline at once, and so the shell can suspend
 * or resume it independently of itself.
 *
 * Ownership model:
 *   - `t_shell.jobs`       : `t_list*` of `t_job*`       (one entry per known job)
 *   - `t_job.processes`    : `t_list*` of `t_process*`   (one entry per pipeline stage)
 */

#ifndef JOB_CONTROL_H
# define JOB_CONTROL_H

# include <sys/types.h>
# include "libft.h"

/* Forward declaration — the full definition lives in 42sh.h, which itself
 * includes this header.  We only need a pointer type here. */
struct s_shell;

/**
 * @brief Lifecycle state of a job, recomputed from its processes.
 *
 * @details
 * - JOB_RUNNING    : at least one process is still running and none stopped.
 * - JOB_STOPPED    : at least one process received SIGTSTP/SIGSTOP (Ctrl-Z).
 * - JOB_DONE       : every process exited normally (any exit code).
 * - JOB_TERMINATED : at least one process was killed by a signal.
 */
typedef enum e_job_status
{
	JOB_RUNNING,
	JOB_STOPPED,
	JOB_DONE,
	JOB_TERMINATED
}	t_job_status;

/**
 * @brief A single process belonging to a job (one pipeline stage).
 *
 * @details Stored in `t_job.processes` as the `content` of a `t_list` node.
 *          Reaped and updated by `job_update_statuses`.
 *
 * @param pid       The child pid returned by fork().
 * @param cmd       Human-readable description of this stage (for listings).
 * @param status    Raw `wstatus` value from the last waitpid() on this pid.
 * @param completed 1 once the process has exited (normally or by signal).
 * @param stopped   1 while the process is stopped (SIGTSTP/SIGSTOP).
 */
typedef struct s_process
{
	pid_t	pid;
	char	*cmd;
	int		status;
	int		completed;
	int		stopped;
}	t_process;

/**
 * @brief One pipeline launched by the shell (background or foreground).
 *
 * @details Stored in `t_shell.jobs` as the `content` of a `t_list` node.
 *          Built by `job_create`, grown by `job_add_process`, reaped by
 *          `job_update_statuses`, and released by `job_free` (either via
 *          `job_notify` when completed, or `job_control_cleanup` on exit).
 *
 * @param id         Shell-assigned sequential id (1-based).  Used by `%N`
 *                   job-spec and by `jobs` listings.
 * @param pgid       Process-group id shared by every stage of the pipeline.
 * @param cmd_line   Displayed by `jobs`/notifications.  Built from the AST
 *                   subtree via `ast_to_string`, not the raw input line,
 *                   so `ls ; sleep 5 &` shows only `sleep 5`.
 * @param processes  `t_list*` of `t_process*` — one node per pipeline stage.
 * @param status     Aggregate lifecycle state (see `t_job_status`).
 * @param notified   1 once the user has been informed about the current
 *                   `status`; cleared whenever `status` changes so the next
 *                   `job_notify` prints it again.
 * @param foreground 1 if the job currently owns the controlling terminal.
 */
typedef struct s_job
{
	int				id;
	pid_t			pgid;
	char			*cmd_line;
	t_list			*processes;
	t_job_status	status;
	int				notified;
	int				foreground;
}	t_job;

/**
 * @brief Allocate a job, append it to `shell->jobs`, and make it `current_job`.
 * @details The next free id is `max(existing ids) + 1`.  `cmd_line` is
 *          duplicated with `ft_strdup` and released by `job_free`.
 * @return Newly created job, or NULL on allocation failure.
 */
t_job		*job_create(struct s_shell *shell, const char *cmd_line);

/**
 * @brief Register a forked child as a stage of `job`.
 * @details Appends a `t_process` to `job->processes`.  `cmd` is duplicated.
 */
void		job_add_process(t_job *job, pid_t pid, const char *cmd);

/** @brief Find a job by its shell-assigned id, or NULL. */
t_job		*job_find_by_id(struct s_shell *shell, int id);

/** @brief Find the job containing a given pid, or NULL. */
t_job		*job_find_by_pid(struct s_shell *shell, pid_t pid);

/**
 * @brief Free a job and all of its processes.
 * @details Matches the `void (*)(void *)` deleter signature of `ft_lstdel`.
 */
void		job_free(void *job_ptr);

/**
 * @brief Hand a job off to the background and print `[id] pgid` on stderr.
 * @details The child was already forked with its own pgid by the caller.
 *          This does *not* wait — the job is reaped later by
 *          `job_update_statuses` between prompts.
 * @return 0 on success, 1 if `job` is NULL.
 */
int			job_launch_background(struct s_shell *shell, t_job *job);

/**
 * @brief Non-blocking reap of every shell-owned child.
 * @details Loops `waitpid(-1, ..., WNOHANG | WUNTRACED | WCONTINUED)` until
 *          there is nothing more to collect.  Updates per-process flags and
 *          recomputes the owning job's `status`.  Safe to call when no jobs
 *          are tracked.
 */
void		job_update_statuses(struct s_shell *shell);

/**
 * @brief Print a status line for every job that has not been notified yet,
 *        then drop JOB_DONE / JOB_TERMINATED jobs from `shell->jobs`.
 * @details Call between prompts.  `shell->current_job` is cleared if it
 *          referenced a job that gets removed.
 */
void		job_notify(struct s_shell *shell);

/**
 * @brief Human-readable label for a `t_job_status` value (for listings).
 */
const char	*job_status_str(t_job_status s);

/**
 * @brief Release every job and process still tracked by `shell`.
 * @details Called from `shell_cleanup` on exit.  Leaves `shell->jobs` NULL
 *          and `shell->current_job` NULL.
 */
void		job_control_cleanup(struct s_shell *shell);

#endif
