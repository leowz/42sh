#include "parser.h"

/**
 * @file memory.c
 * @brief file for free memory functions
 * @author jguillem
 */

void	redir_free(t_redir *redir)
{
	if (!redir)
		return;
	if (redir->target)
		free(redir->target);
	if (redir->heredoc_delim)
		free(redir->heredoc_delim);
	if (redir->heredoc_fd >= 0)
	{
		close(redir->heredoc_fd);
		redir->heredoc_fd = -1;
	}
	free(redir);
}

static void	free_argv(char **argv)
{
	int	i;

	if (!argv)
		return;
	i = 0;
	while (argv[i])
	{
		free(argv[i]);
		i++;
	}
}

static void	cmd_free(t_cmd	*cmd)
{
	if (!cmd)
		return;
	if (cmd->argv)
	{
		free_argv(cmd->argv);
		free(cmd->argv);
	}
	if (cmd->assignments)
		ft_lstdel(&(cmd->assignments), &free);
	if (cmd->redirs)
		ft_lstdel(&(cmd->redirs), (void (*)(void *))&redir_free);
	free(cmd);
}

void	ast_free(t_ast *node)
{
	if (!node)
		return;
	
	if (node->type == NODE_COMMAND)
		cmd_free(node->data.cmd);
	else if (node->type == NODE_SUBSHELL || node->type == NODE_BLOCK
		|| node->type == NODE_BACKGROUND)
	{
		ft_lstdel(&(node->data.group->redirs), (void (*)(void *))&redir_free);
		ast_free(node->data.group->child);
		free(node->data.group);
	}
	else
	{
		ast_free(node->data.binary->left);
		ast_free(node->data.binary->right);
		free(node->data.binary);
	}
	free(node);
	node = NULL;
}
