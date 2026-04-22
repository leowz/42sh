/**
 * @file exec_pipeline.c
 * @brief Pipeline command execution functionality for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"
#include "job_control.h"
#include <string.h>

/**
 * @brief Flatten nested PIPE nodes left-to-right into a flat array.
 * @details Example: (A | B) | C  =>  [A, B, C]
 */
static int	collect_pipeline(t_ast *ast, t_ast **cmds, int max)
{
	int	n;

	if (ast->type != NODE_PIPE)
	{
		cmds[0] = ast;
		return (1);
	}
	n = collect_pipeline(ast->data.binary->left, cmds, max);
	if (n >= max)
		return (n);
	cmds[n] = ast->data.binary->right;
	return (n + 1);
}

static void	close_pipes(int pipes[][2], int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		close(pipes[i][0]);
		close(pipes[i][1]);
		i++;
	}
}

/**
 * @brief Pipeline stage in the child: join the job pgrp, wire pipes, exec.
 * @details info = { stage_index, stage_count, group_pgid }.  When
 *          `group_pgid == 0` this stage becomes the pgrp leader; the parent
 *          mirrors `setpgid` race-free.
 */
static void	pipe_child(t_shell *shell, t_ast *cmd_ast,
		int pipes[][2], int info[3])
{
	int		i;
	int		n;
	pid_t	pgid;
	int		status;

	i = info[0];
	n = info[1];
	pgid = (pid_t)info[2];
	setpgid(0, pgid);
	if (shell->interactive && pgid == 0)
		tcsetpgrp(shell->terminal_fd, getpid());
	signals_setup_child();
	if (i > 0)
		dup2(pipes[i - 1][0], STDIN_FILENO);
	if (i < n - 1)
		dup2(pipes[i][1], STDOUT_FILENO);
	close_pipes(pipes, n - 1);
	if (cmd_ast->type == NODE_COMMAND)
	{
		expand_command(shell, cmd_ast->data.cmd);
		if (setup_redirections(cmd_ast->data.cmd->redirs, NULL) == -1)
			_exit(1);
		if (!cmd_ast->data.cmd->argv || !cmd_ast->data.cmd->argv[0])
			_exit(0);
		if (builtin_get(cmd_ast->data.cmd->argv[0]))
		{
			status = builtin_get(cmd_ast->data.cmd->argv[0])(shell,
					cmd_ast->data.cmd->argc, cmd_ast->data.cmd->argv);
			_exit(status);
		}
		exec_pipeline_external(shell, cmd_ast->data.cmd);
	}
	status = executor_execute(shell, cmd_ast);
	exit(status);
}

void	exec_pipeline_external(t_shell *shell, t_cmd *cmd)
{
	char	*path;

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
	exit(126);
}

static int	open_pipes(int pipes[][2], int n)
{
	int	i;

	i = -1;
	while (++i < n - 1)
	{
		if (pipe(pipes[i]) == -1)
		{
			while (--i >= 0)
			{
				close(pipes[i][0]);
				close(pipes[i][1]);
			}
			return (-1);
		}
	}
	return (0);
}

static int	fork_stage(t_shell *shell, t_ast *cmd_ast,
		int pipes[][2], int info[3])
{
	pid_t	pid;

	pid = fork();
	if (pid == -1)
		return (-1);
	if (pid == 0)
		pipe_child(shell, cmd_ast, pipes, info);
	return ((int)pid);
}

static int	spawn_pipeline(t_shell *shell, t_ast **cmds, int n,
		int pipes[][2], t_job *job)
{
	int		i;
	int		info[3];
	pid_t	pgid;
	pid_t	pid;

	pgid = 0;
	i = -1;
	while (++i < n)
	{
		info[0] = i;
		info[1] = n;
		info[2] = (int)pgid;
		pid = fork_stage(shell, cmds[i], pipes, info);
		if (pid == -1)
			return (-1);
		if (pgid == 0)
			pgid = pid;
		setpgid(pid, pgid);
		job_add_process(job, pid, NULL);
	}
	job->pgid = pgid;
	return (0);
}

int	execute_pipeline(t_shell *shell, t_ast *ast)
{
	t_ast	*cmds[MAX_PIPELINE];
	int		pipes[MAX_PIPELINE][2];
	int		n;
	t_job	*job;
	char	*cmd_line;
	int		status;

	n = collect_pipeline(ast, cmds, MAX_PIPELINE);
	if (open_pipes(pipes, n) == -1)
		return (1);
	cmd_line = ast_to_string(ast);
	job = job_create(shell, cmd_line);
	free(cmd_line);
	if (!job)
	{
		close_pipes(pipes, n - 1);
		return (1);
	}
	if (spawn_pipeline(shell, cmds, n, pipes, job) == -1)
	{
		close_pipes(pipes, n - 1);
		job_remove(shell, job);
		return (1);
	}
	close_pipes(pipes, n - 1);
	status = job_launch_foreground(shell, job);
	if (job->status == JOB_STOPPED)
		job->notified = 0;
	else
		job_remove(shell, job);
	return (status);
}
