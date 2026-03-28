/**
 * @file parser_heredoc.c
 * @brief file to handle heredoc
 * @author jguillem
 */
#include "parser.h"

/*
 * @param begin char pointer : the begin of the dest
 * @param len size_t : the len of the dest
 * @param read size_t : the len of the src
 * @param src char pointer : the src to append
 * @brief helper function for read heredoc
 * @details allocate memory and contatenate dest and src
 * @return a char pointer on the heredoc content
 */
static char	*result_concat(char *begin, size_t *len, size_t read, char *src)
{
	char	*tmp;
	char	*result;

	tmp = realloc(begin, *len + read + 1);
	if (!tmp)
	{
		free(begin);
		return (NULL);
	}
	result = tmp;
	memcpy(result + *len, src, read);
	*len += read;
	return (result);
}

/**
 * @param delimiter : const char pointer of the string representing EOF
 * @param prompt : the prompt symbol of the heredoc
 * @brief read the user input and store each line
 * @return a char pointer on the content of the heredoc
 */
static char	*read_heredoc(const char *delimiter, const char *prompt)
{
	char		*line;
	char		*result;
	size_t		n;
	ssize_t		read;
	size_t		result_len;

	n = 0;
	result = NULL;
	line = NULL;
	result_len = 0;
	while (1)
	{
		printf("%s", prompt);
		read = getline(&line, &n, stdin);
		if (read == -1)
			break;
		line[strcspn(line, "\n")] = '\0';
		if (strcmp(line, delimiter) == 0)
			break ;
		result = result_concat(result, &result_len, (size_t)read, line);
		result = result_concat(result, &result_len, 1, "\n");
		if (!result)
			break ;
	}
	free(line);
	return (result);
}

/**
 * @param lst : a struct s_list pointer
 * @brief helper function for collect heredocs from command or subshell
 * @details store redirections
 */
static void	collect_heredocs_from_list(t_list *lst)
{
	t_redir	*redir;

	while (lst)
	{
		redir = lst->content;
		if (redir && redir->type == TOK_HEREDOC)
			redir->heredoc_content = read_heredoc(redir->heredoc_delim, "> ");
		lst = lst->next;
	}
}

/**
 * @param cmd : struct s_cmd pointer
 * @brief collect redirections of commands
 */
static void	collect_heredocs_from_command(t_cmd *cmd)
{
	t_list	*tmp;

	if (cmd)
	{
		tmp = cmd->redirs;
		collect_heredocs_from_list(tmp);
	}
}

/**
 * @param group : struct s_group pointer
 * @brief collect redirections of subshells
 */
static void	collect_heredocs_from_subshell(t_group *group)
{
	t_list	*tmp;

	if (group)
	{
		tmp = group->redirs;
		collect_heredocs_from_list(tmp);
	}
}

void	heredoc_expand_config(t_redir *redir)
{
	char	quote;
	char	*end;

	if (redir->type == TOK_HEREDOC)
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
 * @param node : a pointer on a struct s_ast
 * @brief traverses the ast tree and collect heredocs content
 */
void	ast_walk(t_ast *node)
{
	if (!node)
		return ;
	if (node->type == NODE_COMMAND)
		collect_heredocs_from_command(node->data.cmd);
	else if (node->type == NODE_SUBSHELL)
	{
		collect_heredocs_from_subshell(node->data.group);
		ast_walk(node->data.group->child);
	}
	else if (node->type == NODE_PIPE
		|| node->type == NODE_AND
		|| node->type == NODE_OR
		|| node->type == NODE_SEQUENCE
		|| node->type == NODE_BACKGROUND)
	{
		ast_walk(node->data.binary->left);
		ast_walk(node->data.binary->right);
	}
}
