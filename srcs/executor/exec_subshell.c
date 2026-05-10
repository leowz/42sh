/**
 * @file exec_subshell.c
 * @brief Subshell, block, and background execution for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"
#include "signals.h"
#include <string.h>

static void	subshell_child(t_shell *shell, t_ast *ast)
{
	int	status;

	setpgid(0, 0);
	if (shell->interactive)
		tcsetpgrp(shell->terminal_fd, getpid());
	signals_setup_child();
	if (setup_redirections(ast->data.group->redirs, NULL) == -1)
		_exit(1);
	status = executor_execute(shell, ast->data.group->child);
	fflush(NULL);
	_exit(status);
}

static int	launch_subshell_job(t_shell *shell, t_ast *ast, pid_t pid)
{
	t_job	*job;
	char	*cmd_line;
	int		status;

	setpgid(pid, pid);
	cmd_line = ast_to_string(ast);
	job = job_create(shell, cmd_line);
	free(cmd_line);
	if (!job)
	{
		waitpid(pid, &status, 0);
		return (get_exit_status(status));
	}
	job->pgid = pid;
	job_add_process(job, pid, job->cmd_line);
	status = job_launch_foreground(shell, job);
	if (job->status == JOB_STOPPED)
		job->notified = 0;
	else
		job_remove(shell, job);
	return (status);
}

/**
 * @details ( cmd ) - runs in a forked child (subshell) with its own pgrp,
 *                   routed through the job-control machinery so Ctrl-Z works.
 */
int	execute_subshell(t_shell *shell, t_ast *ast)
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
	{
		ft_putstr_fd("42sh: fork: ", 2);
		ft_putendl_fd(strerror(errno), 2);
		return (1);
	}
	if (pid == 0)
		subshell_child(shell, ast);
	return (launch_subshell_job(shell, ast, pid));
}

/**
 * @details { cmd; } - runs in the current shell, but with its own redirections.
 */
int	execute_block(t_shell *shell, t_ast *ast)
{
	int	saved_fds[3];
	int	status;

	if (setup_redirections(ast->data.group->redirs, saved_fds) == -1)
		return (1);
	status = executor_execute(shell, ast->data.group->child);
	restore_redirections(saved_fds);
	return (status);
}

/**
 * @brief Execute a background command.
 * @details cmd & - runs in a forked child with its own process group.
 * @param shell The shell instance.
 * @param ast The abstract syntax tree node.
 * @return The exit status of the command.
 */
static void	bg_child(t_shell *shell, t_ast *ast)
{
	int	devnull;
	int	status;

	setpgid(0, 0);
	signals_setup_child();
	devnull = open("/dev/null", O_RDONLY);
	if (devnull >= 0)
	{
		dup2(devnull, STDIN_FILENO);
		close(devnull);
	}
	if (setup_redirections(ast->data.group->redirs, NULL) == -1)
		_exit(1);
	status = executor_execute(shell, ast->data.group->child);
	fflush(NULL);
	_exit(status);
}

int	execute_background(t_shell *shell, t_ast *ast)
{
	pid_t	pid;
	t_job	*job;
	char	*cmd_line;

	pid = fork();
	if (pid == -1)
	{
		ft_putstr_fd("42sh: fork: ", 2);
		ft_putendl_fd(strerror(errno), 2);
		return (1);
	}
	if (pid == 0)
		bg_child(shell, ast);
	setpgid(pid, pid);
	cmd_line = ast_to_string(ast->data.group->child);
	job = job_create(shell, cmd_line);
	free(cmd_line);
	if (!job)
		return (1);
	job->pgid = pid;
	job_add_process(job, pid, job->cmd_line);
	job_launch_background(shell, job);
	return (0);
}
