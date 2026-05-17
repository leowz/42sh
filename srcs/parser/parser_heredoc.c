/**
 * @file parser_heredoc.c
 * @brief file to handle heredoc
 * @author jguillem
 */
#include "parser.h"

volatile sig_atomic_t	g_sigint_heredoc = 0;

/**
 * @param origin : raw string
 * @brief strip the leading tabulations of origin
 * @return an allocated cleaned string
 */
static char	*strip_tab(char *origin)
{
	while (*origin && *origin == '\t')
		origin++;
	return (strdup(origin));
}

/*
 * @param fd : the fd to write
 * @param line : the line source to read
 * @param len : the len of the line
 * @brief write until all the line will be copied
 * @return the number of bytes writed or -1 if failure
 */
static int	write_line(int fd, char *line, size_t len)
{
	size_t	total = 0;
	ssize_t	bytes = 0;

	while (total < len)
	{
		bytes = write(fd, line + total, len - total);
		if (bytes < 0)
			return (-1);
		if (!bytes)
			break;
		total += bytes;
	}
	return (0);
}

/**
 * @param redir : a pointer on struct s_redir
 * @param shell : shell state, used to reach the shared input reader
 * @param fd : the backing-store fd the heredoc body is written to
 * @brief read input line by line through the shell's single input reader
 *        (shell_read_line) so the heredoc body and the surrounding command
 *        stream stay synchronised on one stdin cursor.
 * @return 0 (success) | -1 (failure or SIGINT)
 */
static int	read_heredoc(t_redir *redir, t_shell *shell, int fd)
{
	char	*line;
	char	*convert_line;

	convert_line = NULL;
	while (!g_sigint_heredoc)
	{
		line = shell_read_line(shell, "> ");
		if (!line)
		{
			fprintf(stderr,
				"\nwarning: here-document delimited by end-of-file (wanted`%s')\n",
				redir->heredoc_delim);
			break ;
		}
		if (redir->type == TOK_HEREDOC_STRIP)
			convert_line = strip_tab(line);
		else
			convert_line = strdup(line);
		free(line);
		if (strcmp(convert_line, redir->heredoc_delim) == 0)
		{
			free(convert_line);
			break ;
		}
		if (write_line(fd, convert_line, strlen(convert_line)) == -1
			|| write(fd, "\n", 1) == -1)
		{
			free(convert_line);
			return (-1);
		}
		free(convert_line);
	}
	if (g_sigint_heredoc)
		return (-1);
	return (0);
}

/**
 * @param redir : pointer on a struct s_redir
 * @param shell : shell state
 * @brief collect one heredoc body into an unlinked temp file and store the
 *        readable fd (rewound to offset 0) in redir->heredoc_fd. There is
 *        no fork: the collector runs in the shell process and therefore
 *        shares the one stdin cursor used to read command lines.
 * @return 0 | -1
 */
static int	collect_heredoc(t_redir *redir, t_shell *shell)
{
	char	tmpl[] = "/tmp/42sh_heredoc_XXXXXX";
	int		fd;

	fd = mkstemp(tmpl);
	if (fd == -1)
		return (-1);
	unlink(tmpl);
	if (read_heredoc(redir, shell, fd) == -1)
	{
		close(fd);
		return (-1);
	}
	if (lseek(fd, 0, SEEK_SET) == -1)
	{
		close(fd);
		return (-1);
	}
	redir->heredoc_fd = fd;
	return (0);
}

/**
 * @param lst : a struct s_list pointer
 * @param shell : shell state
 * @brief helper function for collect heredocs from command or subshell
 * @details store redirections
 */
static int	collect_heredocs_from_list(t_list *lst, t_shell *shell)
{
	t_redir	*redir;

	while (lst)
	{
		redir = lst->content;
		if (redir
			&& (redir->type == TOK_HEREDOC || redir->type == TOK_HEREDOC_STRIP))
		{
			if (collect_heredoc(redir, shell) == -1)
				return (-1);
		}
		lst = lst->next;
	}
	return (0);
}

/**
 * @param cmd : struct s_cmd pointer
 * @param shell : shell state
 * @brief collect redirections of commands
 */
static int	collect_heredocs_from_command(t_cmd *cmd, t_shell *shell)
{
	if (cmd)
		return (collect_heredocs_from_list(cmd->redirs, shell));
	return (0);
}

/**
 * @param group : struct s_group pointer
 * @param shell : shell state
 * @brief collect redirections of group
 */
static int	collect_heredocs_from_group(t_group *group, t_shell *shell)
{
	if (group)
		return (collect_heredocs_from_list(group->redirs, shell));
	return (0);
}

void	heredoc_expand_config(t_redir *redir)
{
	char	quote;
	char	*end;

	if (redir->type == TOK_HEREDOC || redir->type == TOK_HEREDOC_STRIP)
	{
		if (redir->target[0] == '\'' || redir->target[0] == '"')
		{
			redir->heredoc_quoted = 1;
			quote = redir->target[0];
			end = strrchr(redir->target, quote);
			if (end > redir->target)
			{
				redir->heredoc_delim
					= strndup(redir->target + 1, end - redir->target - 1);
			}
			else
				redir->heredoc_delim = strdup(redir->target);
		}
		else
			redir->heredoc_delim = strdup(redir->target);
	}
}

/**
 * @param ast : a pointer on a struct s_ast
 * @param shell : the shell struct
 * @brief traverses the ast tree and collect heredocs content, stopping on SIGINT
 * @return 0 on success, -1 on SIGINT
 */
static int	ast_walk(t_ast *ast, t_shell *shell)
{
	if (!ast)
		return (0);
	if (ast->type == NODE_COMMAND)
		return (collect_heredocs_from_command(ast->data.cmd, shell));
	else if (ast->type == NODE_SUBSHELL || ast->type == NODE_BLOCK)
	{
		if (collect_heredocs_from_group(ast->data.group, shell) == -1)
			return (-1);
		return (ast_walk(ast->data.group->child, shell));
	}
	else if (ast->type == NODE_PIPE
		|| ast->type == NODE_AND
		|| ast->type == NODE_OR
		|| ast->type == NODE_SEQUENCE
		|| ast->type == NODE_BACKGROUND)
	{
		if (ast_walk(ast->data.binary->left, shell) == -1)
			return (-1);
		return (ast_walk(ast->data.binary->right, shell));
	}
	return (0);
}

/*
 * @brief SIGINT handler used during heredoc collect
 */
static void	heredoc_sigint_handler(int signal)
{
	(void)signal;
	g_sigint_heredoc = 1;
	write(STDIN_FILENO, "\n", 1);
	rl_replace_line("", 0);
	rl_done = 1;
}

int	parser_collect_heredocs(t_ast *ast, t_shell *shell)
{
	struct sigaction	sa_heredoc;
	struct sigaction	sa_old;
	int					ret;

	sigemptyset(&sa_heredoc.sa_mask);
	sa_heredoc.sa_flags = 0;
	sa_heredoc.sa_handler = heredoc_sigint_handler;
	sigaction(SIGINT, &sa_heredoc, &sa_old);
	if (g_sigint_heredoc)
		shell->last_exit_status = 130;
	g_sigint_heredoc = 0;
	ret = ast_walk(ast, shell);
	sigaction(SIGINT, &sa_old, NULL);
	g_sigint_heredoc = 0;
	return (ret);
}
