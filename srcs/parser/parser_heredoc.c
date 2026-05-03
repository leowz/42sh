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
static char *strip_tab(char *origin)
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
 * @param prompt : the prompt symbol of the heredoc
 * @param fd : the pipe write end
 * @brief read the user input and write directly each line
 * @return 0 (success) | -1 (failure)
 */
static int	read_heredoc(t_redir *redir, const char *prompt, int fd)
{
	char		*line;
	char		*convert_line;

	while (!g_sigint_heredoc)
	{
		line = readline(prompt);
		if (!line)
		{
			fprintf(stderr,
				"\nwarning: here-document delimited by end-of-file (wanted`%s')\n",
				redir->heredoc_delim);
			break;
		}
		if (redir->type == TOK_HEREDOC_STRIP)
			convert_line = strip_tab(line);
		else
			convert_line = strdup(line);
		free(line);
		if (strcmp(convert_line, redir->heredoc_delim) == 0)
		{
			free(convert_line);
			break;
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
	{
			free(convert_line);
			return (-1);
	}
	return (0);
}

/**
 * @param redir pointer on a struct s_redir
 * @brief create a fork for the heredoc collecting
 * @brief in order to catch SIGINT only at the heredoc level
 * @return 0 | -1
 */
static int	fork_heredoc(t_redir *redir)
{
	int		pipefd[2];
	pid_t	pid;
	int		status;

	if (pipe(pipefd) == -1)
		return (-1);
	fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
	pid = fork();
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (-1);
	}
	if (pid == 0) //CHILD PROCESS
	{
		signal(SIGINT, SIG_DFL);
		close(pipefd[0]);
		if (read_heredoc(redir, "> ", pipefd[1]) == -1)
		{
			close(pipefd[1]);
			exit(130);
		}
		close(pipefd[1]);
		exit(0);
	}
	else // PARENT PROCESS
	{
		close(pipefd[1]);
		waitpid(pid, &status, 0);
		if (WIFSIGNALED(status) || WEXITSTATUS(status) == 130)
		{
			close(pipefd[0]);
			g_sigint_heredoc = 1;
			return (-1);
		}
		redir->heredoc_fd = pipefd[0];
		return (0);
	}
}

/**
 * @param lst : a struct s_list pointer
 * @brief helper function for collect heredocs from command or subshell
 * @details store redirections
 */
static int	collect_heredocs_from_list(t_list *lst)
{
	t_redir	*redir;

	while (lst)
	{
		redir = lst->content;
		if (redir &&
			(redir->type == TOK_HEREDOC || redir->type == TOK_HEREDOC_STRIP))
		{
			if (fork_heredoc(redir) == -1)
				return (-1);
		}
		lst = lst->next;
	}
	return (0);
}

/**
 * @param cmd : struct s_cmd pointer
 * @brief collect redirections of commands
 */
static int	collect_heredocs_from_command(t_cmd *cmd)
{
	if (cmd)
		collect_heredocs_from_list(cmd->redirs);
	return (0);
}

/**
 * @param group : struct s_group pointer
 * @brief collect redirections of group
 */
static int	collect_heredocs_from_group(t_group *group)
{
	if (group)
		collect_heredocs_from_list(group->redirs);
	return (0);
}

/**
 * @param redir : t_redir struct
 * @brief fill the heredoc_delim and heredoc_quoted fields of the t_redir struct
 */
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
		return(0) ;
	if (ast->type == NODE_COMMAND)
		return (collect_heredocs_from_command(ast->data.cmd));
	else if (ast->type == NODE_SUBSHELL || ast->type == NODE_BLOCK)
	{
		if (collect_heredocs_from_group(ast->data.group) == -1)
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
		return (ast_walk(ast->data.binary->right,shell));
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

/**
 * @param ast : struct s_ast pointer
 * @param shell : struct s_shell pointer
 * @brief stop ast walking and collect heredoc on SIGINT
 * @return 0 on success, -1 on SIGINT
 */
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
