/**
 * @file exec_subshell.c
 * @brief Subshell, block, and background execution for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "ast.h"
#include "builtins.h"
#include "executor.h"
#include "expander.h"
#include "signals.h"
#include <string.h>

static void	subshell_child(t_shell *shell, t_ast *ast)
{
	int	status;

	setpgid(0, 0);
	shell->shell_pgid = getpid();
	if (shell->interactive)
		tcsetpgrp(shell->terminal_fd, shell->shell_pgid);
	signals_setup_subshell();
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

	fflush(NULL);
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
	if (!shell->interactive)
	{
		devnull = open("/dev/null", O_RDONLY);
		if (devnull >= 0)
		{
			dup2(devnull, STDIN_FILENO);
			close(devnull);
		}
	}
	if (setup_redirections(ast->data.group->redirs, NULL) == -1)
		_exit(1);
	status = executor_execute(shell, ast->data.group->child);
	fflush(NULL);
	_exit(status);
}

/**
 * @brief Apply (and export) inline assignments in a bg subshell child.
 * @details Equivalent to `apply_assignments(shell, ..., 1)` in exec_command,
 *          inlined here because that helper isn't exported and pulling it
 *          in would over-couple the modules.
 */
static void	bg_apply_assignments(t_shell *shell, t_list *assigns)
{
	t_list	*node;
	char	*name;
	char	*value;

	node = assigns;
	while (node)
	{
		split_assignment((char *)node->content, &name, &value);
		var_set(shell, name, value);
		var_export(shell, name);
		free(name);
		free(value);
		node = node->next;
	}
}

/**
 * @brief Child path for `simple-command &`: fork once, exec directly.
 * @details Bypasses the bg_child → executor_execute → exec_command →
 *          exec_child double-fork. That chain put the actual program in
 *          its own pgrp (not the job's pgrp) and made exec_child call
 *          tcsetpgrp from a background pgrp — both broken. This single
 *          fork keeps the program in the same pgrp the parent tracks
 *          and never claims the terminal.
 */
static void	bg_simple_child(t_shell *shell, t_ast *outer, t_cmd *cmd)
{
	char	*path;
	int		status;

	setpgid(0, 0);
	signals_setup_child();
	if (setup_redirections(outer->data.group->redirs, NULL) == -1)
		_exit(1);
	if (setup_redirections(cmd->redirs, NULL) == -1)
		_exit(1);
	if (!cmd->argv || !cmd->argv[0])
		_exit(0);
	bg_apply_assignments(shell, cmd->assignments);
	if (builtin_get(cmd->argv[0]))
	{
		status = builtin_get(cmd->argv[0])(shell, cmd->argc, cmd->argv);
		fflush(NULL);
		_exit(status);
	}
	path = find_command(shell, cmd->argv[0]);
	if (!path)
	{
		ft_putstr_fd("42sh: ", 2);
		ft_putstr_fd(cmd->argv[0], 2);
		ft_putendl_fd(": command not found", 2);
		_exit(127);
	}
	execve(path, cmd->argv, var_get_environ(shell));
	ft_putstr_fd("42sh: ", 2);
	ft_putstr_fd(cmd->argv[0], 2);
	ft_putstr_fd(": ", 2);
	ft_putendl_fd(strerror(errno), 2);
	_exit(126);
}

int	execute_background(t_shell *shell, t_ast *ast)
{
	t_ast	*inner;
	t_cmd	*cmd;
	pid_t	pid;
	t_job	*job;
	char	*cmd_line;

	inner = ast->data.group->child;
	cmd = NULL;
	if (inner && inner->type == NODE_COMMAND)
	{
		cmd = inner->data.cmd;
		if (expand_command(shell, cmd) == -1)
			return (1);
	}
	fflush(NULL);
	pid = fork();
	if (pid == -1)
	{
		ft_putstr_fd("42sh: fork: ", 2);
		ft_putendl_fd(strerror(errno), 2);
		return (1);
	}
	if (pid == 0)
	{
		if (cmd)
			bg_simple_child(shell, ast, cmd);
		else
			bg_child(shell, ast);
	}
	setpgid(pid, pid);
	cmd_line = ast_to_string(inner);
	job = job_create(shell, cmd_line);
	free(cmd_line);
	if (!job)
		return (1);
	job->pgid = pid;
	job_add_process(job, pid, job->cmd_line);
	job_launch_background(shell, job);
	return (0);
}
