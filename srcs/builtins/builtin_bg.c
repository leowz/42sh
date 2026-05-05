/**
 * @file builtin_bg.c
 * @brief `bg [%spec]` — resume a stopped job in the background.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "builtins.h"
#include "job_control.h"

static void	no_such_job(const char *spec)
{
	ft_putstr_fd("42sh: bg: ", STDERR_FILENO);
	ft_putstr_fd(spec ? (char *)spec : "current", STDERR_FILENO);
	ft_putendl_fd(": no such job", STDERR_FILENO);
}

int	builtin_bg(t_shell *shell, int argc, char **argv)
{
	t_job		*job;
	const char	*spec;

	spec = NULL;
	if (argc > 1)
		spec = argv[1];
	job_update_statuses(shell);
	job = job_find_by_spec(shell, spec);
	if (!job)
	{
		no_such_job(spec);
		return (1);
	}
	return (job_continue_background(shell, job));
}
