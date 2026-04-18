/**
 * @file job_launch.c
 * @brief Launch helpers for jobs (background only, for now).
 * @author pulgamecanica
 */

#include "42sh.h"
#include "job_control.h"

int	job_launch_background(t_shell *shell, t_job *job)
{
	(void)shell;
	if (!job)
		return (1);
	job->foreground = 0;
	ft_putstr_fd("[", STDERR_FILENO);
	ft_putnbr_fd(job->id, STDERR_FILENO);
	ft_putstr_fd("] ", STDERR_FILENO);
	ft_putnbr_fd((int)job->pgid, STDERR_FILENO);
	ft_putstr_fd("\n", STDERR_FILENO);
	return (0);
}
