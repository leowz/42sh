/**
 * @file exec_pipeline.c
 * @brief Pipeline command execution functionality for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"
#include <string.h>

/**
 * @brief Flatten nested PIPE nodes left-to-right into a flat array.
 * @details Example: (A | B) | C  =>  [A, B, C]
 * @param ast The abstract syntax tree node.
 * @param cmds The array to store the pipeline commands.
 * @param max The maximum number of commands to store.
 * @return The number of commands collected.
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

/**
 * @brief Close all pipe fds in the array.
 * @param pipes The array of pipe file descriptors.
 * @param count The number of pipes to close.
 */
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
 * @brief Execute a single pipeline stage in the child process.
 * @details Wire stdin/stdout from pipes, then exec or run builtin.
 * @param shell The shell instance.
 * @param cmd_ast The abstract syntax tree node for the command.
 * @param pipes The array of pipe file descriptors.
 * @param info The information array for the pipeline stage.
 */
static void	pipe_child(t_shell *shell, t_ast *cmd_ast,
		int pipes[][2], int info[3])
{
	int	i;
	int	n;
	int	status;

	i = info[0];
	n = info[1];
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

/**
 * @brief Execute external command in pipeline child (does not return).
 * @param shell The shell instance.
 * @param cmd The command structure.
 */
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

int	execute_pipeline(t_shell *shell, t_ast *ast)
{
	t_ast	*cmds[MAX_PIPELINE];
	int		pipes[MAX_PIPELINE][2];
	pid_t	pids[MAX_PIPELINE];
	int		n;
	int		i;
	int		wstatus;
	int		info[3];

	n = collect_pipeline(ast, cmds, MAX_PIPELINE);
	i = -1;
	while (++i < n - 1)
		if (pipe(pipes[i]) == -1)
			return (1);
	i = -1;
	while (++i < n)
	{
		pids[i] = fork();
		if (pids[i] == -1)
			return (1);
		if (pids[i] == 0)
		{
			info[0] = i;
			info[1] = n;
			pipe_child(shell, cmds[i], pipes, info);
		}
	}
	close_pipes(pipes, n - 1);
	i = -1;
	while (++i < n)
		waitpid(pids[i], &wstatus, 0);
	return (get_exit_status(wstatus));
}
