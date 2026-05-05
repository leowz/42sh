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

/* Forward declaration - the full definition lives in 42sh.h, which itself
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
 * @param processes  `t_list*` of `t_process*` - one node per pipeline stage.
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
	t_list		*processes;
	t_job_status	status;
	int				notified;
	int				foreground;
}	t_job;

/**
 * @brief Place the shell in its own process group and claim the terminal.
 * @details Idempotent when the shell is already a session leader.  Skipped
 *          when `shell->interactive` is 0 (job control is a no-op without a
 *          controlling TTY).  On return `shell->shell_pgid` is authoritative
 *          and `shell->original_termios` holds the saved terminal settings.
 * @return 0 on success, 1 on `setpgid` failure.
 */
int			job_control_init(struct s_shell *shell);

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
 * @brief Resolve a bash-style job spec to a `t_job*`.
 * @details Accepted forms: NULL / "" / "%" / "%+" / "%%" → current job;
 *          "%-" → the job preceding the current one; "%N" → id N;
 *          "%prefix" → first job whose `cmd_line` starts with `prefix`.
 * @return Matching job or NULL if spec is malformed / no such job.
 */
t_job		*job_find_by_spec(struct s_shell *shell, const char *spec);

/**
 * @brief Free a job and all of its processes.
 * @details Matches the `void (*)(void *)` deleter signature of `ft_lstdel`.
 */
void		job_free(void *job_ptr);

/**
 * @brief Unlink `job` from `shell->jobs` and release it.
 * @details Clears `shell->current_job` if it referenced the removed job.
 *          No-op if `job` is not in the list (silent for safety).
 */
void		job_remove(struct s_shell *shell, t_job *job);

/**
 * @brief Hand a job off to the background and print `[id] pgid` on stderr.
 * @details The child was already forked with its own pgid by the caller.
 *          This does *not* wait - the job is reaped later by
 *          `job_update_statuses` between prompts.
 * @return 0 on success, 1 if `job` is NULL.
 */
int			job_launch_background(struct s_shell *shell, t_job *job);

/**
 * @brief Hand the terminal to `job`, wait for it, then take it back.
 * @details Uses `tcsetpgrp(terminal_fd, job->pgid)` to transfer terminal
 *          ownership so `SIGINT`/`SIGTSTP` reach the pipeline.  After
 *          `job_wait` returns, the shell reclaims the terminal and restores
 *          its saved termios.  Non-interactive shells skip the tc*
 *          handoff and just reap the pgid.
 * @return Exit status of the pipeline (last completed process), or
 *         128+WSTOPSIG if the job was stopped.
 */
int			job_launch_foreground(struct s_shell *shell, t_job *job);

/**
 * @brief Resume a stopped job in the foreground.
 * @details Sends `SIGCONT` to the job's pgid, hands the terminal back to it,
 *          and waits.  Leaves the job in `shell->jobs` if it stops again,
 *          removes it on completion.
 * @return Exit status, or 128+WSTOPSIG if stopped again.
 */
int			job_continue_foreground(struct s_shell *shell, t_job *job);

/**
 * @brief Resume a stopped job in the background.
 * @details Sends `SIGCONT` and prints `[id] cmd_line &` on stderr.  Does not
 *          wait - the job is reaped between prompts like any bg job.
 * @return 0 on success, 1 on invalid input or `kill` failure.
 */
int			job_continue_background(struct s_shell *shell, t_job *job);

/**
 * @brief Blocking reap of every process in `job`.
 * @details Loops `waitpid(-pgid, ..., WUNTRACED)` until every process is
 *          completed or at least one is stopped.  Per-process flags and the
 *          aggregate `status` are updated in place.
 * @return Exit status of the last completed process, 128+WSTOPSIG on stop,
 *         -1 on unrecoverable waitpid error.
 */
int			job_wait(struct s_shell *shell, t_job *job);

/**
 * @brief Apply a single `waitpid` result to one process of `job`.
 * @details Updates `status`, `completed`, `stopped`, clears `notified`, and
 *          flips the aggregate `status` to JOB_TERMINATED on a signalled
 *          death.  Does **not** recompute running/stopped - call
 *          `job_recompute_status` afterwards.
 * @return 1 if `pid` belonged to this job, 0 otherwise.
 */
int			job_apply_status(t_job *job, pid_t pid, int status);

/**
 * @brief Recompute `job->status` from the flags of its processes.
 * @details Result is JOB_DONE (all completed, no signalled), JOB_TERMINATED
 *          (preserved if set by `job_apply_status`), JOB_STOPPED (any stopped),
 *          or JOB_RUNNING.  Idempotent.
 */
void		job_recompute_status(t_job *job);

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
