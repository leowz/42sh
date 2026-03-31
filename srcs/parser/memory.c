#include "parser.h"

/**
 * @file memory.c
 * @brief file for free memory functions
 * @author jguillem
 */

/**
 * @param redir t_redir struct
 * @brief free t_redir helper
 */
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

/**
 * @param argv array of strings
 * @brief free argv
 */
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

/**
 * @param cmd t_cmd struct
 * @brief free t_cmd
 */
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

/**
 * @param node t_ast struct
 * @brief free ast recursively
 */
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
	node = NULL;
}
