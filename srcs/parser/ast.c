#include "parser.h"

void	redir_free(t_redir *redir)
{
	if (!redir)
		return;
	if (redir->target)
		free(redir->target);
	if (redir->heredoc_delim)
		free(redir->heredoc_delim);
	if (redir->heredoc_content)
		free(redir->heredoc_content);	
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
	else if (node->type == NODE_SUBSHELL || node->type == NODE_BLOCK)
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
}
