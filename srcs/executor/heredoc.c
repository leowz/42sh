/**
 * @file heredoc.c
 * @brief Heredoc functionality for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"

/**
 * @brief Set up a heredoc: create a pipe, write the collected content to
 * @details the write end, close the write end, and return the read end fd.
 * @param redir The redirection structure.
 * @return The read-end fd on success, -1 on error.
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
