/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/27 00:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/03/27 00:00:00 by wengzhang        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "42sh.h"
#include "executor.h"

/*
** Set up a heredoc: create a pipe, write the collected content to
** the write end, close the write end, and return the read end fd.
** Returns the read-end fd on success, -1 on error.
*/
int	setup_heredoc(t_redir *redir)
{
	int		pipefd[2];
	size_t	len;

	if (pipe(pipefd) == -1)
	{
		ft_putstr_fd("42sh: pipe: ", 2);
		ft_putendl_fd(strerror(errno), 2);
		return (-1);
	}
	if (redir->heredoc_content)
	{
		len = ft_strlen(redir->heredoc_content);
		write(pipefd[1], redir->heredoc_content, len);
	}
	close(pipefd[1]);
	return (pipefd[0]);
}
