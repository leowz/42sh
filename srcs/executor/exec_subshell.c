/**
 * @file command_search.c
 * @brief Command search functionality for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"
#include "signals.h"
#include <string.h>

/**
 * @brief Execute a command in a subshell.
 * @details ( cmd ) - runs in a forked child (subshell).
 * @param shell The shell instance.
 * @param ast The abstract syntax tree node.
 * @return The exit status of the command.
 */
int	execute_subshell(t_shell *shell, t_ast *ast)
{
	pid_t	pid;
	int		wstatus;
	int		status;

	pid = fork();
	if (pid == -1)
	{
		ft_putstr_fd("42sh: fork: ", 2);
		ft_putendl_fd(strerror(errno), 2);
		return (1);
	}
	if (pid == 0)
	{
		signals_setup_child();
		if (setup_redirections(ast->data.group->redirs, NULL) == -1)
			_exit(1);
		status = executor_execute(shell, ast->data.group->child);
		_exit(status);
	}
	waitpid(pid, &wstatus, 0);
	return (get_exit_status(wstatus));
}

/**
 * @brief Execute a command block.
 * @details { cmd; } - runs in the current shell, but with its own redirections.
 * @param shell The shell instance.
 * @param ast The abstract syntax tree node.
 * @return The exit status of the command.
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
