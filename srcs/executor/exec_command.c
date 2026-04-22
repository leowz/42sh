/**
 * @file exec_command.c
 * @brief Command execution functionality for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"
#include <string.h>

/**
 * @brief Apply assignments permanently to shell variables.
 * @details Used when command is empty (bare assignment) or in child for execve.
 * @param shell The shell instance.
 * @param assigns The list of assignments.
 * @param do_export Whether to export the variables.
 */
static void	apply_assignments(t_shell *shell, t_list *assigns, int do_export)
{
	t_list	*node;
	char	*name;
	char	*value;

	node = assigns;
	while (node)
	{
		split_assignment((char *)node->content, &name, &value);
		var_set(shell, name, value);
		if (do_export)
			var_export(shell, name);
		free(name);
		free(value);
		node = node->next;
	}
}

/**
 * @brief Handle empty command (just assignments and/or redirections).
 * @details Example: FOO=bar  or  FOO=bar > file
 * @param shell The shell instance.
 * @param cmd The command structure.
 * @return 0 on success, 1 on failure.
 */
static int	exec_assignment_only(t_shell *shell, t_cmd *cmd)
{
	int	saved_fds[3];

	apply_assignments(shell, cmd->assignments, 0);
	if (cmd->redirs)
	{
		if (setup_redirections(cmd->redirs, saved_fds) == -1)
			return (1);
		restore_redirections(saved_fds);
	}
	return (0);
}

/**
 * @brief Save old values, apply temporary assignments for builtin scope.
 * @details old_names/old_vals arrays store what to restore afterward.
 * @param shell The shell instance.
 * @param assigns The list of assignments.
 * @param old_names Array to store old variable names.
 * @param old_vals Array to store old variable values.
 * @return Count of saved assignments.
 */
static int	save_and_apply_assigns(t_shell *shell, t_list *assigns,
		char **old_names, char **old_vals)
{
	t_list	*node;
	char	*name;
	char	*value;
	char	*prev;
	int		i;

	i = 0;
	node = assigns;
	while (node)
	{
		split_assignment((char *)node->content, &name, &value);
		prev = var_get_value(shell, name);
		old_names[i] = ft_strdup(name);
		old_vals[i] = prev ? ft_strdup(prev) : NULL;
		var_set(shell, name, value);
		free(name);
		free(value);
		i++;
		node = node->next;
	}
	return (i);
}

/**
 * @brief Restore old variable values after builtin execution.
 * @param shell The shell instance.
 * @param old_names Array of old variable names.
 * @param old_vals Array of old variable values.
 * @param count Number of assignments to restore.
 */
static void	restore_assigns(t_shell *shell, char **old_names,
		char **old_vals, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		if (old_vals[i])
			var_set(shell, old_names[i], old_vals[i]);
		else
			var_unset(shell, old_names[i]);
		free(old_names[i]);
		free(old_vals[i]);
		i++;
	}
}

/**
 * @brief Execute a builtin with temporary assignments and redirections.
 * @details Assignments are scoped to this command only, then restored.
 * @param shell The shell instance.
 * @param cmd The command structure.
 * @param fn The builtin function to execute.
 * @return The exit status of the builtin.
 */
static int	exec_builtin(t_shell *shell, t_cmd *cmd, t_builtin_fn fn)
{
	int		saved_fds[3];
	int		status;
	char	*old_names[64];
	char	*old_vals[64];
	int		assign_count;

	assign_count = save_and_apply_assigns(shell, cmd->assignments,
			old_names, old_vals);
	if (setup_redirections(cmd->redirs, saved_fds) == -1)
	{
		restore_assigns(shell, old_names, old_vals, assign_count);
		return (1);
	}
	status = fn(shell, cmd->argc, cmd->argv);
	restore_redirections(saved_fds);
	restore_assigns(shell, old_names, old_vals, assign_count);
	return (status);
}

/**
 * @brief Child process: place in own pgrp, apply assignments, exec.
 * @details `setpgid(0, 0)` race-free paired with parent-side `setpgid(pid,pid)`;
 *          `tcsetpgrp` hands the terminal off before the child could block on
 *          stdin (parent's SIG_IGN for SIGTTOU is still inherited here).
 */
static void	exec_child(t_shell *shell, t_cmd *cmd)
{
	char	*path;

	setpgid(0, 0);
	if (shell->interactive)
		tcsetpgrp(shell->terminal_fd, getpid());
	signals_setup_child();
	apply_assignments(shell, cmd->assignments, 1);
	if (setup_redirections(cmd->redirs, NULL) == -1)
		_exit(1);
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
	free(path);
	exit(126);
}

static int	launch_simple_job(t_shell *shell, t_cmd *cmd, pid_t pid)
{
	t_job	*job;
	char	*cmd_line;
	int		status;

	setpgid(pid, pid);
	cmd_line = cmd_to_string(cmd);
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

int	execute_simple_command(t_shell *shell, t_cmd *cmd)
{
	t_builtin_fn	fn;
	pid_t			pid;

	expand_command(shell, cmd);
	if (!cmd->argv || !cmd->argv[0])
		return (exec_assignment_only(shell, cmd));
	fn = builtin_get(cmd->argv[0]);
	if (fn)
		return (exec_builtin(shell, cmd, fn));
	pid = fork();
	if (pid == -1)
	{
		ft_putstr_fd("42sh: fork: ", 2);
		ft_putendl_fd(strerror(errno), 2);
		return (1);
	}
	if (pid == 0)
		exec_child(shell, cmd);
	return (launch_simple_job(shell, cmd, pid));
}
