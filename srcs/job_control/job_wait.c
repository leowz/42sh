/**
 * @file job_wait.c
 * @brief Foreground handoff: block on a job's process group, manage terminal.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "executor.h"
#include "job_control.h"
#include <sys/wait.h>

static int	last_process_exit(t_job *job)
{
	t_list		*node;
	t_process	*last;

	node = job->processes;
	last = NULL;
	while (node)
	{
		last = LST_PROC(node);
		node = node->next;
	}
	if (!last)
		return (0);
	return (get_exit_status(last->status));
}

int	job_wait(t_shell *shell, t_job *job)
{
	pid_t	pid;
	int		status;

	(void)shell;
	while (1)
	{
		pid = waitpid(-job->pgid, &status, WUNTRACED);
		if (pid < 0)
		{
			if (errno == ECHILD)
				break ;
			return (-1);
		}
		job_apply_status(job, pid, status);
		job_recompute_status(job);
		if (job->status == JOB_STOPPED)
			return (128 + WSTOPSIG(status));
		if (job->status == JOB_DONE || job->status == JOB_TERMINATED)
			break ;
	}
	return (last_process_exit(job));
}

int	job_launch_foreground(t_shell *shell, t_job *job)
{
	int	status;

	if (!job)
		return (1);
	job->foreground = 1;
	if (shell->interactive)
		tcsetpgrp(shell->terminal_fd, job->pgid);
	status = job_wait(shell, job);
	if (shell->interactive)
	{
		tcsetpgrp(shell->terminal_fd, shell->shell_pgid);
		tcsetattr(shell->terminal_fd, TCSADRAIN, &shell->original_termios);
	}
	return (status);
}
