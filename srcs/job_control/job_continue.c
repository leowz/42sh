/**
 * @file job_continue.c
 * @brief Resume a stopped job in the foreground (`fg`) or background (`bg`).
 * @author pulgamecanica
 */

#include "42sh.h"
#include "job_control.h"

static void	mark_running(t_job *job)
{
	t_list		*node;
	t_process	*p;

	node = job->processes;
	while (node)
	{
		p = LST_PROC(node);
		p->stopped = 0;
		node = node->next;
	}
	job->status = JOB_RUNNING;
	job->notified = 0;
}

int	job_continue_foreground(t_shell *shell, t_job *job)
{
	int	status;

	if (!job)
		return (1);
	mark_running(job);
	shell->current_job = job;
	if (kill(-job->pgid, SIGCONT) < 0)
		return (1);
	status = job_launch_foreground(shell, job);
	if (job->status == JOB_STOPPED)
		job->notified = 0;
	else
		job_remove(shell, job);
	return (status);
}

int	job_continue_background(t_shell *shell, t_job *job)
{
	if (!job)
		return (1);
	mark_running(job);
	job->foreground = 0;
	shell->current_job = job;
	if (kill(-job->pgid, SIGCONT) < 0)
		return (1);
	ft_putstr_fd("[", STDERR_FILENO);
	ft_putnbr_fd(job->id, STDERR_FILENO);
	ft_putstr_fd("] ", STDERR_FILENO);
	ft_putstr_fd(job->cmd_line ? job->cmd_line : "", STDERR_FILENO);
	ft_putstr_fd(" &\n", STDERR_FILENO);
	return (0);
}
