/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   redirections.c                                    :+:      :+:    :+:   */
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

static int	redir_get_fd(t_redir *redir)
{
	if (redir->fd >= 0)
		return (redir->fd);
	if (redir->type == TOK_REDIR_IN || redir->type == TOK_HEREDOC
		|| redir->type == TOK_REDIR_DUP_IN)
		return (0);
	return (1);
}

static int	redir_open_file(t_redir *redir)
{
	int	fd;

	if (redir->type == TOK_REDIR_IN)
		fd = open(redir->target, O_RDONLY);
	else if (redir->type == TOK_REDIR_OUT)
		fd = open(redir->target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else if (redir->type == TOK_REDIR_APPEND)
		fd = open(redir->target, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else
		return (-2);
	if (fd == -1)
	{
		ft_putstr_fd("42sh: ", 2);
		ft_putstr_fd(redir->target, 2);
		ft_putstr_fd(": ", 2);
		ft_putendl_fd(strerror(errno), 2);
	}
	return (fd);
}

static int	apply_dup_redir(t_redir *redir, int target_fd)
{
	int	src_fd;

	src_fd = redir_get_fd(redir);
	if (ft_strcmp(redir->target, "-") == 0)
	{
		close(src_fd);
		return (0);
	}
	if (!ft_isdigit(redir->target[0]))
	{
		ft_putstr_fd("42sh: ", 2);
		ft_putstr_fd(redir->target, 2);
		ft_putendl_fd(": ambiguous redirect", 2);
		return (-1);
	}
	(void)target_fd;
	if (dup2(ft_atoi(redir->target), src_fd) == -1)
	{
		ft_putstr_fd("42sh: ", 2);
		ft_putendl_fd(strerror(errno), 2);
		return (-1);
	}
	return (0);
}

static int	apply_one_redir(t_redir *redir)
{
	int	target_fd;
	int	src_fd;

	src_fd = redir_get_fd(redir);
	if (redir->type == TOK_REDIR_DUP_IN || redir->type == TOK_REDIR_DUP_OUT)
		return (apply_dup_redir(redir, src_fd));
	if (redir->type == TOK_HEREDOC)
	{
		target_fd = setup_heredoc(redir);
		if (target_fd == -1)
			return (-1);
		dup2(target_fd, src_fd);
		close(target_fd);
		return (0);
	}
	target_fd = redir_open_file(redir);
	if (target_fd == -1)
		return (-1);
	dup2(target_fd, src_fd);
	close(target_fd);
	return (0);
}

/*
** Set up all redirections in the list.
** If saved_fds is not NULL, save stdin/stdout/stderr first (for restore).
** Returns 0 on success, -1 on error.
*/
int	setup_redirections(t_list *redirs, int saved_fds[3])
{
	t_list	*node;

	if (saved_fds)
	{
		saved_fds[0] = dup(0);
		saved_fds[1] = dup(1);
		saved_fds[2] = dup(2);
	}
	node = redirs;
	while (node)
	{
		if (apply_one_redir(REDIR(node)) == -1)
		{
			if (saved_fds)
				restore_redirections(saved_fds);
			return (-1);
		}
		node = node->next;
	}
	return (0);
}

/*
** Restore stdin/stdout/stderr from saved fds, then close the saved copies.
*/
void	restore_redirections(int saved_fds[3])
{
	dup2(saved_fds[0], 0);
	dup2(saved_fds[1], 1);
	dup2(saved_fds[2], 2);
	close(saved_fds[0]);
	close(saved_fds[1]);
	close(saved_fds[2]);
}
