/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_subshell.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/27 00:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/03/27 00:00:00 by wengzhang        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "42sh.h"
#include "executor.h"
#include <string.h>

/*
** ( cmd ) - runs in a forked child (subshell).
** Variables modified inside do NOT affect the parent.
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
		if (setup_redirections(ast->data.group.redirs, NULL) == -1)
			_exit(1);
		status = executor_execute(shell, ast->data.group.child);
		_exit(status);
	}
	waitpid(pid, &wstatus, 0);
	return (get_exit_status(wstatus));
}

/*
** { cmd; } - runs in the current shell, but with its own redirections.
** Variables modified inside DO affect the current shell.
*/
int	execute_block(t_shell *shell, t_ast *ast)
{
	int	saved_fds[3];
	int	status;

	if (setup_redirections(ast->data.group.redirs, saved_fds) == -1)
		return (1);
	status = executor_execute(shell, ast->data.group.child);
	restore_redirections(saved_fds);
	return (status);
}

/*
** cmd & - runs in a forked child with its own process group.
** Stdin redirected from /dev/null. Parent returns immediately.
*/
int	execute_background(t_shell *shell, t_ast *ast)
{
	pid_t	pid;
	int		status;
	int		devnull;

	pid = fork();
	if (pid == -1)
	{
		ft_putstr_fd("42sh: fork: ", 2);
		ft_putendl_fd(strerror(errno), 2);
		return (1);
	}
	if (pid == 0)
	{
		setpgid(0, 0);
		signals_setup_child();
		devnull = open("/dev/null", O_RDONLY);
		if (devnull >= 0)
		{
			dup2(devnull, STDIN_FILENO);
			close(devnull);
		}
		if (setup_redirections(ast->data.group.redirs, NULL) == -1)
			_exit(1);
		status = executor_execute(shell, ast->data.group.child);
		_exit(status);
	}
	ft_putstr_fd("[", 2);
	ft_putnbr_fd((int)pid, 2);
	ft_putendl_fd("]", 2);
	return (0);
}
