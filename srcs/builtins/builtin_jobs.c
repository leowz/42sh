/**
 * @file builtin_jobs.c
 * @brief `jobs` builtin — list active background jobs.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "builtins.h"
#include "job_control.h"

static void	print_job_line(t_shell *shell, t_job *job)
{
	char	marker;

	marker = ' ';
	if (shell->current_job == job)
		marker = '+';
	ft_putstr_fd("[", STDOUT_FILENO);
	ft_putnbr_fd(job->id, STDOUT_FILENO);
	ft_putstr_fd("]  ", STDOUT_FILENO);
	ft_putchar_fd(marker, STDOUT_FILENO);
	ft_putstr_fd(" ", STDOUT_FILENO);
	ft_putstr_fd((char *)job_status_str(job->status), STDOUT_FILENO);
	ft_putstr_fd("\t", STDOUT_FILENO);
	ft_putstr_fd(job->cmd_line ? job->cmd_line : "", STDOUT_FILENO);
	ft_putstr_fd("\n", STDOUT_FILENO);
}

int	builtin_jobs(t_shell *shell, int argc, char **argv)
{
	t_list	*node;

	(void)argc;
	(void)argv;
	job_update_statuses(shell);
	node = shell->jobs;
	while (node)
	{
		print_job_line(shell, LST_JOB(node));
		LST_JOB(node)->notified = 1;
		node = node->next;
	}
	return (0);
}
